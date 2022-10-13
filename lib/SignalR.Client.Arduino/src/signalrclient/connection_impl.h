// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <mutex>
#include <atomic>
#include "signalrclient/trace_level.h"
#include "signalrclient/connection_state.h"
#include "signalrclient/transfer_format.h"
#include "signalrclient/transport.h"
#include "signalrclient/websocket_transport.h"
#include "logger.h"

namespace signalr
{    
    class connection_impl
    {
    public:
        connection_impl(const std::string& url, trace_level trace_level, log_writer* log_writer, bool skip_negotiation);

        void start(std::function<void(std::exception_ptr)> callback) noexcept;
        void send(const std::string &data, transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept;
        void stop(std::function<void(std::exception_ptr)> callback, std::exception_ptr exception) noexcept;

        connection_state get_connection_state() const noexcept;
        void set_message_received(const std::function<void(std::string&&)>& message_received);
        void set_disconnected(const std::function<void(std::exception_ptr)>& disconnected);

    private:
        std::string m_base_url;
        std::atomic<connection_state> m_connection_state;
        logger m_logger;
        transport* m_transport;
        bool m_skip_negotiation;
        std::exception_ptr m_stop_error;

        std::function<void(std::string&&)> m_message_received;
        std::function<void(std::exception_ptr)> m_disconnected;

        std::mutex m_stop_lock;
        std::string m_connection_id;
        std::string m_connection_token;

        void start_transport(const std::string& url);
        void send_connect_request(const std::shared_ptr<transport>& transport,
            const std::string& url, std::function<void(std::exception_ptr)> callback);
        void start_negotiate(const std::string& url, std::function<void(std::exception_ptr)> callback);
        void start_negotiate_internal(const std::string& url, int redirect_count);

        void process_response(std::string&& response);

        void shutdown(std::function<void(std::exception_ptr)> callback);
        void stop_connection(std::exception_ptr);

        bool change_state(connection_state old_state, connection_state new_state);
        connection_state change_state(connection_state new_state);
        void handle_connection_state_change(connection_state old_state, connection_state new_state);
        void invoke_message_received(std::string&& message);

        static std::string translate_connection_state(connection_state state);
        void ensure_disconnected(const std::string& error_message) const;
    };
}
