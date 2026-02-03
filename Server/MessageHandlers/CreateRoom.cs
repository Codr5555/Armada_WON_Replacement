using System.Text;

namespace ArmadaServer {
	internal partial class Network {
		internal void CreateRoom(Span<byte> data) {
			var length = BitConverter.ToInt32(data);
			var name = Encoding.Latin1.GetString(data.Slice(4,length));

			var passwordLength = BitConverter.ToInt32(data.Slice(4 + length));
			var password = Encoding.Latin1.GetString(data.Slice(4 + length + 4,passwordLength));

			if (Server.Rooms.ContainsKey(name)) {
				QueueBooleanMessage(OutgoingTCPMessageID.RoomCreate,0);
				return;
			}

			var room = new Room(name,password);
			Server.Rooms.TryAdd(name,room);

			RoomJoin(OutgoingTCPMessageID.RoomCreate,room);
		}
	}
}