namespace ArmadaServer {
	internal static class OutputHelper {
		internal static string GetByteString(Span<byte> data) {
			return string.Join(' ',Array.ConvertAll(data.ToArray(),e => e.ToString("X2")));
		}
	}
}
