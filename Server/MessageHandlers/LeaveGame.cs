namespace ArmadaServer {
	internal partial class Network {
		internal void LeaveGame(Span<byte> data) {
			if (Player.Game == null) {
				QueueMessage(OutgoingTCPMessageID.LeaveGame);
				return;
			}

			var IPAddress = Player.IPAddress;
			if (Player.Game.Host.IPAddress == IPAddress) {
				Server.Games.TryRemove(IPAddress,out _);
			}
			else {
				lock (Player.Game.playersLock) {
					Player.Game.Players.TryRemove(IPAddress,out _);
				}
			}
			Player.Game = null;

			lock (Server.playersRoomLock) {
				//This is necessary because the player appears to not be in the room while in a game, despite still being assigned to the room.
				Player.Room.AddPlayer(Player,false);

				QueueMessage(OutgoingTCPMessageID.LeaveGame,Player.Room.GetPlayerData());
			}
		}
	}
}