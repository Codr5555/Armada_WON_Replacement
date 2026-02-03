#include "MessageProcessor.h"
#include "InterfaceData.h"

void MessageProcessor::RemovePlayers(const char *data,int messageLength) {
	bool forCurrentGame = *data;

	const char *position = data + 1;
	if (forCurrentGame) {
		if (interfaceData->gameHostIPAddress != *std::bit_cast<int*>(position)) {
			return;
		}

		position += 4;
	}
	else {
		if (strcmp(interfaceData->GetCurrentRoom(),position)) {
			return;
		}
		position += strlen(position) + 1;
	}

	int count = *std::bit_cast<int*>(position);
	position += 4;

	std::scoped_lock lock(interfaceData->playersMutex);

	for (int index = 0;index < count;index++) {
		int IPAddress = *std::bit_cast<int*>(position);
		position += 4;

		for (auto iterator = interfaceData->players.begin();iterator != interfaceData->players.end();++iterator) {
			if (iterator->IPAddress == IPAddress) {
				interfaceData->players.erase(iterator);
				break;
			}
		}
	}
}