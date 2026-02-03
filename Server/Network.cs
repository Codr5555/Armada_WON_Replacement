using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Text;
using Serilog;

namespace ArmadaServer {
	internal partial class Network {
		internal enum OutgoingTCPMessageID : byte {
			Connect,
			Login,
			RoomJoin,
			GetGameList,
			RoomCreate,
			GetRooms,
			AddPlayers,
			Ping,
			Chat,
			CreateGame,
			LeaveGame,
			JoinGame,
			UpdatePing,
			RemovePlayers,
			Data,
		};

		private delegate void TCPMessageHandler(Span<byte> data);
		private delegate void UDPMessageHandler(SocketAsyncEventArgs socketData,Span<byte> data);

		private static readonly byte[] authenticationToken = [0xcf,0x83,0xe1,0x35,0x7e,0xef,0xb8,0xbd,0xf1,0x54,0x28,0x50,0xd6,0x6d,0x80,0x07,0xd6,0x20,0xe4,0x05,0x0b,0x57,0x15,0xdc,0x83,0xf4,0xa9,0x21,0xd3,0x6c,0xe9,0xce,0x47,0xd0,0xd1,0x3c,0x5d,0x85,0xf2,0xb0,0xff,0x83,0x18,0xd2,0x87,0x7e,0xec,0x2f,0x63,0xb9,0x31,0xbd,0x47,0x41,0x7a,0x81,0xa5,0x38,0x32,0x7a,0xf9,0x27,0xda,0x3e];

		private readonly CancellationTokenSource terminateToken = new();
		private readonly TCPMessageHandler[] TCPHandlers;
		private readonly UDPMessageHandler[] UDPHandlers;
		private DateTime lastPing = DateTime.MinValue;
		private DateTime lastPingResponse = DateTime.MinValue;

		private BlockingCollection<byte[]> OutgoingTCPMessages { get; } = [];
		private BlockingCollection<byte[]> OutgoingUDPMessages { get; } = [];
		private readonly ConcurrentStack<SocketAsyncEventArgs> argumentsPool = [];

		internal const int messageHeaderLength = 5;

		internal Socket TCPSocket { get; }
		internal Socket UDPSocket { get; }
		internal EndPoint? UDPEndoint { get; set; } = null;
		internal int BaseBufferOffset { get; }

		/// <summary>
		/// This is the current READ position, which may differ from <see cref="SocketAsyncEventArgs.Offset"/>.
		/// </summary>
		internal int CurrentBufferOffset { get; private set; }

		/// <summary>
		/// The number of bytes that were previously read from the socket, but not yet processed.
		/// </summary>
		internal int RemainingReadLength { get; set; } = 0;

		internal Player Player { get; private set; } = null!;

		internal Network(Socket TCPSocket,Socket UDPSocket,SocketAsyncEventArgs arguments) {
			this.TCPSocket = TCPSocket;
			this.UDPSocket = UDPSocket;

			BaseBufferOffset = CurrentBufferOffset = arguments.Offset;

			TCPHandlers = [
				Login,
				RoomJoin,
				GetGameList,
				CreateRoom,
				GetRooms,
				Chat,
				PrivateChat,
				GroupChat,
				CreateGame,
				LeaveGame,
				JoinGame,
				RenameGame,
				GameSummaryUpdate,
				TCPData,
				TCPDataTargetted,
			];

			UDPHandlers = [
				UDPConnect,
				UDPData,
				UDPDataTargetted,
			];

			_ = Task.Run(OutgoingTCPMessageProcessor);
			_ = Task.Run(OutgoingUDPMessageProcessor);
		}

		internal static byte[] CreateSendBufferHeader(OutgoingTCPMessageID messageID,int dataLength) {
			var buffer = new byte[messageHeaderLength];
			buffer[0] = (byte)messageID;
			BitConverter.GetBytes(dataLength).CopyTo(buffer.AsSpan(1));
			return buffer;
		}
		internal static byte[] CreateBuffer(OutgoingTCPMessageID messageID,int dataLength) {
			var buffer = new byte[messageHeaderLength + dataLength];
			WriteSendBufferHeader(buffer,messageID,dataLength);
			return buffer;
		}
		internal static void WriteSendBufferHeader(Span<byte> buffer,OutgoingTCPMessageID messageID,int dataLength) {
			buffer[0] = (byte)messageID;
			BitConverter.GetBytes(dataLength).CopyTo(buffer[1..]);
		}

		internal static byte[] CreateAddPlayerMessageData(bool clearExistingList,params Player[] players) {
			var list = new List<byte>();
			list.Add(BitConverter.GetBytes(clearExistingList)[0]);
			list.AddRange(BitConverter.GetBytes(players.Length));
			foreach (var player in players) {
				list.AddRange(player.GetBytes());
			}
			return list.ToArray();
		}

		internal void QueueMessage(byte[] data) {
			OutgoingTCPMessages.Add(data);
		}
		internal void QueueBooleanMessage(OutgoingTCPMessageID messageID,byte value) {
			OutgoingTCPMessages.Add([(byte)messageID,1,0,0,0,value]);
		}
		internal void QueueMessage(OutgoingTCPMessageID messageID) {
			OutgoingTCPMessages.Add([(byte)messageID,0,0,0,0]);
		}
		internal void QueueMessage(OutgoingTCPMessageID messageID,Span<byte> bytes) {
			var buffer = new byte[5 + bytes.Length];
			buffer[0] = (byte)messageID;
			BitConverter.GetBytes(bytes.Length).CopyTo(buffer.AsSpan(1));
			bytes.CopyTo(buffer.AsSpan(5));
			OutgoingTCPMessages.Add(buffer);
		}
		internal void QueueMessage(OutgoingTCPMessageID messageID,params byte[][] byteLists) {
			var length = byteLists.Sum(e => e.Length);

			var buffer = new byte[5 + length];
			buffer[0] = (byte)messageID;
			BitConverter.GetBytes(length).CopyTo(buffer.AsSpan(1));
			int position = 5;
			foreach (var list in byteLists) {
				list.CopyTo(buffer,position);
				position += list.Length;
			}
			OutgoingTCPMessages.Add(buffer);
		}
		internal void QueueMessage(OutgoingTCPMessageID messageID,params Memory<byte>[] byteLists) {
			var length = byteLists.Sum(e => e.Length);

			var buffer = new byte[5 + length];
			buffer[0] = (byte)messageID;
			BitConverter.GetBytes(length).CopyTo(buffer.AsSpan(1));
			int position = 5;
			foreach (var list in byteLists) {
				list.Span.CopyTo(buffer.AsSpan(position));
				position += list.Length;
			}
			OutgoingTCPMessages.Add(buffer);
		}

		internal void QueueDataMessage(uint sourceIPAddress,Span<byte> data) {
			var buffer = new byte[4 + data.Length];
			BitConverter.GetBytes(sourceIPAddress).CopyTo(buffer);
			data.CopyTo(new Span<byte>(buffer,4,data.Length));
			OutgoingUDPMessages.Add(buffer);
		}

		internal void QueuePingRequest() {
			if (lastPingResponse > DateTime.MinValue && (DateTime.Now - lastPingResponse).TotalSeconds >= 45) {
#if !DEBUG
				Close();
				return;
#endif
			}

			if ((DateTime.Now - lastPing).TotalSeconds >= 7.5) {
				QueueMessage(OutgoingTCPMessageID.Ping);
				lastPing = DateTime.Now;
			}
		}

		internal void Close() {
			Console.WriteLine($"Closing the connection for player: {Player.Account}.");

			terminateToken.Cancel();
			Player.Disconnect();
			TCPSocket.Close();
		}

		internal async Task ProcessTCPMessages(SocketAsyncEventArgs arguments) {
			while (!terminateToken.IsCancellationRequested) {
				if (EnsureSufficientData(arguments,5)) {
					return;
				}

				var data = new Span<byte>(SocketBufferPool.SourceBuffer,CurrentBufferOffset,RemainingReadLength);
				if (data.Length == 0) {
					continue;
				}

				byte messageID = data[0];
				int messageLength = BitConverter.ToInt32(data.Slice(1));

				//Invalid message.
				if (messageID != 255 && messageID > TCPHandlers.Length) {
					Log.Warning($"Invalid message ID received from {Player.Account} ({Player.IPAddress}).  Closing the connection.");
					Close();
					return;
				}

				if (messageID == 0) {
					if (EnsureSufficientData(arguments,65)) {
						return;
					}

					Console.WriteLine($"Connect request from {TCPSocket.RemoteEndPoint!.GetIPv4Address()}.");

					data = new Span<byte>(SocketBufferPool.SourceBuffer,CurrentBufferOffset,RemainingReadLength);
					Connect(data.Slice(1,64));
					RemainingReadLength -= 65;
					CurrentBufferOffset += 65;

					continue;
				}

				if (Player == null) {
					throw new Exception("Invalid message prior to execution of Connect.");
				}

				if (messageID == 255) {
					//When running locally, a value of 0 can be calculated.  Armada logic doesn't support this.
					Player.Ping = Math.Max(1,(int)(DateTime.Now - lastPing).TotalMilliseconds);
					lastPingResponse = DateTime.Now;

					CurrentBufferOffset += messageHeaderLength;
					RemainingReadLength -= messageHeaderLength;

					continue;
				}

				if (EnsureSufficientData(arguments,5 + messageLength)) {
					return;
				}

				data = new Span<byte>(SocketBufferPool.SourceBuffer,CurrentBufferOffset + 5,messageLength);

				Console.WriteLine($"Executing TCP handler: {TCPHandlers[messageID - 1].Method.Name} - Player: {Player.Account}");
				try {
					TCPHandlers[messageID - 1](data);
				}
				catch (Exception exception) {
					Log.Error(exception,$"An exception occurred in the TCP handler \"{TCPHandlers[messageID - 1].Method.Name}\".");
					continue;
				}
				finally {
					RemainingReadLength -= messageHeaderLength + messageLength;
					CurrentBufferOffset += messageHeaderLength + messageLength;
				}
			}
		}

		internal void ProcessUDPMessage(SocketAsyncEventArgs socketData,Span<byte> data) {
			byte messageID = data[0];
			if (messageID >= UDPHandlers.Length) {
				return;
			}

			Console.WriteLine($"Executing UDP handler: {UDPHandlers[messageID].Method.Name} - Player: {Player.Account}");
			UDPHandlers[messageID](socketData,data[1..]);
		}

		/// <summary>
		/// Returns false if the data was read synchronously, identically to <see cref="Socket.ReceiveAsync(SocketAsyncEventArgs)">.
		/// </summary>
		private bool EnsureSufficientData(SocketAsyncEventArgs arguments,int lengthNeeded) {
			if (lengthNeeded <= RemainingReadLength) {
				return false;
			}

			var buffer = arguments.Buffer!;

			//At the end of the buffer.  Copy the remaining bytes to the base and wait for writes after it.
			if (CurrentBufferOffset + lengthNeeded >= BaseBufferOffset + SocketBufferPool.bufferSize) {
				Array.Copy(buffer,CurrentBufferOffset,buffer,BaseBufferOffset,RemainingReadLength);
				arguments.SetBuffer(SocketBufferPool.SourceBuffer,BaseBufferOffset + RemainingReadLength,SocketBufferPool.bufferSize - RemainingReadLength);
				CurrentBufferOffset = BaseBufferOffset;
			}
			else if (RemainingReadLength == 0) {
				ResetReadBuffer(arguments);
			}
			else {
				int offset = CurrentBufferOffset + RemainingReadLength;
				arguments.SetBuffer(SocketBufferPool.SourceBuffer,offset,SocketBufferPool.bufferSize - (offset - BaseBufferOffset));
			}

			var asynchronous = TCPSocket.ReceiveAsync(arguments);
			if (!asynchronous) {
				RemainingReadLength += arguments.BytesTransferred;
			}
			return asynchronous;
		}

		internal void ResetReadBuffer(SocketAsyncEventArgs arguments) {
			arguments.SetBuffer(SocketBufferPool.SourceBuffer,BaseBufferOffset,SocketBufferPool.bufferSize);
			CurrentBufferOffset = BaseBufferOffset;
			RemainingReadLength = 0;
		}

		internal byte[] GetRemovePlayerMessageData(Room room,bool forGame) {
			var messageList = new List<byte>();
			messageList.Add(BitConverter.GetBytes(forGame)[0]);
			if (forGame) {
				messageList.AddRange(BitConverter.GetBytes(Player.Game!.Host.IPAddress));
			}
			else {
				messageList.AddRange(Encoding.Latin1.GetBytes(room.Name));
				messageList.Add(0);
			}
			messageList.AddRange(BitConverter.GetBytes(1));
			messageList.AddRange(BitConverter.GetBytes(Player.IPAddress));
			return messageList.ToArray();
		}

		private async Task OutgoingTCPMessageProcessor() {
			while (!terminateToken.IsCancellationRequested) {
				byte[] data;
				try {
					data = OutgoingTCPMessages.Take(terminateToken.Token);
				}
				catch (OperationCanceledException) {
					return;
				}
				catch (Exception exception) {
					Log.Error(exception,"An error occurred when attempting to access the next TCP message.");
					Close();
					return;
				}

				try {
					Console.WriteLine($"Sending TCP message ({(OutgoingTCPMessageID)data[0]}, {data.Length - 5} bytes) to {Player.Account} ({new IPAddress(Player.IPAddress)}): {OutputHelper.GetByteString(data.AsSpan(1))}");
					await TCPSocket.SendAsync(data);
				}
				catch (SocketException exception) {
					Log.Error(exception,"A TCP socket error occurred.  The player is being disconnected.");

					if (!TCPSocket.Connected) {
						Close();
					}
					return;
				}
			}
		}

		private async Task OutgoingUDPMessageProcessor() {
			while (!terminateToken.IsCancellationRequested) {
				byte[] data;
				try {
					data = OutgoingUDPMessages.Take(terminateToken.Token);
				}
				catch (OperationCanceledException) {
					return;
				}
				catch (Exception exception) {
					Log.Error(exception,"An error occurred when attempting to access the next UDP message.");
					Close();
					return;
				}

				SocketAsyncEventArgs? arguments = null;
				try {
					Console.WriteLine($"Sending UDP message (Data, {data.Length} bytes) to {Player.Account} ({new IPAddress(Player.IPAddress)}): {OutputHelper.GetByteString(data)}");

					if (!argumentsPool.TryPop(out arguments)) {
						arguments = new SocketAsyncEventArgs();
						arguments.Completed += UDPSendCompleted;
						arguments.RemoteEndPoint = UDPEndoint;
					}
					arguments.SetBuffer(data,0,data.Length);
					if (!UDPSocket.SendToAsync(arguments)) {
						argumentsPool.Push(arguments);
					}
				}
				catch (SocketException exception) {
					if (arguments != null) {
						argumentsPool.Push(arguments);
					}
					Log.Error(exception,"A UDP socket error occurred.  A delay is being imposed before the next send attempt.");
					await Task.Delay(2500);
				}
			}
		}

		private void UDPSendCompleted(object? sender,SocketAsyncEventArgs e) {
			argumentsPool.Push(e);
		}
	}
}