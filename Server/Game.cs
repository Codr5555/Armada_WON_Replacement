using System.Collections.Concurrent;

namespace ArmadaServer {
	internal class Game {
		internal string Name { get; set; }
		internal Player Host { get; set; }
		internal uint Flags { get; set; }
		internal string Map { get; set; }

		internal Lock playersLock = new();
		internal ConcurrentDictionary<uint,Player> Players { get; } = [];

		internal int Ping {
			get => (int)(Flags & 0x7F);
		}
		internal int PlayerCount {
			get => (int)(Flags >> 9 & 0xF);
		}
		internal int MaximumPlayers {
			get => (int)(Flags >> 13 & 0xF);
		}
		internal bool Closed {
			get => Convert.ToBoolean(Flags >> 17 & 1);
		}
		internal bool InProgress {
			get => Convert.ToBoolean(Flags >> 18 & 1);
		}

		internal Game(string name,Player host,uint flags,string map) {
			Name = name;
			Host = host;
			Flags = flags;
			Map = map;

			Players.TryAdd(host.IPAddress,host);
		}

		internal byte[] GetPlayerData() {
			var data = new List<byte>();
			data.AddRange(BitConverter.GetBytes(Players.Count));
			foreach (var (_,player) in Players) {
				data.AddRange(player.GetBytes());
			}
			return data.ToArray();
		}

		internal void RemovePlayer(Player player) {
			if (player.Game == null) {
				return;
			}

			var removeMessage = player.Network.GetRemovePlayerMessageData(player.Room,true);

			//Notify the host that the player is leaving.
			player.Game.Host.Network.QueueDataMessage(player.IPAddress,[7]);

			lock (playersLock) {
				Players.Remove(player.IPAddress,out _);

				foreach (var (_,gamePlayer) in Players) {
					gamePlayer.Network.QueueMessage(Network.OutgoingTCPMessageID.RemovePlayers,removeMessage);
				}
			}
		}
	};
}