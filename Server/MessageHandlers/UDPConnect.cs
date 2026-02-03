using System.Net.Sockets;

namespace ArmadaServer {
	internal partial class Network {
		internal void UDPConnect(SocketAsyncEventArgs socketData,Span<byte> data) {
			UDPEndoint = socketData.RemoteEndPoint!;
			//Special message to indicate a successful connection.
			QueueDataMessage(1,[]);
		}
	}
}