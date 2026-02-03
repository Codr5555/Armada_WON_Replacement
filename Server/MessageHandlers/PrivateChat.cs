namespace ArmadaServer {
	internal partial class Network {
		internal void PrivateChat(Span<byte> data) {
			var recipientIPAddress = BitConverter.ToUInt32(data);
			if (!Server.Players.TryGetValue(recipientIPAddress,out var recipient)) {
				//Ignore the message; the recipient may have logged off in the interim.
				return;
			}

			BitConverter.GetBytes(Player.IPAddress).CopyTo(data);

			QueueMessage(OutgoingTCPMessageID.Chat,data);
			if (recipient != Player) {
				recipient.Network.QueueMessage(OutgoingTCPMessageID.Chat,data);
			}
		}
	}
}