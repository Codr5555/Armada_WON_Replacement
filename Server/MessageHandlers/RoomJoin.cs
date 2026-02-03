using System.Text;

namespace ArmadaServer {
	internal partial class Network {
		internal void RoomJoin(OutgoingTCPMessageID messageID,Room room) {
			//Prevent other players from joining rooms while the new room player list is being sent.
			//This avoids scenarios where player list changes occur in the middle of building the list of players for the currently-joining player, which would lead to desynchronization of the list.
			lock (Server.playersRoomLock) {
				room.AddPlayer(Player,false);

				QueueMessage(messageID,Encoding.Latin1.GetBytes(room.Name),[0],room.GetPlayerData());
			}
		}

		internal void RoomJoin(Span<byte> data) {
			var length = BitConverter.ToInt32(data);
			var name = Encoding.Latin1.GetString(data.Slice(4,length));

			var passwordLength = BitConverter.ToInt32(data.Slice(4 + length));
			var password = Encoding.Latin1.GetString(data.Slice(4 + length + 4,passwordLength));

			if (!Server.Rooms.TryGetValue(name,out var room) || room.Password != password) {
				QueueBooleanMessage(OutgoingTCPMessageID.RoomJoin,0);
				return;
			}

			RoomJoin(OutgoingTCPMessageID.RoomJoin,room);
		}
	}
}