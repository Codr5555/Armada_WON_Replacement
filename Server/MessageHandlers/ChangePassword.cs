using System.Net;
using System.Text;
using Serilog;

namespace ArmadaServer {
	internal partial class Network {
		internal void ChangePassword(Span<byte> data) {
			if (data.Length != 32) {
				throw new Exception("Invalid data.");
			}

			if (Player.Account == null) {
				Log.Warning($"A password change attempt was made without being logged in.  Source: {new IPAddress(Player.IPAddress)}");
				Player.Disconnect();
				return;
			}

			try {
				Server.Database.ChangePassword(Player.Account,data);
			}
			catch (Exception exception) {
				Log.Error(exception,$"A password change failed.  Name: {Player.Account}");
				QueueMessage(OutgoingTCPMessageID.ChangePassword,[0]);
			}

			QueueMessage(OutgoingTCPMessageID.ChangePassword,[1],Encoding.Latin1.GetBytes(Server.GetMOTD()),[0]);
		}
	}
}