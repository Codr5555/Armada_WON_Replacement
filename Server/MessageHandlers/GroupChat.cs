namespace ArmadaServer {
	internal partial class Network {
		internal void GroupChat(Span<byte> data) {
			if (Player.Game == null) {
				throw new Exception("The player isn't in a game.");
			}

			int targetCount = BitConverter.ToInt32(data);

			var targetsLength = 4 + 4 * targetCount;
			var textLength = data.Length - targetsLength;

			var message = CreateBuffer(OutgoingTCPMessageID.Chat,4 + textLength);
			BitConverter.GetBytes(Player.IPAddress).CopyTo(new Span<byte>(message,messageHeaderLength,4));
			data.Slice(targetsLength,textLength).CopyTo(new Span<byte>(message,messageHeaderLength + 4,textLength));

			bool messagedSelf = false;
			for (int index = 0;index < targetCount;index++) {
				if (!Player.Game.Players.TryGetValue(BitConverter.ToUInt32(data.Slice(4 + index * 4)),out var target)) {
					continue;
				}

				target.Network.OutgoingTCPMessages.Add(message);

				if (target == Player) {
					messagedSelf = true;
				}
			}

			if (!messagedSelf) {
				OutgoingTCPMessages.Add(message);
			}
		}
	}
}