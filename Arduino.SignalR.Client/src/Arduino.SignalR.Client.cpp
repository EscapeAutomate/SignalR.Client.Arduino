#include "Arduino.SignalR.Client.h"
#include "WebSockets/WebSocketsClient.h"

WebSocketsClient webSocket;
SignalRClientClass SignalRClient;

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {

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
		Serial.printf("[WSc] get text: %s\n", payload);

		// send message to server
		// webSocket.sendTXT("message here");
		break;
	case WStype_BIN:
		Serial.printf("[WSc] get binary length: %u\n", length);

		// send data to server
		// webSocket.sendBIN(payload, length);
		break;
	case WStype_ERROR:
	case WStype_FRAGMENT_TEXT_START:
	case WStype_FRAGMENT_BIN_START:
	case WStype_FRAGMENT:
	case WStype_FRAGMENT_FIN:
		break;
	}

}

void SignalRClientClass::Setup(String address, int port, String path, String username, String password)
{
	// server address, port and URL
	webSocket.begin("192.168.0.123", 81, "/");

	// event handler
	webSocket.onEvent(webSocketEvent);

	// use HTTP Basic Authorization this is optional remove if not needed
	webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);
}

void SignalRClientClass::UseMessagePack()
{

}

void SignalRClientClass::On(char* fctName, SignalRClientOnEvent cbEvent)
{

}

void SignalRClientClass::Connect()
{

}

void SignalRClientClass::Invoke(char* fctName, const ArduinoJson::DynamicJsonDocument& doc)
{

}