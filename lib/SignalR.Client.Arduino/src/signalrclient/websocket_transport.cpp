#include "signalrclient/websocket_transport.h"

#if defined(ARDUINO)

websocket_transport::websocket_transport() : transport()
{
    
}

void websocket_transport::WebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
	switch (type) {
		
	case WStype_DISCONNECTED:
		Serial.printf("[WSc] Disconnected!\n");
		break;

	case WStype_CONNECTED:
	{
		Serial.printf("[WSc] Connected to url: %s\n", payload);

		/*waitForHandshake = true;

		HandshakeRequestMessage* handshakeRequestMessage = new HandshakeRequestMessage();
		handshakeRequestMessage->version = 1;
		handshakeRequestMessage->protocol = hub->name().c_str();

		String str = handshakeRequestMessage->Serialize();
		Serial.println("[SR] Sending handshake!");

		webSocket.sendTXT(str);*/
		break;
	}

	case WStype_TEXT:
	case WStype_BIN:
	{
		/*if (waitForHandshake)
		{
			waitForHandshake = false;

			if (payload[length - 1] != 0x1E)
			{
				//bad handshake
				webSocket.disconnect();
				return;
			}
			
			HandshakeResponseMessage handshakeResponseMessage;
			handshakeResponseMessage.Deserialize(payload, length);

			if (strlen(handshakeResponseMessage.error) != 0)
			{
				Serial.print("[SR] Received handshake with error: ");
				Serial.println(handshakeResponseMessage.error);
				webSocket.disconnect();
				return;
			}

			Serial.println("[SR] Received handshake");
			return;
		}
		else
		{
			hub->HandleMessage((char*)payload);
		}*/

		break;
	}
		
	case WStype_PING:
	case WStype_PONG:
	case WStype_ERROR:
	case WStype_FRAGMENT_TEXT_START:
	case WStype_FRAGMENT_BIN_START:
	case WStype_FRAGMENT:
	case WStype_FRAGMENT_FIN:
		break;
	}
}

void websocket_transport::on_receive(std::function<void(std::string&&, std::exception_ptr)> callback)
{
    m_process_response_callback = callback;
}

void websocket_transport::send(const std::string& payload, signalr::transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept
{
    //TODO send
}
#else
websocket_transport::websocket_transport() : transport()
{
    
}

void websocket_transport::WebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
}

void websocket_transport::on_receive(std::function<void(std::string&&, std::exception_ptr)> callback)
{
    m_process_response_callback = callback;
}

void websocket_transport::send(const std::string& payload, signalr::transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept
{
    //TODO send
}
#endif

//m_process_response_callback(message, nullptr);