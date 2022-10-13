#pragma once

#include "signalrclient/transport.h"

#if defined(ARDUINO)
#include <WebSocketsClient.h>

class websocket_transport : public transport
{
public:
    websocket_transport();

    void start(const std::string& url, std::function<void(std::exception_ptr)> callback) noexcept override;
    void stop(std::function<void(std::exception_ptr)> callback) noexcept override;

    void send(const std::string& payload, signalr::transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept override;

    void on_receive(std::function<void(std::string&&, std::exception_ptr)>) override;

private:

    void WebSocketEvent(WStype_t type, uint8_t* payload, size_t length);

    std::function<void(std::string, std::exception_ptr)> m_process_response_callback;
    WebSocketsClient webSocket;
};
#else
typedef enum {
    WStype_ERROR,
    WStype_DISCONNECTED,
    WStype_CONNECTED,
    WStype_TEXT,
    WStype_BIN,
    WStype_FRAGMENT_TEXT_START,
    WStype_FRAGMENT_BIN_START,
    WStype_FRAGMENT,
    WStype_FRAGMENT_FIN,
    WStype_PING,
    WStype_PONG,
} WStype_t;

class websocket_transport : public transport
{
public:
    websocket_transport();

    void start(const std::string& url, std::function<void(std::exception_ptr)> callback) noexcept override;
    void stop(std::function<void(std::exception_ptr)> callback) noexcept override;

    void send(const std::string& payload, signalr::transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept override;

    void on_receive(std::function<void(std::string&&, std::exception_ptr)>) override;

private:

    void WebSocketEvent(WStype_t type, uint8_t* payload, size_t length);

    std::function<void(std::string, std::exception_ptr)> m_process_response_callback;
};
#endif