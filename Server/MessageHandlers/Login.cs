using System.Text;

namespace ArmadaServer {
	internal partial class Network {
		internal void Login(Span<byte> data) {
			var accountLength = BitConverter.ToInt32(data);
			if (accountLength <= 0) {
				throw new Exception("Invalid data.");
			}

			if (data.Length != 4 + accountLength + 32) {
				throw new Exception("Invalid data.");
			}

			//Armada limits accounts to 20 characters.
			var account = Encoding.Latin1.GetString(data.Slice(4,Math.Min(20,accountLength)));

			var passwordHash = data.Slice(4 + accountLength,32);

			var command = Server.Database.CreateCommand();
			command.CommandText = "select 1 from Accounts where Name = @name";
			command.Parameters.AddWithValue("@name",account);
			if (command.ExecuteScalar() == null) {
				QueueBooleanMessage(OutgoingTCPMessageID.Login,2);
				return;
			}

			command.CommandText = "select 1 from Accounts where Name = @name and Password = @password";
			command.Parameters.AddWithValue("@password",Convert.ToBase64String(passwordHash));
			if (command.ExecuteScalar() == null) {
				QueueBooleanMessage(OutgoingTCPMessageID.Login,3);
				return;
			}

			if (Server.Players.Any(e => e.Value.Network.TCPSocket.Connected && e.Value.Account.Equals(account,StringComparison.OrdinalIgnoreCase))) {
				QueueBooleanMessage(OutgoingTCPMessageID.Login,0);
				return;
			}

			command.CommandText = "update Accounts set [Last Login] = @lastLogin where Name = @name";
			command.Parameters.Clear();
			command.Parameters.AddWithValue("@name",account);
			command.Parameters.AddWithValue("@lastLogin",DateTime.UtcNow);
			command.ExecuteNonQuery();

			Player.Account = account;

			QueueMessage(OutgoingTCPMessageID.Login,[1],Encoding.Latin1.GetBytes(Server.GetMOTD()),[1]);
		}
	}
}