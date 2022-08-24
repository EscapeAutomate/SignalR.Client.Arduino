/*#include "SignalR.Client.Arduino.h"
#include "WiFi.h"

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);

	WiFi.begin("OMEGA-0000", "134Apwwn44");

	SignalRClient.Setup("192.168.1.164", 6000, "/TestHub", "user", "password");
	SignalRClient.UseMessagepack();

	SignalRClient.On("ReceivedMessage", ReceivedMessage);

	ArduinoJson::DynamicJsonDocument doc(200);

	SignalRClient.Invoke("EchoMessage", doc);
}

// the loop function runs over and over again until power down or reset
void loop() {
	SignalRClient.Loop();
}

void ReceivedMessage(ArduinoJson::DynamicJsonDocument* doc)
{
	Serial.print("ReceivedMessage: ");
	Serial.println("message");
}
*/

#include "SignalR_Client_Arduino.h"

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
}

// the loop function runs over and over again until power down or reset
void loop() {
}