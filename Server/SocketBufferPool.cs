using System.Net.Sockets;

namespace ArmadaServer {
	internal class SocketBufferPool {
		internal const int bufferSize = 2048;

		internal static byte[] SourceBuffer  { get; private set; } = [];

		private readonly Stack<int> availableIndices = [];

		internal SocketBufferPool(int maximumClients) {
			SourceBuffer = new byte[maximumClients * bufferSize];

			for (int index = 0;index < maximumClients;index++) {
				availableIndices.Push(index);
			}
		}

		internal void SetBuffer(SocketAsyncEventArgs arguments) {
			arguments.SetBuffer(SourceBuffer,availableIndices.Pop() * bufferSize,bufferSize);
		}

		internal void ReleaseBuffer(SocketAsyncEventArgs arguments) {
			availableIndices.Push(arguments.Offset / bufferSize);
		}
	}
}