#ifndef _HANDSHAKEREQUESTMESSAGE_h
#define _HANDSHAKEREQUESTMESSAGE_h

#include "ArduinoJson.hpp"

#define HANDSHAKETERMINATOR String((char)0x1E)
#define JSON_PROTOCOL "json"
#define MESSAGEPACK_PROTOCOL "messagepack"

class BaseMessage
{
public:
	virtual String Serialize() = 0;
	virtual void Deserialize(uint8_t* payload, size_t length) = 0;
};

class HandshakeRequestMessage : public BaseMessage
{
public:
	char* protocol;
	int version;

	String Serialize()
	{
		ArduinoJson::StaticJsonDocument<60> doc;
		String res;

		doc["protocol"] = protocol;
		doc["version"] = version;

		serializeJson(doc, res);
		return res + HANDSHAKETERMINATOR;
	}

	void Deserialize(uint8_t* payload, size_t length)
	{

	}
};

class HandshakeResponseMessage : public BaseMessage
{
public:
	const char* error;

	String Serialize()
	{
		String res;
		return res;
	}

	void Deserialize(uint8_t* payload, size_t length)
	{
		ArduinoJson::StaticJsonDocument<200> doc;
		ArduinoJson::deserializeJson(doc, payload, length-1);

		error = doc["error"];
	}
};

#endif