using System.Net.Sockets;
using Serilog;

namespace ArmadaServer {
	internal partial class Network {
		internal void Data(Span<byte> data) {
			Log.Information($"  ({data.Length} bytes): {OutputHelper.GetByteString(data)}");

			if (Player.Game == null) {
				throw new Exception("Data was called without a game assigned to the player.");
			}

			if (!Server.Players.ContainsKey(Player.Game.Host.IPAddress)) {
				return;
			}

			int messageLength = 4 + data.Length;
			var buffer = new byte[messageLength];

			BitConverter.GetBytes(Player.IPAddress).CopyTo(buffer);
			data.CopyTo(new Span<byte>(buffer,4,data.Length));

			foreach (var (_,gamePlayer) in Player.Game.Players) {
				if (gamePlayer == Player) {
					continue;
				}

				Console.WriteLine($"  Sending to {gamePlayer.Account}.");
				if (Server.useUDP && Player.Game.InProgress) {
					gamePlayer.Network.OutgoingUDPMessages.Add(buffer);
				}
				else {
					gamePlayer.Network.QueueMessage(OutgoingTCPMessageID.Data,buffer);
				}
			}
		}

		internal void TCPData(Span<byte> data) {
			Data(data);
		}

		internal void UDPData(SocketAsyncEventArgs socketData,Span<byte> data) {
			Data(data);
		}
	}
}