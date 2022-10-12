#include "SignalRHub.h"
#include "signalrclient/handshake_protocol.h"

SignalRHub::SignalRHub(const std::string& url, trace_level trace_level, const log_writer& log_writer, bool skip_negotiation) :
    m_callback_manager("connection went out of scope before invocation result was received"),
    m_connection(url, trace_level, log_writer, skip_negotiation),
    m_logger(log_writer, trace_level)
{
}

void SignalRHub::Setup(bool useMsgPack)
{
    if (useMsgPack)
	{
		m_hub_protocol = new messagepack_hub_protocol();
	}
	else
	{
		m_hub_protocol = new json_hub_protocol();
	}
}



void SignalRHub::Start(std::function<void(std::exception_ptr)> callback) noexcept
{
    if (m_connection.get_connection_state() != connection_state::disconnected)
    {
        callback(std::make_exception_ptr(signalr_exception(
            "the connection can only be started if it is in the disconnected state")));
        return;
    }

    m_handshakeReceived = false;
    connection_impl* weak_connection = &m_connection;
    hub_protocol* weak_hub_protocol = m_hub_protocol;
    m_connection.start([weak_connection, weak_hub_protocol, callback](std::exception_ptr start_exception)
        {
            auto connection = weak_connection;
            auto hub_protocol = weak_hub_protocol;

            if (start_exception)
            {
                if(connection->get_connection_state() == connection_state::disconnected)
                    // connection didn't start, don't call stop
                    callback(start_exception);
                return;
            }

            auto handshake_request = handshake::write_handshake(hub_protocol);

            connection->send(handshake_request, hub_protocol->transfer_format(), [callback](std::exception_ptr exception)
            {
                if (exception)
                {
                    callback(exception);
                    return;
                }
            });
        });
}

void SignalRHub::Stop(std::function<void(std::exception_ptr)> callback) noexcept
{
    if (m_connection.get_connection_state() == connection_state::disconnected)
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
        m_connection.stop([weak_connection](std::exception_ptr exception)
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
        case message_type::invocation:
            auto invocation = static_cast<invocation_message*>(msg.get());
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
        
        case message_type::completion:
            {
                auto completion = static_cast<completion_message*>(msg.get());
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

void SignalRHub::On(const std::string& event_name, const std::function<void(const std::vector<value>&)>& handler)
{
	if (event_name.length() == 0)
	{
		throw std::invalid_argument("event_name cannot be empty");
	}

	if (m_subscriptions.find(event_name) != m_subscriptions.end())
	{
		throw signalr_exception("an action for this event has already been registered. event name: " + event_name);
	}

	m_subscriptions.insert({event_name, handler});
}

void SignalRHub::Invoke(const std::string& method_name, const std::vector<value>& arguments, std::function<void(const value&, std::exception_ptr)> callback) noexcept
{
    const auto& callback_id = m_callback_manager.register_callback(
        create_hub_invocation_callback([callback](const value& result) { callback(result, nullptr); },
            [callback](const std::exception_ptr e) { callback(value(), e); }));

    Invoke_hub_method(method_name, arguments, callback_id, nullptr,
        [callback](const std::exception_ptr e){ callback(value(), e); });
}

void SignalRHub::Send(const std::string& method_name, const std::vector<value>& arguments, std::function<void(std::exception_ptr)> callback) noexcept
{
    Invoke_hub_method(method_name, arguments, "",
        [callback]() { callback(nullptr); },
        [callback](const std::exception_ptr e){ callback(e); });
}

void SignalRHub::Invoke_hub_method(const std::string& method_name, const std::vector<value>& arguments,
    const std::string& callback_id, std::function<void()> set_completion, std::function<void(const std::exception_ptr)> set_exception) noexcept
{
    try
    {
        invocation_message invocation(callback_id, method_name, arguments);
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

static std::function<void(const char* error, const value&)> create_hub_invocation_callback(
            const std::function<void(const value&)>& set_result,
            const std::function<void(const std::exception_ptr)>& set_exception)
{
    return [set_result, set_exception](const char* error, const value& message)
    {
        if (error != nullptr)
        {
            set_exception(std::make_exception_ptr(hub_exception(error)));
        }
        else
        {
            set_result(message);
        }
    };
}