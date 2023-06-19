#if defined(ARDUINO)
#include <Arduino.h>
#include "SignalR_Client_Arduino.h"

void setup() {
  std::promise<void> start_task;
  signalr::hub_connection connection = signalr::hub_connection_builder::create("http://localhost:5000/hub").build();

  connection.on("Echo", [](const std::vector<signalr::value>& m)
  {
      std::cout << m[0].as_string() << std::endl;
  });

  connection.start([&start_task](std::exception_ptr exception) {
      start_task.set_value();
  });

  start_task.get_future().get();

  std::promise<void> send_task;
  std::vector<signalr::value> args { "Hello world" };
  connection.invoke("Echo", args, [&send_task](const signalr::value& value, std::exception_ptr exception) {
      send_task.set_value();
  });

  send_task.get_future().get();

  std::promise<void> stop_task;
  connection.stop([&stop_task](std::exception_ptr exception) {
      stop_task.set_value();
  });

  stop_task.get_future().get();
}

void loop() {
  // put your main code here, to run repeatedly:
}
#else

int main(int argc, char **argv)
{
	return 0;
}
#endif