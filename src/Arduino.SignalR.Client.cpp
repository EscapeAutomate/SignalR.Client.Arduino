#include "Arduino.SignalR.Client.h"

WebSocketsClient webSocket;
SignalRClientClass SignalRClient;

void SignalRClientClass::WebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {

	switch (type) {
	case WStype_DISCONNECTED:
		Serial.printf("[WSc] Disconnected!\n");
		break;
	case WStype_CONNECTED:
		Serial.printf("[WSc] Connected to url: %s\n", payload);

		// send message to server when Connected
		webSocket.sendTXT("Connected");
		break;
	case WStype_TEXT:
	case WStype_BIN:
		if (waitForHandshake)
		{
			waitForHandshake = false;
			HandshakeResponseMessage message;
			message.Deserialize(payload, length);
		}
		else
		{

		}

		break;
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

	needSendHandshake = true;
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

	if (needSendHandshake)
	{
		needSendHandshake = false;
		waitForHandshake = true;

		HandshakeRequestMessage* message = new HandshakeRequestMessage();
		message->version = 1;

		if (useMessagepack)
		{
			message->protocol = MESSAGEPACK_PROTOCOL;
		}
		else
		{
			message->protocol = JSON_PROTOCOL;
		}

		Send(message);
	}
}

void SignalRClientClass::Send(BaseMessage* message)
{
	String str = message->Serialize();

	webSocket.sendBIN((uint8_t*)str.c_str(), str.length());
}