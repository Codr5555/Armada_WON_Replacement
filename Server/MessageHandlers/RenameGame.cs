using System.Text;

namespace ArmadaServer {
	internal partial class Network {
		internal void RenameGame(Span<byte> data) {
			if (Player.Game == null) {
				return;
			}

			if (Player != Player.Game.Host) {
				throw new Exception("Invalid request.");
			}

			Player.Game.Name = Encoding.Latin1.GetString(data);
		}
	}
}