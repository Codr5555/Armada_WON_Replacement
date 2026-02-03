#include "MessageProcessor.h"

Network::Won::Player MessageProcessor::ParsePlayerData(const char *&data) {
	Network::Won::Player player;

	player.name = data;
	data += player.name.length + 1;

	player.IPAddress = *std::bit_cast<int*>(data);
	data += 4;

	player.ping = *std::bit_cast<int*>(data);
	data += 4;

	player.unknownIdentifier = *std::bit_cast<int*>(data);
	data += 4;

	return player;
}