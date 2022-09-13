#ifndef _SignalR_h
#define _SignalR_h

#if defined(ARDUINO)
#include <WebSocketsClient.h>
#endif

//#include <signalrclient/hub_connection_builder.h>

//typedef std::function<void(ArduinoJson::DynamicJsonDocument* doc)> SignalRClientOnEvent;

class SignalRClientClass
{
private:
	/*bool useMessagepack = false;
	bool waitForHandshake = false;
	std::map<std::string, SignalRClientOnEvent> cbls;

	void Send(BaseMessage* message);
	void WebSocketEvent(WStype_t type, uint8_t* payload, size_t length);

public:
	void Setup(const String& address, uint16_t port, const String& path, const String& username = "", const String& password = "");
	void UseMessagepack();
	void On(char* fctName, SignalRClientOnEvent cbEvent);
	void Invoke(char* fctName, const ArduinoJson::DynamicJsonDocument& doc);
	void Loop();*/
};

extern SignalRClientClass SignalRClient;

#endif

