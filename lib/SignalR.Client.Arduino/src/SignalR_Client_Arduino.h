#ifndef _SignalR_h
#define _SignalR_h

#if defined(ARDUINO)
#include <WebSocketsClient.h>
#include "SignalRHub.h"

class SignalRClientClass
{
private:
	bool waitForHandshake = false;
	SignalRHub* hub;

	void WebSocketEvent(WStype_t type, uint8_t* payload, size_t length);

public:
	void Setup(const String& address, uint16_t port, const String& path, bool useMsgPack = false, const String& username = "", const String& password = "");
	void On(const std::string& event_name, const std::function<void(const std::vector<signalr::value>&)>& handler);
	void Loop();
};

extern SignalRClientClass SignalRClient;
#endif

//#include <signalrclient/hub_connection_builder.h>

//typedef std::function<void(ArduinoJson::DynamicJsonDocument* doc)> SignalRClientOnEvent;

#endif

