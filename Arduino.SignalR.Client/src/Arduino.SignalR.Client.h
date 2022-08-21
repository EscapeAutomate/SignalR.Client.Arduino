#ifndef _Arduino_h
#define _Arduino_h

#include "ArduinoJson.hpp"
#include "Models/Messages.h"
#include "WebSockets/WebSocketsClient.h"
#include <map>

typedef std::function<void(ArduinoJson::DynamicJsonDocument* doc)> SignalRClientOnEvent;

class SignalRClientClass
{
private:
	bool useMessagepack = false;
	bool needSendHandshake = false;
	bool waitForHandshake = false;
	std::map<std::string, SignalRClientOnEvent> cbls;

public:
	void Setup(const String& address, uint16_t port, const String& path, const String& username = "", const String& password = "");
	void UseMessagepack();
	void On(char* fctName, SignalRClientOnEvent cbEvent);
	void Send(BaseMessage* message);
	void WebSocketEvent(WStype_t type, uint8_t* payload, size_t length);
	void Invoke(char* fctName, const ArduinoJson::DynamicJsonDocument& doc);
	void Loop();
};

extern SignalRClientClass SignalRClient;

#endif

