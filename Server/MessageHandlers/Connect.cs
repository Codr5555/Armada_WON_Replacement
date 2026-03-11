using System.Net;
using Serilog;

namespace ArmadaServer {
	internal partial class Network {
		internal void Connect(Span<byte> data) {
			if (!data.SequenceEqual(authenticationToken)) {
				throw new Exception();
			}

			if (Server.Players.TryGetValue(TCPSocket.RemoteEndPoint!.GetIPv4Address(),out var player)) {
				//Assume the existing connection is no longer valid and disconnect it.
				Log.Information($"A connection is being made by {new IPAddress(player.IPAddress)} that already has an active connection.  The old connection is being dropped.");
				player.Disconnect();
			}

			Player = new Player(this);
			Server.Players[Player.IPAddress] = Player;

			QueuePingRequest();
			QueueMessage(OutgoingTCPMessageID.Connect,BitConverter.GetBytes(Player.IPAddress),BitConverter.GetBytes(Server.useUDP));
		}
	}
}