using System.Text;

namespace ArmadaServer {
	internal partial class Network {
		internal void GetRooms(Span<byte> data) {
			var output = new List<byte>();
			output.AddRange(new byte[messageHeaderLength]);
			output.AddRange(BitConverter.GetBytes((short)Server.Rooms.Count));
			foreach (var room in Server.Rooms) {
				output.AddRange(Encoding.Latin1.GetBytes(room.Value.Name));
				output.Add(0);
				output.AddRange(BitConverter.GetBytes(room.Value.Password.Length > 0));
				output.AddRange(BitConverter.GetBytes(room.Value.PlayerCount));
			}

			var outputBytes = output.ToArray();
			WriteSendBufferHeader(outputBytes,OutgoingTCPMessageID.GetRooms,outputBytes.Length - messageHeaderLength);
			OutgoingTCPMessages.Add(outputBytes);
		}
	}
}