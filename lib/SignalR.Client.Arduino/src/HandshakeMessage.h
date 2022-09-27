#ifndef _HANDSHAKEREQUESTMESSAGE_h
#define _HANDSHAKEREQUESTMESSAGE_h

#include "ArduinoJson.hpp"
#include "signalrclient/json_helpers.h"

class HandshakeRequestMessage
{
public:
	const char* protocol;
	int version;

	String Serialize()
	{
		ArduinoJson::StaticJsonDocument<60> doc;
		String res;

		doc["protocol"] = protocol;
		doc["version"] = version;

		serializeJson(doc, res);
		return res + signalr::record_separator;
	}
};

class HandshakeResponseMessage
{
public:
	const char* error;

	void Deserialize(uint8_t* payload, size_t length)
	{
		ArduinoJson::StaticJsonDocument<400> doc;
		ArduinoJson::deserializeJson(doc, payload, length-1);

		if(doc.containsKey("ProtocolVersion"))
		{
			error = "Detected a connection attempt to an ASP.NET SignalR Server. This client only supports connecting to an ASP.NET Core SignalR Server. See https://aka.ms/signalr-core-differences for details.";
			return;
		}

		error = doc["error"];
	}
};

#endif