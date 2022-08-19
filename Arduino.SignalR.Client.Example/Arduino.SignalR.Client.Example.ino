#include "Arduino.SignalR.Client.h"

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);

	SignalRClient.Setup("192.168.1.1", 5000, "/TestHub", "user", "password");
	SignalRClient.UseMessagePack();

	SignalRClient.On("ReceivedMessage", ReceivedMessage);

	SignalRClient.Connect();

	ArduinoJson::DynamicJsonDocument doc(200);

	SignalRClient.Invoke("EchoMessage", doc);
}

// the loop function runs over and over again until power down or reset
void loop() {
  
}

void ReceivedMessage(ArduinoJson::DynamicJsonDocument* doc)
{
	Serial.print("ReceivedMessage: ");
	Serial.println("message");
}
