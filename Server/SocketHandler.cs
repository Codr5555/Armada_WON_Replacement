using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using Serilog;

namespace ArmadaServer {
	internal class SocketHandler {
		private const int maximumUsers = 1000;

		private static readonly IPEndPoint anyEndpoint = new(IPAddress.Any,0);

		private readonly SemaphoreSlim connectionLimiter;
		private readonly SocketBufferPool bufferPool;
		private readonly Socket listenSocket;
		private readonly Socket UDPSocket;
		private readonly ConcurrentStack<SocketAsyncEventArgs> argumentsPool = [];

		internal IPAddress IPAddress => ((IPEndPoint)listenSocket.LocalEndPoint!).Address;
		internal int Port => ((IPEndPoint)listenSocket.LocalEndPoint!).Port;

		internal SocketHandler(string IPPort) {
			var (IPAddress,port) = ValidateArgument(IPPort);

			connectionLimiter = new(maximumUsers,maximumUsers);
			bufferPool = new(maximumUsers);
			
			SocketAsyncEventArgs arguments;
			for (int index = 0;index < maximumUsers;index++) {
				arguments = new SocketAsyncEventArgs();
				arguments.Completed += CommunicationCompleted;
				bufferPool.SetBuffer(arguments);
				argumentsPool.Push(arguments);
			}

			listenSocket = new Socket(AddressFamily.InterNetwork,SocketType.Stream,ProtocolType.Tcp);
			listenSocket.Bind(new IPEndPoint(IPAddress,port));
			listenSocket.Listen(500);

			UDPSocket = new Socket(AddressFamily.InterNetwork,SocketType.Dgram,ProtocolType.Udp);
			UDPSocket.Bind(new IPEndPoint(IPAddress,port));

			arguments = new SocketAsyncEventArgs();
			arguments.SetBuffer(new byte[768],0,768);
			arguments.Completed += UDPReceive;
			arguments.RemoteEndPoint = anyEndpoint;
			UDPSocket.ReceiveFromAsync(arguments);

			_ = Task.Run(ConnectionMonitor);
		}

		internal Task StartAsync() {
			var asyncArguments = new SocketAsyncEventArgs();
			asyncArguments.Completed += ListenSocket_Completed;
			return ProcessIncomingConnections(asyncArguments);
		}

		private void UDPReceive(object? sender,SocketAsyncEventArgs socketData) {
			while (true) {
				Network? network = null;
				var address = socketData.RemoteEndPoint!.GetIPv4Address();
				foreach (var (_,player) in Server.Players) {
					if (address == player.Network.TCPSocket.RemoteEndPoint!.GetIPv4Address()) {
						network = player.Network;
						break;
					}
				}

				if (network != null) {
					var data = new Span<byte>(socketData.Buffer,0,socketData.BytesTransferred);
					try {
						network.ProcessUDPMessage(socketData,data);
					}
					catch (Exception exception) {
						Log.Error(exception,$"An exception occurred in the UDP message processor.  It's being ignored.  (Player: {network.Player?.Account ?? "(Not yet authenticated.)"} - {new IPAddress(address)})  (Data: {OutputHelper.GetByteString(data)})");
					}
				}

				socketData.RemoteEndPoint = anyEndpoint;
				try {
					if (UDPSocket.ReceiveFromAsync(socketData)) {
						return;
					}
				}
				catch (Exception exception) {
					Log.Error(exception,$"A UDP ReceiveFrom call threw an exception.  It's being ignored.  (Player: {network?.Player?.Account ?? "(Not yet authenticated.)"} - {new IPAddress(address)})");

					continue;
				}
			}
		}

		private async Task ProcessIncomingConnections(SocketAsyncEventArgs arguments) {
			while (true) {
				await connectionLimiter.WaitAsync();

				try {
					arguments.AcceptSocket = null;
					if (listenSocket.AcceptAsync(arguments)) {
						return;
					}
				}
				catch (Exception exception) {
					Log.Error(exception,"An error occurred while accepting a new connection.");
					connectionLimiter.Release();
					
					continue;
				}

				await StartCommunication(arguments);
			}
		}

		private async void ListenSocket_Completed(object? sender,SocketAsyncEventArgs arguments) {
			Log.Information($"Accepted a connection.  IP address: {new IPAddress(arguments.AcceptSocket!.RemoteEndPoint!.GetIPv4Address())}");

			await StartCommunication(arguments);

			await ProcessIncomingConnections(arguments);
		}

		private async Task StartCommunication(SocketAsyncEventArgs arguments) {
			if (!argumentsPool.TryPop(out var communicationArguments)) {
				Log.Error("Unable to retrieve async arguments for starting communication.  This shouldn't be possible and indicates that there's a desynchronization between the connection-limiting semaphore and arguments pool.");
				return;
			}

			try {
				communicationArguments.UserToken = new Network(arguments.AcceptSocket!,UDPSocket,communicationArguments);
			}
			catch (Exception exception) {
				argumentsPool.Push(communicationArguments);
				Log.Error(exception,"Failed to create a Network instance for the new connection.");
				return;
			}

			try {
				if (!arguments.AcceptSocket!.ReceiveAsync(communicationArguments)) {
					await ProcessReceiveCompletion(communicationArguments);
				}
			}
			catch (Exception exception) {
				var network = communicationArguments.UserToken as Network;
				CloseConnection(communicationArguments);
				Log.Error(exception,$"An error occurred during communication and the connection was terminated.  Account: {network?.Player?.Account ?? "(Not yet authenticated.)"}");
			}
		}

		private async void CommunicationCompleted(object? sender,SocketAsyncEventArgs arguments) {
			switch (arguments.LastOperation) {
				case SocketAsyncOperation.Receive:
					try {
						await ProcessReceiveCompletion(arguments);
					}
					catch (Exception exception) {
						var network = arguments.UserToken as Network;
						CloseConnection(arguments);
						Log.Error(exception,$"An error occurred during communication and the connection was terminated.  Account: {network?.Player?.Account ?? "(Not yet authenticated.)"}");
					}
					break;
				default:
					Log.Warning($"Unexpected last operation ({arguments.LastOperation}) on a socket.  It's being closed.");
					CloseConnection(arguments);
					break;
			}
		}

		internal Task ProcessReceiveCompletion(SocketAsyncEventArgs arguments) {
			if (arguments.BytesTransferred == 0 || arguments.SocketError != SocketError.Success) {
				if (arguments.SocketError != SocketError.Success && arguments.SocketError != SocketError.ConnectionReset) {
					Log.Warning($"A socket read event resulted in a non-successful status: {arguments.SocketError}");
				}
				CloseConnection(arguments);
				return Task.CompletedTask;
			}

			var network = (Network)arguments.UserToken!;
			network.RemainingReadLength += arguments.BytesTransferred;
			return network.ProcessTCPMessages(arguments);
		}

		internal void CloseConnection(SocketAsyncEventArgs arguments) {
			var network = arguments.UserToken! as Network;

			Log.Information($"Client disconnected: {network?.Player?.Account ?? "(Not yet authenticated.)"} - {GetIPAddress(arguments)}");

			if (network != null) {
				try {
					if (arguments.SocketError != SocketError.OperationAborted) {
						network.TCPSocket.Shutdown(SocketShutdown.Send);

						network.Close();
					}
				}
				catch (Exception exception) {
					Log.Error(exception,"An exception occurred during connection closure.  It was ignored.");
				}
			}

			try {
				//If the network isn't set, the buffer should never have been changed.
				if (network != null) {
					arguments.SetBuffer(SocketBufferPool.SourceBuffer,network.BaseBufferOffset,SocketBufferPool.bufferSize);
				}
				arguments.UserToken = null;
				argumentsPool.Push(arguments);
			}
			catch (Exception exception) {
				Log.Error(exception,"Trying to restore arguments to the pool failed.  This will reduce the total number of possible connections over time.");
			}

			try {
				connectionLimiter.Release();
			}
			catch (Exception exception) {
				Log.Error(exception,"Releasing the connection-limiting semaphore failed.  This shouldn't happen and means that redundant releases have occurred.");
			}
		}

		internal static async Task ConnectionMonitor() {
			while (true) {
				foreach (var (_,player) in Server.Players) {
					player.Network.QueuePingRequest();
				}

				await Task.Delay(1000);
			}
		}

		private static (IPAddress IPAddress,ushort Port) ValidateArgument(string argument) {
			if (argument.Length == 0) {
				throw new ArgumentException("An IP address and port argument is required.");
			}

			var parts = argument.Split(':');
			if (parts.Length != 2) {
				throw new ArgumentException("Invalid IP address and port argument.");
			}

			if (!IPAddress.TryParse(parts[0],out var address)) {
				throw new ArgumentException("The IP address is invalid.");
			}
			if (!ushort.TryParse(parts[1],out var port)) {
				throw new ArgumentException("The port is invalid.");
			}
			return (address,port);
		}

		private static string GetIPAddress(SocketAsyncEventArgs arguments) {
			var network = arguments.UserToken as Network;

			uint? IPAddress = null;
			try {
				IPAddress = network!.TCPSocket.RemoteEndPoint?.GetIPv4Address();
			}
			catch {
			}

			string logAddress = "";
			if (IPAddress == null) {
				IPAddress = network!.UDPEndoint?.GetIPv4Address();
				if (IPAddress == null) {
					IPAddress = arguments.RemoteEndPoint?.GetIPv4Address();
					if (IPAddress == null) {
						logAddress = "(IP address is not available.)";
					}
				}
			}
			if (IPAddress != null) {
				logAddress = new IPAddress(IPAddress.Value).ToString();
			}

			return logAddress;
		}
	}
}