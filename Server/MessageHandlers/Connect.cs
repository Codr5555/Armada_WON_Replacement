namespace ArmadaServer {
	internal partial class Network {
		internal void Connect(Span<byte> data) {
			if (!data.SequenceEqual(authenticationToken)) {
				throw new Exception();
			}

			if (Server.Players.ContainsKey(TCPSocket.RemoteEndPoint!.GetIPv4Address())) {
				throw new Exception("A connection is already established with this IP address.");
			}

			Player = new Player(this);
			Server.Players[Player.IPAddress] = Player;

			QueuePingRequest();
			QueueMessage(OutgoingTCPMessageID.Connect,BitConverter.GetBytes(Player.IPAddress));
		}
	}
}