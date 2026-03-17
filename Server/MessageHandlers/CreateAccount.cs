using System.Text;
using Serilog;

namespace ArmadaServer {
	internal partial class Network {
		internal void CreateAccount(Span<byte> data) {
			var accountLength = BitConverter.ToInt32(data);
			if (accountLength <= 0) {
				throw new Exception("Invalid data.");
			}

			var account = Encoding.Latin1.GetString(data.Slice(4,accountLength));
			if (accountLength > 20) {
				account = account[..20];
			}

			if (Server.Database.AccountExists(account)) {
				QueueMessage(OutgoingTCPMessageID.CreateAccount,[3]);
				return;
			}

			try {
				Server.Database.CreateAccount(account,data.Slice(4 + accountLength,32));
			}
			catch (Exception exception) {
				Log.Error(exception,$"Account creation failed.  Name: {account}");
				QueueMessage(OutgoingTCPMessageID.CreateAccount,[2]);
			}

			Player.Account = account;

			QueueMessage(OutgoingTCPMessageID.CreateAccount,[1],Encoding.Latin1.GetBytes(Server.GetMOTD()),[0]);
		}
	}
}