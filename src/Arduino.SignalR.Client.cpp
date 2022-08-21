#include "Arduino.SignalR.Client.h"

WebSocketsClient webSocket;
SignalRClientClass SignalRClient;

void hexdump(const void* mem, uint32_t len, uint8_t cols = 16) {
	const uint8_t* src = (const uint8_t*)mem;
	for (uint32_t i = 0; i < len; i++) {
		Serial.printf("%02X ", *src);
		src++;
	}
	Serial.printf("\n");
}

void SignalRClientClass::WebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {

	switch (type) {
	case WStype_DISCONNECTED:
		Serial.printf("[WSc] Disconnected!\n");
		break;
	case WStype_CONNECTED:
	{
		Serial.printf("[WSc] Connected to url: %s\n", payload);

		waitForHandshake = true;

		HandshakeRequestMessage* handshakeRequestMessage = new HandshakeRequestMessage();
		handshakeRequestMessage->version = 1;

		if (useMessagepack)
		{
			handshakeRequestMessage->protocol = MESSAGEPACK_PROTOCOL;
		}
		else
		{
			handshakeRequestMessage->protocol = JSON_PROTOCOL;
		}

		String str = handshakeRequestMessage->Serialize();
		Serial.println("[SR] Sending handshake!");
		Serial.println(str);

		webSocket.sendTXT(str);
		break;
	}		
	case WStype_TEXT:
		Serial.printf("[WSc] message received: %s\n", payload);
		Serial.printf("[WSc] message received len: %i\n", length);

		if (waitForHandshake)
		{
			if (payload[length-1] != 0x1E)
			{
				//bad handshake
				webSocket.disconnect();
				return;
			}
			waitForHandshake = false;
			HandshakeResponseMessage handshakeResponseMessage;
			handshakeResponseMessage.Deserialize(payload, length);
			Serial.print("[SR] Received handshake! Data in error: ");
			Serial.println(handshakeResponseMessage.error);
			return;
		}

		break;
	case WStype_BIN:
		hexdump(payload+1, length-1);

		if (waitForHandshake)
		{
			if (payload[length - 1] != 0x1E)
			{
				Serial.println("[SR] BAD handshake! Closing!");
				//bad handshake
				webSocket.disconnect();
				return;
			}
			waitForHandshake = false;
			HandshakeResponseMessage handshakeResponseMessage;
			handshakeResponseMessage.Deserialize(payload, length);
			Serial.print("[SR] Received handshake! Data in error: ");
			Serial.println(handshakeResponseMessage.error);
			return;
		}

		break;
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

void SignalRClientClass::Setup(const String& address, uint16_t port, const String& path, const String& username, const String& password)
{
	if (webSocket.isConnected())
	{
		webSocket.disconnect();
	}

	webSocket.begin(address, port, path);

	webSocket.onEvent(std::bind(&SignalRClientClass::WebSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	if (!username.isEmpty())
	{
		// use HTTP Basic Authorization this is optional remove if not needed
		webSocket.setAuthorization(username.c_str(), password.c_str());
	}

	webSocket.setReconnectInterval(5000);
}

void SignalRClientClass::UseMessagepack()
{
	useMessagepack = true;
}

void SignalRClientClass::On(char* fctName, SignalRClientOnEvent cbEvent)
{
	if (cbls.count(fctName) == 0)
	{
		cbls.insert(std::pair<std::string, SignalRClientOnEvent>(fctName, cbEvent));
	}
}

void SignalRClientClass::Invoke(char* fctName, const ArduinoJson::DynamicJsonDocument& doc)
{

}

void SignalRClientClass::Loop()
{
	webSocket.loop();
}

void SignalRClientClass::Send(BaseMessage* message)
{
	String str = message->Serialize();

	webSocket.sendBIN((uint8_t*)str.c_str(), str.length());
}