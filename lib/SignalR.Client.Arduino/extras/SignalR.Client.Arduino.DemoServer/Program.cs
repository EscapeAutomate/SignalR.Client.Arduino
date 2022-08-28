namespace SignalR.Client.Arduino.DemoServer
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var builder = WebApplication.CreateBuilder(args);

            builder.Services.AddSignalR().AddMessagePackProtocol();

            var app = builder.Build();

            app.MapHub<TestHub>("/TestHub");

            app.Run();
        }
    }
}