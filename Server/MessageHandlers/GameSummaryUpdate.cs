using System.Text;

namespace ArmadaServer {
	internal partial class Network {
		internal void GameSummaryUpdate(Span<byte> data) {
			if (Player.Game == null) {
				return;
			}

			if (Player != Player.Game.Host) {
				throw new Exception("Invalid request.");
			}

			Player.Game.Flags = BitConverter.ToUInt32(data);

			int stringLength = data.Slice(4,12).IndexOf<byte>(0);
			if (stringLength == -1) {
				stringLength = 12;
			}
			Player.Game.Map = Encoding.Latin1.GetString(data.Slice(4,stringLength));
		}
	}
}