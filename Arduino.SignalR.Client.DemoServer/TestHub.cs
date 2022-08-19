using Microsoft.AspNetCore.SignalR;

namespace Arduino.SignalR.Client.DemoServer
{
    public class TestHub : Hub
    {
        public async Task EchoMessage(string message)
        {
            await Clients.All.SendAsync("ReceivedMessage", message);
        }
    }
}
