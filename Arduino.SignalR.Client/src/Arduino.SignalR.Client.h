#ifndef _Arduino_h
#define _Arduino_h

#include "ArduinoJson.hpp"

typedef std::function<void(ArduinoJson::DynamicJsonDocument* doc)> SignalRClientOnEvent;

class SignalRClientClass
{
private:

public:
	void Setup(String address, int port, String path, String username = "", String password = "");
	void UseMessagePack();
	void On(char* fctName, SignalRClientOnEvent cbEvent);
	void Connect();
	void Invoke(char* fctName, const ArduinoJson::DynamicJsonDocument& doc);
};

extern SignalRClientClass SignalRClient;

#endif

