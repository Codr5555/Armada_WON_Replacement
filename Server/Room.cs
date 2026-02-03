namespace ArmadaServer {
	internal class Room {
		internal string Name { get; }
		internal string Password { get; }

		internal int PlayerCount => Server.Players.Where(e => e.Value.Room == this).Count();

		internal Room(string name) {
			Name = name;
			Password = "";
		}

		internal Room(string name,string password) {
			Name = name;
			Password = password;
		}

		internal void AddPlayer(Player player,bool sendUpdateToAddedPlayer = true) {
			var oldRoom = player.Room;
			player.Room = this;

			var addPlayerMessage = Network.CreateAddPlayerMessageData(false,player);

			byte[] leaveMessage = [];
			if (oldRoom != null && oldRoom != this) {
				if (oldRoom.PlayerCount > 0) {
					leaveMessage = player.Network.GetRemovePlayerMessageData(oldRoom,false);
				}
				else if (oldRoom.Name != "Lobby") {
					Server.Rooms.Remove(oldRoom.Name,out _);
				}
			}

			foreach (var (_,globalPlayer) in Server.Players) {
				if (globalPlayer.Game != null) {
					continue;
				}

				if (oldRoom != null && oldRoom != this && globalPlayer.Room == oldRoom) {
					globalPlayer.Network.QueueMessage(Network.OutgoingTCPMessageID.RemovePlayers,leaveMessage);
				}
				else if (globalPlayer.Room == this && (globalPlayer != player || sendUpdateToAddedPlayer)) {
					globalPlayer.Network.QueueMessage(Network.OutgoingTCPMessageID.AddPlayers,addPlayerMessage);
				}
			}
		}

		internal void SendRemovePlayerMessages(Player player) {
			var messageData = player.Network.GetRemovePlayerMessageData(player.Room,false);
			foreach (var (_,globalPlayer) in Server.Players) {
				if (globalPlayer != player && globalPlayer.Room == this && globalPlayer.Game == null) {
					globalPlayer.Network.QueueMessage(Network.OutgoingTCPMessageID.RemovePlayers,messageData);
				}
			}
		}

		internal byte[] GetPlayerData() {
			var data = new List<byte>();

			int count = 0;
			foreach (var (_,player) in Server.Players) {
				if (player.Room == this && player.Game == null) {
					data.AddRange(player.GetBytes());
					count++;
				}
			}
			data.InsertRange(0,BitConverter.GetBytes(count));

			return data.ToArray();
		}
	}
}
