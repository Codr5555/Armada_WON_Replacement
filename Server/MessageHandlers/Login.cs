using System.Text;

namespace ArmadaServer {
	internal partial class Network {
		internal void Login(Span<byte> data) {
			var accountLength = BitConverter.ToInt32(data);
			if (accountLength <= 0) {
				throw new Exception("Invalid data.");
			}
			if (accountLength > 20) {
				accountLength = 20; //Armada-imposed limit.
			}

			var account = Encoding.Latin1.GetString(data.Slice(4,accountLength));

			var passwordLength = BitConverter.ToInt32(data.Slice(4 + accountLength));
			var password = Encoding.Latin1.GetString(data.Slice(4 + accountLength + 4,passwordLength));

			if (Server.Players.Any(e => e.Value.Network.TCPSocket.Connected && e.Value.Account.Equals(account,StringComparison.OrdinalIgnoreCase))) {
				QueueBooleanMessage(OutgoingTCPMessageID.Login,0);
				return;
			}

			Player.Account = account;

			string MOTD = "";
			try {
				MOTD = File.ReadAllText("MOTD.txt");
			}
			catch {
			}
			QueueMessage(OutgoingTCPMessageID.Login,[1],Encoding.Latin1.GetBytes(MOTD),[0]);
		}
	}
}