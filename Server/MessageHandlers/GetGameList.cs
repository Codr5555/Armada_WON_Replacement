using System.Net;
using System.Text;

namespace ArmadaServer {
	internal partial class Network {
		internal void GetGameList(Span<byte> data) {
			var gameList = Server.Games.Where(e => e.Value.Host.Room == Player.Room);

			var output = new List<byte>();
			output.AddRange(new byte[messageHeaderLength]);
			output.AddRange(BitConverter.GetBytes((short)gameList.Count()));

			foreach (var game in gameList) {
				output.AddRange(Encoding.Latin1.GetBytes(game.Value.Name));
				output.Add(0);
				output.AddRange(Encoding.Latin1.GetBytes(new IPAddress(game.Value.Host.IPAddress).ToString()));
				output.Add(0);
				output.AddRange(BitConverter.GetBytes(game.Value.Flags));
				output.AddRange(Encoding.Latin1.GetBytes(game.Value.Map));
				output.Add(0);
			}

			var outputBytes = output.ToArray();
			WriteSendBufferHeader(outputBytes,OutgoingTCPMessageID.GetGameList,outputBytes.Length - messageHeaderLength);
			OutgoingTCPMessages.Add(outputBytes);
		}
	}
}