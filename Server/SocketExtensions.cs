using System.Net;

namespace ArmadaServer {
	internal static class SocketExtensions {
		internal static string GetIPv4AddressString(this EndPoint endPoint) {
			var address = (endPoint as IPEndPoint)?.Address;
			return address!.ToString();
		}
		internal static uint GetIPv4Address(this EndPoint endPoint) {
			var address = (endPoint as IPEndPoint)?.Address;
			var bytes = address!.GetAddressBytes();
			return BitConverter.ToUInt32(bytes);
		}
	}
}