#include "SignalRHub.h"

SignalRHub::SignalRHub(signalr::log_writer* logger, signalr::trace_level logLevel) : m_callback_manager("connection went out of scope before invocation result was received")
{
}

void SignalRHub::Setup(bool useMsgPack)
{
    if (useMsgPack)
	{
		hub_protocol = new signalr::messagepack_hub_protocol();
	}
	else
	{
		hub_protocol = new signalr::json_hub_protocol();
	}
}

void SignalRHub::Start(std::function<void(std::exception_ptr)> callback) noexcept
{
    if (m_connection->get_connection_state() != connection_state::disconnected)
    {
        callback(std::make_exception_ptr(signalr_exception(
            "the connection can only be started if it is in the disconnected state")));
        return;
    }

    m_connection->set_client_config(m_signalr_client_config);
    m_handshakeTask = std::make_shared<completion_event>();
    m_disconnect_cts = std::make_shared<cancellation_token_source>();
    m_handshakeReceived = false;
    std::weak_ptr<hub_connection_impl> weak_connection = shared_from_this();
    m_connection->start([weak_connection, callback](std::exception_ptr start_exception)
        {
            auto connection = weak_connection.lock();
            if (!connection)
            {
                // The connection has been destructed
                callback(std::make_exception_ptr(signalr_exception("the hub connection has been deconstructed")));
                return;
            }

            if (start_exception)
            {
                assert(connection->get_connection_state() == connection_state::disconnected);
                // connection didn't start, don't call stop
                callback(start_exception);
                return;
            }

            std::shared_ptr<std::mutex> handshake_request_lock = std::make_shared<std::mutex>();
            std::shared_ptr<bool> handshake_request_done = std::make_shared<bool>();

            auto handle_handshake = [weak_connection, handshake_request_done, handshake_request_lock, callback](std::exception_ptr exception, bool fromSend)
            {
                assert(fromSend ? *handshake_request_done : true);

                auto connection = weak_connection.lock();
                if (!connection)
                {
                    // The connection has been destructed
                    callback(std::make_exception_ptr(signalr_exception("the hub connection has been deconstructed")));
                    return;
                }

                {
                    std::lock_guard<std::mutex> lock(*handshake_request_lock);
                    // connection.send will be waiting on the handshake task which has been set by the caller already
                    if (!fromSend && *handshake_request_done == true)
                    {
                        return;
                    }
                    *handshake_request_done = true;
                }

                try
                {
                    if (exception == nullptr)
                    {
                        connection->m_handshakeTask->get();
                        callback(nullptr);
                    }
                }
                catch (...)
                {
                    exception = std::current_exception();
                }

                if (exception != nullptr)
                {
                    connection->m_connection->stop([callback, exception](std::exception_ptr)
                        {
                            callback(exception);
                        }, exception);
                }
                else
                {
                    connection->start_keepalive();
                }
            };

            auto handshake_request = handshake::write_handshake(connection->m_protocol);
            auto handshake_task = connection->m_handshakeTask;
            auto handshake_timeout = connection->m_signalr_client_config.get_handshake_timeout();

            connection->m_disconnect_cts->register_callback([handle_handshake, handshake_request_lock, handshake_request_done]()
                {
                    {
                        std::lock_guard<std::mutex> lock(*handshake_request_lock);
                        // no op after connection.send returned, m_handshakeTask should be set before m_disconnect_cts is canceled
                        if (*handshake_request_done == true)
                        {
                            return;
                        }
                    }

                    // if the request isn't completed then no one is waiting on the handshake task
                    // so we need to run the callback here instead of relying on connection.send completing
                    // handshake_request_done is set in handle_handshake, don't set it here
                    handle_handshake(nullptr, false);
                });

            timer(connection->m_signalr_client_config.get_scheduler(),
                [handle_handshake, handshake_task, handshake_timeout, handshake_request_lock](std::chrono::milliseconds duration)
                {
                    {
                        std::lock_guard<std::mutex> lock(*handshake_request_lock);

                        // if the task is set then connection.send is either already waiting on the handshake or has completed,
                        // or stop has been called and will be handling the callback
                        if (handshake_task->is_set())
                        {
                            return true;
                        }

                        if (duration < handshake_timeout)
                        {
                            return false;
                        }
                    }

                    auto exception = std::make_exception_ptr(signalr_exception("timed out waiting for the server to respond to the handshake message."));
                    // unblocks connection.send if it's waiting on the task
                    handshake_task->set(exception);

                    handle_handshake(exception, false);
                    return true;
                });

            connection->m_connection->send(handshake_request, connection->m_protocol->transfer_format(),
                [handle_handshake, handshake_request_done, handshake_request_lock](std::exception_ptr exception)
            {
                {
                    std::lock_guard<std::mutex> lock(*handshake_request_lock);
                    if (*handshake_request_done == true)
                    {
                        // callback ran from timer or cancellation token, nothing to do here
                        return;
                    }

                    // indicates that the handshake timer doesn't need to call the callback, it just needs to set the timeout exception
                    // handle_handshake will be waiting on the handshake completion (error or success) to call the callback
                    *handshake_request_done = true;
                }

                handle_handshake(exception, true);
            });
        });
}

void SignalRHub::Stop(std::function<void(std::exception_ptr)> callback) noexcept
{
    if (get_connection_state() == connection_state::disconnected)
    {
        m_logger.log(trace_level::info, "Stop ignored because the connection is already disconnected.");
        callback(nullptr);
        return;
    }
    else
    {
        {
            std::lock_guard<std::mutex> lock(m_stop_callback_lock);
            m_stop_callbacks.push_back(callback);

            if (m_stop_callbacks.size() > 1)
            {
                m_logger.log(trace_level::info, "Stop is already in progress, waiting for it to finish.");
                // we already registered the callback
                // so we can just return now as the in-progress stop will trigger the callback when it completes
                return;
            }
        }
        std::weak_ptr<hub_connection_impl> weak_connection = shared_from_this();
        m_connection->stop([weak_connection](std::exception_ptr exception)
            {
                auto connection = weak_connection.lock();
                if (!connection)
                {
                    return;
                }

                assert(connection->get_connection_state() == connection_state::disconnected);

                std::vector<std::function<void(std::exception_ptr)>> callbacks;

                {
                    std::lock_guard<std::mutex> lock(connection->m_stop_callback_lock);
                    // copy the callbacks out and clear the list inside the lock
                    // then run the callbacks outside of the lock
                    callbacks = connection->m_stop_callbacks;
                    connection->m_stop_callbacks.clear();
                }

                for (auto& callback : callbacks)
                {
                    callback(exception);
                }
            }, nullptr);
    }
}

void SignalRHub::HandleMessage(const std::string& message)
{
    auto messages = hub_protocol->parse_messages(message);

    for (auto &msg : messages)
    {
        switch (msg->message_type)
        {
        case signalr::message_type::invocation:
            auto invocation = static_cast<signalr::invocation_message*>(msg.get());
            auto event = m_subscriptions.find(invocation->target);
            if (event != m_subscriptions.end())
            {
                const auto& args = invocation->arguments;
                event->second(args);
            }
            else
            {
                //m_logger.log(trace_level::info, "handler not found");
            }
            break;
        
        case signalr::message_type::completion:
            {
                auto completion = static_cast<signalr::completion_message*>(msg.get());
                const char* error = nullptr;
                if (!completion->error.empty())
                {
                    error = completion->error.data();
                }

                // TODO: consider transferring ownership of 'result' so that if we run user callbacks on a different thread we don't need to
                // worry about object lifetime
                if (!m_callback_manager.invoke_callback(completion->invocation_id, error, completion->result, true))
                {
                    /*if (m_logger.is_enabled(trace_level::info))
                    {
                        m_logger.log(trace_level::info, std::string("no callback found for id: ").append(completion->invocation_id));
                    }*/
                }
                break;
            }
        }
    }
    
}

void SignalRHub::On(const std::string& event_name, const std::function<void(const std::vector<signalr::value>&)>& handler)
{
	if (event_name.length() == 0)
	{
		throw std::invalid_argument("event_name cannot be empty");
	}

	if (m_subscriptions.find(event_name) != m_subscriptions.end())
	{
		throw signalr::signalr_exception("an action for this event has already been registered. event name: " + event_name);
	}

	m_subscriptions.insert({event_name, handler});
}

void SignalRHub::Invoke(const std::string& method_name, const std::vector<signalr::value>& arguments, std::function<void(const signalr::value&, std::exception_ptr)> callback) noexcept
{
    const auto& callback_id = m_callback_manager.register_callback(
        create_hub_invocation_callback([callback](const signalr::value& result) { callback(result, nullptr); },
            [callback](const std::exception_ptr e) { callback(signalr::value(), e); }));

    Invoke_hub_method(method_name, arguments, callback_id, nullptr,
        [callback](const std::exception_ptr e){ callback(signalr::value(), e); });
}

void SignalRHub::Send(const std::string& method_name, const std::vector<signalr::value>& arguments, std::function<void(std::exception_ptr)> callback) noexcept
{
    Invoke_hub_method(method_name, arguments, "",
        [callback]() { callback(nullptr); },
        [callback](const std::exception_ptr e){ callback(e); });
}

void SignalRHub::Invoke_hub_method(const std::string& method_name, const std::vector<signalr::value>& arguments,
    const std::string& callback_id, std::function<void()> set_completion, std::function<void(const std::exception_ptr)> set_exception) noexcept
{
    try
    {
        signalr::invocation_message invocation(callback_id, method_name, arguments);
        auto message = hub_protocol->write_message(&invocation);

        // weak_ptr prevents a circular dependency leading to memory leak and other problems
        auto weak_hub_connection = std::weak_ptr<hub_connection_impl>(shared_from_this());

        m_connection->send(message, hub_protocol->transfer_format(), [set_completion, set_exception, weak_hub_connection, callback_id](std::exception_ptr exception)
            {
                if (exception)
                {
                    auto hub_connection = weak_hub_connection.lock();
                    if (hub_connection)
                    {
                        hub_connection->m_callback_manager.remove_callback(callback_id);
                    }
                    set_exception(exception);
                }
                else
                {
                    if (callback_id.empty())
                    {
                        // complete nonBlocking call
                        set_completion();
                    }
                }
            });

        //reset_send_ping();
    }
    catch (const std::exception& e)
    {
        m_callback_manager.remove_callback(callback_id);
        /*if (m_logger.is_enabled(trace_level::warning))
        {
            m_logger.log(trace_level::warning, std::string("failed to send invocation: ").append(e.what()));
        }*/
        set_exception(std::current_exception());
    }
}

static std::function<void(const char* error, const signalr::value&)> create_hub_invocation_callback(
            const std::function<void(const signalr::value&)>& set_result,
            const std::function<void(const std::exception_ptr)>& set_exception)
{
    return [set_result, set_exception](const char* error, const signalr::value& message)
    {
        if (error != nullptr)
        {
            set_exception(std::make_exception_ptr(signalr::hub_exception(error)));
        }
        else
        {
            set_result(message);
        }
    };
}