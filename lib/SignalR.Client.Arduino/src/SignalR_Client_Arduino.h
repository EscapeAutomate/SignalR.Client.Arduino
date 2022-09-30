#ifndef _SignalR_h
#define _SignalR_h

#if defined(ARDUINO)
#include "SignalRHub.h"
#include "signalrclient/log_writer.h"
#include "signalrclient/trace_level.h"

class SignalRClientClass
{
private:
	SignalRHub* hub;

public:
	void Setup(const String& address, uint16_t port, const String& path, bool useMsgPack = false, const String& username = "", const String& password = "",
	signalr::log_writer* logger = nullptr, signalr::trace_level logLevel = signalr::trace_level::verbose);
	void On(const std::string& event_name, const std::function<void(const std::vector<signalr::value>&)>& handler);
	void Loop();
};

extern SignalRClientClass SignalRClient;
#endif

#endif

