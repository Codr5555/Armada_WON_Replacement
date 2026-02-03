using System.Text;

namespace ArmadaServer {
	internal partial class Network {
		internal void JoinGame(Span<byte> data) {
			var hostIPAddress = BitConverter.ToUInt32(data);

			var passwordLength = BitConverter.ToInt32(data.Slice(4));
			var password = Encoding.Latin1.GetString(data.Slice(8,passwordLength));

			if (!Server.Games.TryGetValue(hostIPAddress,out var game)) {
				QueueMessage(OutgoingTCPMessageID.JoinGame);
				return;
			}

			lock (game.playersLock) {
				if (!game.Players.TryAdd(Player.IPAddress,Player)) {
					QueueMessage(OutgoingTCPMessageID.JoinGame);
					return;
				}

				var message = CreateAddPlayerMessageData(false,Player);
				foreach (var (_,player) in game.Players) {
					player.Network.QueueMessage(OutgoingTCPMessageID.AddPlayers,message);
				}
			}

			Player.Game = game;

			lock (Server.playersRoomLock) {
				lock (game.playersLock) {
					var playerData = game.GetPlayerData();
					var messageData = new byte[4 + playerData.Length];
					BitConverter.GetBytes(game.Host.IPAddress).CopyTo(messageData,0);
					playerData.CopyTo(messageData,4);
					QueueMessage(OutgoingTCPMessageID.JoinGame,messageData);
				}
			}

			Player.PendingActionCancel = new Timer(
				(_) => {
					Player.Game = null;
					QueueMessage(OutgoingTCPMessageID.AddPlayers,[1],Player.Room.GetPlayerData());
				},
				null,TimeSpan.FromSeconds(20),Timeout.InfiniteTimeSpan
			);
		}
	}
}