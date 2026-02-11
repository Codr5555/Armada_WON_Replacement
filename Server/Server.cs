using System.Collections.Concurrent;
using Serilog;

namespace ArmadaServer {
	internal static class Server {
		internal static ConcurrentDictionary<uint,Player> Players { get; } = [];
		internal static ConcurrentDictionary<uint,Game> Games { get; } = [];
		internal static ConcurrentDictionary<string,Room> Rooms { get; } = [];

		internal static Lock playersRoomLock = new();
		internal static bool useUDP = false;

		internal static async Task Main(string[] arguments) {
			var loggingPath = Path.Combine(Directory.GetCurrentDirectory(),"Logs");

			Log.Logger = new LoggerConfiguration()
				.MinimumLevel.Debug()
				.WriteTo.Console()
				.WriteTo.File(Path.Combine(loggingPath,"Log - .txt"),rollingInterval: RollingInterval.Day)
				.CreateLogger();

			Rooms.TryAdd("Lobby",new Room("Lobby"));

			try {
				var handler = new SocketHandler(arguments[0]);

				if (arguments.Length > 1) {
					useUDP = string.Equals(arguments[1],"UDP",StringComparison.OrdinalIgnoreCase);
				}

				Log.Information($"Server started.\n  Listening via {handler.IPAddress}:{handler.Port}.\n  Logging path: {loggingPath}\n  Game data is sent via {(useUDP ? "UDP" : "TCP")}.");
				await handler.StartAsync();
			}
			catch (ArgumentException exception) {
				Console.WriteLine(exception.Message);
				Console.WriteLine("Expected arguments: <IPv4 address>:<port> [UDP]");
				Console.WriteLine("  UDP can be specified to enable UDP data transmission during game sessions.  Any packet loss seems to freeze Armada in the current state, however.");
				return;
			}
			catch (Exception exception) {
				Log.Error(exception,"An error occurred trying to start the socket listener.");
			}

			await Task.Delay(Timeout.Infinite);
		}
	}
}