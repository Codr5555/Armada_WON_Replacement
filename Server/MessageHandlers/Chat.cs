namespace ArmadaServer {
	internal partial class Network {
		internal void Chat(Span<byte> data) {
			int length = BitConverter.ToInt32(data);

			var message = CreateBuffer(OutgoingTCPMessageID.Chat,4 + length);
			BitConverter.GetBytes(Player.IPAddress).CopyTo(new Span<byte>(message,messageHeaderLength,4));
			data.Slice(4,length).CopyTo(new Span<byte>(message,messageHeaderLength + 4,length));

			var players = Server.Players.Where(e => e.Value.Room == Player.Room && e.Value.Game == Player.Game);
			foreach (var player in players) {
				player.Value.Network.OutgoingTCPMessages.Add(message);
			}
		}
	}
}