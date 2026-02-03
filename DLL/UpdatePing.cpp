#include "MessageProcessor.h"
#include "InterfaceData.h"

void MessageProcessor::UpdatePing(const char *data,int messageLength) {
	int count = *std::bit_cast<int*>(data);

	std::scoped_lock lock(interfaceData->playersMutex);

	const char *position = data + 4;
	for (int index = 0;index < count;index++) {
		int IPAddress = *std::bit_cast<int*>(position);
		position += 4;

		short ping = *std::bit_cast<short*>(position);
		position += 2;

		for (auto &player: interfaceData->players) {
			if (player.IPAddress == IPAddress) {
				player.ping = ping;
				break;
			}
		}
	}
}