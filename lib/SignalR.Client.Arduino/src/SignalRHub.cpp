#include "SignalRHub.h"
#include "signalrclient/handshake_protocol.h"

// unnamed namespace makes it invisble outside this translation unit
namespace
{
    static std::function<void(const char*, const signalr::value&)> create_hub_invocation_callback(
        const std::function<void(const signalr::value&)>& set_result,
        const std::function<void(const std::exception_ptr e)>& set_exception);
}

SignalRHub::SignalRHub(const std::string& url, trace_level trace_level, log_writer* log_writer, bool skip_negotiation) :
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
        connection_impl* weak_connection = &m_connection;
        m_connection.stop([weak_connection, callback](std::exception_ptr exception)
            {
                if (exception)
                {
                    callback(exception);
                    return;
                }
            }, nullptr);
    }
}

void SignalRHub::HandleMessage(const std::string& message)
{
    auto messages = m_hub_protocol->parse_messages(message);

    for (auto &msg : messages)
    {
        switch (msg->message_type)
        {
        case message_type::invocation:
        {
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
        }
        
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
        auto message = m_hub_protocol->write_message(&invocation);
        callback_manager* callback_manager = &m_callback_manager;

        m_connection.send(message, m_hub_protocol->transfer_format(), [set_completion, set_exception, callback_manager, callback_id](std::exception_ptr exception)
            {
                if (exception)
                {
                    callback_manager->remove_callback(callback_id);
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