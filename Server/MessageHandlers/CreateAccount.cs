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

			var hash = data.Slice(4 + accountLength,32);

			if (accountLength > 20) {
				account = account[..20];
			}

			var command = Server.Database.CreateCommand();
			command.CommandText = "select 1 from Accounts where Name = @name";
			command.Parameters.AddWithValue("@name",account);
			if (command.ExecuteScalar() != null) {
				QueueMessage(OutgoingTCPMessageID.CreateAccount,[3]);
				return;
			}

			command.CommandText = "insert into Accounts values (@name,@hash,@lastLogin)";
			command.Parameters.AddWithValue("@hash",Convert.ToBase64String(hash));
			command.Parameters.AddWithValue("@lastLogin",DateTime.UtcNow);
			try {
				command.ExecuteNonQuery();
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