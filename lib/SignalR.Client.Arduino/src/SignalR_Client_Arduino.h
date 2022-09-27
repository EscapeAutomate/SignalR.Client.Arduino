#ifndef _SignalR_h
#define _SignalR_h

#if defined(ARDUINO)
#include <unordered_map>
#include <WebSocketsClient.h>
#include "signalrclient/hub_protocol.h"
#include "signalrclient/messagepack_hub_protocol.h"
#include "signalrclient/json_hub_protocol.h"
#include "signalrclient/case_insensitive_comparison_utils.h"
#include "signalrclient/signalr_exception.h"

class SignalRClientClass
{
private:
	bool waitForHandshake = false;
	signalr::hub_protocol* hub_protocol;
	std::unordered_map<std::string, std::function<void(const std::vector<signalr::value>&)>, signalr::case_insensitive_hash, signalr::case_insensitive_equals> m_subscriptions;

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

