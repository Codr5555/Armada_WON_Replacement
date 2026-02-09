using System.Net;
using System.Net.Sockets;
using Serilog;

namespace ArmadaServer {
	internal partial class Network {
		internal void DataTargetted(Span<byte> data) {
			var targetIPAddress = BitConverter.ToUInt32(data);

			if (!Server.Players.TryGetValue(targetIPAddress,out var targetPlayer)) {
				Log.Error($"A targetted data request was received for {new IPAddress(targetIPAddress)}, but there wasn't a player object matching it.  The request was ignored.");
				return;
			}

			if (Player.Game != null && data[4..][0] == 13 && targetPlayer.IPAddress == Player.Game.Host.IPAddress && Player.PendingActionCancel != null) {
				Player.PendingActionCancel.Dispose();
				Player.PendingActionCancel = null;

				Player.Room.SendRemovePlayerMessages(Player);
			}

			Log.Information($"  ({data.Length - 4} bytes) Target({new IPAddress(targetIPAddress)}): {OutputHelper.GetByteString(data.Slice(4))}");

			//The data already includes an IP address at the beginning, so adding length isn't needed.
			var buffer = new byte[data.Length];

			BitConverter.GetBytes(Player.IPAddress).CopyTo(buffer,0);
			data[4..].CopyTo(new Span<byte>(buffer,4,data.Length - 4));

			if (Player.Game != null && Player.Game.InProgress) {
				targetPlayer.Network.OutgoingUDPMessages.Add(buffer);
			}
			else {
				targetPlayer.Network.QueueMessage(OutgoingTCPMessageID.Data,buffer);
			}
		}

		internal void TCPDataTargetted(Span<byte> data) {
			DataTargetted(data);
		}

		internal void UDPDataTargetted(SocketAsyncEventArgs socketData,Span<byte> data) {
			DataTargetted(data);
		}
	}
}