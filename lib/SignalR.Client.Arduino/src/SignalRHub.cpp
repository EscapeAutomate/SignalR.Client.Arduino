#include "SignalRHub.h"

SignalRHub::SignalRHub() : m_callback_manager("connection went out of scope before invocation result was received")
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
        create_hub_invocation_callback(m_logger, [callback](const signalr::value& result) { callback(result, nullptr); },
            [callback](const std::exception_ptr e) { callback(signalr::value(), e); }));

    invoke_hub_method(method_name, arguments, callback_id, nullptr,
        [callback](const std::exception_ptr e){ callback(signalr::value(), e); });
}

void SignalRHub::Send(const std::string& method_name, const std::vector<signalr::value>& arguments, std::function<void(std::exception_ptr)> callback) noexcept
{
    invoke_hub_method(method_name, arguments, "",
        [callback]() { callback(nullptr); },
        [callback](const std::exception_ptr e){ callback(e); });
}

void SignalRHub::Invoke_hub_method(const std::string& method_name, const std::vector<signalr::value>& arguments,
    const std::string& callback_id, std::function<void()> set_completion, std::function<void(const std::exception_ptr)> set_exception) noexcept
{
    try
    {
        invocation_message invocation(callback_id, method_name, arguments);
        auto message = m_protocol->write_message(&invocation);

        // weak_ptr prevents a circular dependency leading to memory leak and other problems
        auto weak_hub_connection = std::weak_ptr<hub_connection_impl>(shared_from_this());

        m_connection->send(message, m_protocol->transfer_format(), [set_completion, set_exception, weak_hub_connection, callback_id](std::exception_ptr exception)
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

        reset_send_ping();
    }
    catch (const std::exception& e)
    {
        m_callback_manager.remove_callback(callback_id);
        if (m_logger.is_enabled(trace_level::warning))
        {
            m_logger.log(trace_level::warning, std::string("failed to send invocation: ").append(e.what()));
        }
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
            set_exception(std::make_exception_ptr(hub_exception(error)));
        }
        else
        {
            set_result(message);
        }
    };
}