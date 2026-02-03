using System.Text;

namespace ArmadaServer {
	internal partial class Network {
		internal void CreateGame(Span<byte> data) {
			byte nameLength = data[0];
			string name = Encoding.Latin1.GetString(data.Slice(1,nameLength));
			var index = 1 + nameLength;

			byte passwordLength = data.Slice(index)[0];
			string password = Encoding.Latin1.GetString(data.Slice(index + 1,passwordLength));
			index += 1 + passwordLength;

			uint flags = BitConverter.ToUInt32(data.Slice(index));
			index += 4;

			byte mapNameLength = data.Slice(index)[0];
			string mapName = Encoding.Latin1.GetString(data.Slice(index + 1,mapNameLength));

			var game = new Game(name,Player,flags,mapName);
			Server.Games.TryAdd(Player.IPAddress,game);

			Player.Game = game;

			lock (Server.playersRoomLock) {
				Player.Room.SendRemovePlayerMessages(Player);

				QueueBooleanMessage(OutgoingTCPMessageID.CreateGame,1);
			}
		}
	}
}