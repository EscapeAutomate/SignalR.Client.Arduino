using Microsoft.AspNetCore.SignalR;

namespace SignalR.Client.Arduino.DemoServer
{
    public class TestHub : Hub
    {
        public async Task EchoMessage(string message)
        {
            await Clients.All.SendAsync("ReceivedMessage", message);
        }
    }
}
