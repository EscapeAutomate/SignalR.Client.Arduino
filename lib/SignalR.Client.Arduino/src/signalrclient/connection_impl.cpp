// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include <algorithm>
#include "signalrclient/connection_impl.h"
#include "signalrclient/signalr_exception.h"
#include "case_insensitive_comparison_utils.h"
#include <assert.h>

namespace signalr
{
    connection_impl::connection_impl(const std::string& url, trace_level trace_level, const log_writer& log_writer, bool skip_negotiation)
        : m_base_url(url), m_logger(log_writer, trace_level), m_transport(new websocket_transport()), m_skip_negotiation(skip_negotiation),
        m_message_received([](const std::string&) noexcept {}), m_disconnected([](std::exception_ptr) noexcept {})
    {
    }

    void connection_impl::start(std::function<void(std::exception_ptr)> callback) noexcept
    {
        {
            std::lock_guard<std::mutex> lock(m_stop_lock);
            if (!change_state(connection_state::disconnected, connection_state::connecting))
            {
                callback(std::make_exception_ptr(signalr_exception("cannot start a connection that is not in the disconnected state")));
                return;
            }

            m_connection_id = "";
        }

        start_negotiate(m_base_url, callback);
    }

    void connection_impl::start_negotiate(const std::string& url, std::function<void(std::exception_ptr)> callback)
    {
        if (m_skip_negotiation)
        {
            // TODO: check that the websockets transport is explicitly selected

            return start_transport(url);
        }

        start_negotiate_internal(url, 0);
    }

    void connection_impl::start_negotiate_internal(const std::string& url, int redirect_count)
    {

    }

    void connection_impl::start_transport(const std::string& url)
    {
    }
    void connection_impl::process_response(std::string&& response)
    {
        if (m_logger.is_enabled(trace_level::debug))
        {
            // TODO: log binary data better
            m_logger.log(trace_level::debug,
                std::string("processing message: ").append(response));
        }

        invoke_message_received(std::move(response));
    }

    void connection_impl::invoke_message_received(std::string&& message)
    {
        try
        {
            m_message_received(std::move(message));
        }
        catch (const std::exception &e)
        {
            if (m_logger.is_enabled(trace_level::error))
            {
                m_logger.log(
                    trace_level::error,
                    std::string("message_received callback threw an exception: ")
                    .append(e.what()));
            }
        }
        catch (...)
        {
            m_logger.log(trace_level::error, "message_received callback threw an unknown exception");
        }
    }

    void connection_impl::send(const std::string& data, transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept
    {
        if (m_logger.is_enabled(trace_level::info))
        {
            m_logger.log(trace_level::info, std::string("sending data: ").append(data));
        }

        auto logger = m_logger;

        m_transport->send(data, transfer_format, [logger, callback](std::exception_ptr exception)
            mutable {
                try
                {
                    if (exception != nullptr)
                    {
                        std::rethrow_exception(exception);
                    }
                    callback(nullptr);
                }
                catch (const std::exception &e)
                {
                    if (logger.is_enabled(trace_level::error))
                    {
                        logger.log(
                            trace_level::error,
                            std::string("error sending data: ")
                            .append(e.what()));
                    }

                    callback(exception);
                }
            });
    }

    void connection_impl::stop(std::function<void(std::exception_ptr)> callback, std::exception_ptr exception) noexcept
    {
        m_stop_error = exception;
        m_logger.log(trace_level::info, "stopping connection");

        shutdown(callback);
    }

    // do not use `shared_from_this` as it can be called via the destructor
    void connection_impl::stop_connection(std::exception_ptr error)
    {
        {
            // the lock prevents a race where the user calls `stop` on a disconnected connection and calls `start`
            // on a different thread at the same time. In this case we must not null out the transport if we are
            // not in the `disconnecting` state to not affect the 'start' invocation.
            std::lock_guard<std::mutex> lock(m_stop_lock);

            if (m_connection_state == connection_state::disconnected)
            {
                m_logger.log(trace_level::info, "Stopping was ignored because the connection is already in the disconnected state.");
                return;
            }

            // if we have a m_stop_error, it takes precedence over the error from the transport
            if (m_stop_error)
            {
                error = m_stop_error;
                m_stop_error = nullptr;
            }

            change_state(connection_state::disconnected);
            m_transport = nullptr;
        }

        if (error)
        {
            try
            {
                std::rethrow_exception(error);
            }
            catch (const std::exception & ex)
            {
                if (m_logger.is_enabled(trace_level::error))
                {
                    m_logger.log(trace_level::error, std::string("Connection closed with error: ").append(ex.what()));
                }
            }
        }
        else
        {
            m_logger.log(trace_level::info, "Connection closed.");
        }

        try
        {
            m_disconnected(error);
        }
        catch (const std::exception & e)
        {
            if (m_logger.is_enabled(trace_level::error))
            {
                m_logger.log(
                    trace_level::error,
                    std::string("disconnected callback threw an exception: ")
                    .append(e.what()));
            }
        }
        catch (...)
        {
            m_logger.log(
                trace_level::error,
                "disconnected callback threw an unknown exception");
        }
    }

    void connection_impl::set_message_received(const std::function<void(std::string&&)>& message_received)
    {
        ensure_disconnected("cannot set the callback when the connection is not in the disconnected state. ");
        m_message_received = message_received;
    }

    void connection_impl::set_disconnected(const std::function<void(std::exception_ptr)>& disconnected)
    {
        ensure_disconnected("cannot set the disconnected callback when the connection is not in the disconnected state. ");
        m_disconnected = disconnected;
    }

    void connection_impl::ensure_disconnected(const std::string& error_message) const
    {
        const auto state = get_connection_state();
        if (state != connection_state::disconnected)
        {
            throw signalr_exception(
                error_message + "current connection state: " + translate_connection_state(state));
        }
    }

    bool connection_impl::change_state(connection_state old_state, connection_state new_state)
    {
        if (m_connection_state.compare_exchange_strong(old_state, new_state, std::memory_order_seq_cst))
        {
            handle_connection_state_change(old_state, new_state);
            return true;
        }

        return false;
    }

    connection_state connection_impl::change_state(connection_state new_state)
    {
        auto old_state = m_connection_state.exchange(new_state);
        if (old_state != new_state)
        {
            handle_connection_state_change(old_state, new_state);
        }

        return old_state;
    }

    void connection_impl::handle_connection_state_change(connection_state old_state, connection_state new_state)
    {
        if (m_logger.is_enabled(trace_level::verbose))
        {
            m_logger.log(
                trace_level::verbose,
                translate_connection_state(old_state)
                .append(" -> ")
                .append(translate_connection_state(new_state)));
        }

        // Words of wisdom (if we decide to add a state_changed callback and invoke it from here):
        // "Be extra careful when you add this callback, because this is sometimes being called with the m_stop_lock.
        // This could lead to interesting problems.For example, you could run into a segfault if the connection is
        // stopped while / after transitioning into the connecting state."
    }

    std::string connection_impl::translate_connection_state(connection_state state)
    {
        switch (state)
        {
        case connection_state::connecting:
            return "connecting";
        case connection_state::connected:
            return "connected";
        case connection_state::disconnecting:
            return "disconnecting";
        case connection_state::disconnected:
            return "disconnected";
        default:
            assert(false);
            return "(unknown)";
        }
    }
}
