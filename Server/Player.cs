using System.Text;

namespace ArmadaServer {
	internal class Player {
		internal Network Network { get; }
		internal uint IPAddress { get; }
		internal DateTime LastMessage { get; set; }
		internal string Account { get; set; } = "";
		internal Game? Game { get; set; } = null;

		private Room room = null!;
		internal Room Room {
			get => room;
			set => Interlocked.Exchange(ref room,value);
		}

		private int ping;
		internal int Ping {
			get => ping;
			set => Interlocked.Exchange(ref ping,value);
		}

		internal Timer? PendingActionCancel { get; set; }

		internal Player(Network network) {
			Network = network;
			IPAddress = network.TCPSocket.RemoteEndPoint!.GetIPv4Address();
			LastMessage = DateTime.Now;
		}

		internal void Disconnect() {
			if (Game != null) {
				if (Game.Host.IPAddress == IPAddress) {
					//Communicate to all players in the game that the host disconnected.  This sends them back to the lobby.
					lock (Game.playersLock) {
						foreach (var (_,player) in Game.Players) {
							player.Network.QueueDataMessage(IPAddress,[7]);
						}
					}
					Server.Games.TryRemove(Game.Host.IPAddress,out _);
				}
				else {
					Game.RemovePlayer(this);
				}
			}

			if (Room != null) {
				Room.SendRemovePlayerMessages(this);

				lock (Server.playersRoomLock) {
					if (Room.Name != "Lobby" && Room.PlayerCount == 1) {
						Server.Rooms.Remove(Room.Name,out _);
					}
				}
			}

			Server.Players.Remove(IPAddress,out _);
		}

		internal byte[] GetBytes() {
			List<byte> data = [];
			data.AddRange(Encoding.Latin1.GetBytes(Account));
			data.Add(0);
			data.AddRange(BitConverter.GetBytes(IPAddress));
			data.AddRange(BitConverter.GetBytes(Ping));
			data.AddRange(BitConverter.GetBytes(IPAddress));

			return data.ToArray();
		}
	}
}