using System.Security.Cryptography;
using Microsoft.Data.Sqlite;
using Serilog;

namespace ArmadaServer {
	internal class Database {
		internal SqliteConnection Connection { get; private set; } = new SqliteConnection("Data Source=Database");

		public Database() {
			try {
				Connection.Open();

				using var command = Connection.CreateCommand();
				command.CommandText = "create table if not exists Accounts (Name text not null,Password blob not null,Salt blob not null,[Last Login] integer not null)";
				command.ExecuteNonQuery();
			}
			catch (Exception exception) {
				Log.Error(exception,"Attempting to establish a database connection failed.  Ensure the working directory has write access and available space.");
				return;
			}
		}

		internal byte[] GetSalt(string account) {
			using var command = Connection.CreateCommand();
			command.CommandText = "select Salt from Accounts where Name = @name";
			command.Parameters.AddWithValue("@name",account);
			using var reader = command.ExecuteReader();
			reader.Read();
			var salt = new byte[16];
			long readBytes;
			if ((readBytes = reader.GetBytes(reader.GetOrdinal("Salt"),0,salt,0,16)) != 16) {
				throw new Exception($"Invalid salt column for account {account}.  Expected 16 bytes, read {readBytes}.");
			}

			return salt;
		}

		internal bool AccountExists(string account) {
			using var command = Connection.CreateCommand();
			command.CommandText = "select 1 from Accounts where Name = @name";
			command.Parameters.AddWithValue("@name",account);
			return command.ExecuteScalar() != null;
		}

		internal void CreateAccount(string account,ReadOnlySpan<byte> passwordHash) {
			var salt = new byte[16];
			Random.Shared.NextBytes(salt);
			var newHash = HashPassword(account,passwordHash,salt);

			using var command = Connection.CreateCommand();
			command.CommandText = "insert into Accounts values (@name,@hash,@salt,@lastLogin)";
			command.Parameters.AddWithValue("@name",account);
			command.Parameters.AddWithValue("@hash",newHash);
			command.Parameters.AddWithValue("@salt",salt);
			command.Parameters.AddWithValue("@lastLogin",DateTime.UtcNow);
			command.ExecuteNonQuery();
		}

		internal bool Login(string account,ReadOnlySpan<byte> passwordHash) {
			var hash = HashPassword(account,passwordHash,GetSalt(account));

			using var command = Connection.CreateCommand();
			command.CommandText = "select 1 from Accounts where Name = @name and Password = @password";
			command.Parameters.AddWithValue("@name",account);
			command.Parameters.AddWithValue("@password",hash);
			return command.ExecuteScalar() != null;
		}

		internal void UpdateLastLogin(string account) {
			using var command = Connection.CreateCommand();
			command.CommandText = "update Accounts set [Last Login] = @lastLogin where Name = @name";
			command.Parameters.Clear();
			command.Parameters.AddWithValue("@name",account);
			command.Parameters.AddWithValue("@lastLogin",DateTime.UtcNow);
			command.ExecuteNonQuery();
		}

		internal void ChangePassword(string account,ReadOnlySpan<byte> passwordHash) {
			var hash = HashPassword(account,passwordHash,GetSalt(account));

			using var command = Connection.CreateCommand();
			command.CommandText = "update Accounts set Password = @password where Name = @name";
			command.Parameters.AddWithValue("@name",account);
			command.Parameters.AddWithValue("@password",hash);
			command.ExecuteNonQuery();
		}

		private static byte[] HashPassword(string account,ReadOnlySpan<byte> password,byte[] salt) {
			var hash = new byte[32 + 16];
			password.CopyTo(hash);
			salt.CopyTo(hash,32);
			return SHA256.HashData(hash);
		}
	}
}