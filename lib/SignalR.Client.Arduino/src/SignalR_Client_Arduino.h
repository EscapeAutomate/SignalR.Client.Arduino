#ifndef _SignalR_h
#define _SignalR_h

#if defined(ARDUINO)
#include "SignalRHub.h"

class SignalRClientClass
{
private:
	SignalRHub* hub;

public:
	void Setup(const String& address, uint16_t port, const String& path, bool useMsgPack = false, const String& username = "", const String& password = "");
	void On(const std::string& event_name, const std::function<void(const std::vector<signalr::value>&)>& handler);
	void Loop();
};

extern SignalRClientClass SignalRClient;
#endif

#endif

