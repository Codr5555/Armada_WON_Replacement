#include "MessageProcessor.h"
#include "InterfaceData.h"

void MessageProcessor::AddPlayers(const char *data,int messageLength) {
	bool clearList = *std::bit_cast<bool*>(data);

	std::scoped_lock lock(interfaceData->playersMutex);

	if (clearList) {
		interfaceData->players.clear();
	}

	const char *position = data + 1;
	int count = *std::bit_cast<int*>(position);
	position += 4;

	for (int index = 0;index < count;index++) {
		Network::Won::Player player;

		player.name = position;
		position += player.name.length + 1;

		player.IPAddress = *std::bit_cast<int*>(position);
		position += 4;

		player.ping = *std::bit_cast<int*>(position);
		position += 4;

		player.unknownIdentifier = *std::bit_cast<int*>(position);
		position += 4;

		interfaceData->players.push_back(std::move(player));
	}
}