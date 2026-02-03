#include "MessageProcessor.h"
#include "InterfaceData.h"

void MessageProcessor::GetGameList(const char *data,int length) {
	auto gameList = new Network::Won::GameList();

	int gameCount = *std::bit_cast<short*>(data);
	const char *position = data + 2;
	for (int gameIndex = 0;gameIndex < gameCount;gameIndex++) {
		Network::Won::Game game;

		game.name = position;
		position += game.name.length + 1;

		game.IPAddress = position;
		position += game.IPAddress.length + 1;

		game.summaryData.flags = *std::bit_cast<int*>(position) | static_cast<int>(interfaceData->GetCRC()) << 24;
		position += 4;

		strcpy_s(game.summaryData.mapName,12,position);
		position += strlen(game.summaryData.mapName) + 1;

		gameList->AddGame(std::move(game));
	}

	interfaceData->gameList.store(gameList);
	WakeByAddressSingle(&interfaceData->gameList);
}