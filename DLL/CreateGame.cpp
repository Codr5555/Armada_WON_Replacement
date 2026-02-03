#include "MessageProcessor.h"
#include "InterfaceData.h"

using namespace Network::Won;

void MessageProcessor::CreateGame(const char *data,int length) {
	if (*data != 1) {
		interfaceData->AddGenericErrorEvent();
		return;
	}

	std::scoped_lock lock(interfaceData->playersMutex);
	Player player;
	for (auto &listPlayer: interfaceData->players) {
		if (listPlayer.IPAddress == interfaceData->communicator.IPAddress) {
			player = std::move(listPlayer);
			break;
		}
	}
	interfaceData->players.clear();
	interfaceData->players.AddPlayer(std::move(player));

	/*OutputDebugStringA(std::format("{} players upon game creation:\n",interfaceData->players.size()).c_str());
	for (auto &p: interfaceData->players) {
		OutputDebugStringA(std::format("  {}: {}\n",p.name.c_str(),p.IPAddress).c_str());
	}*/

	interfaceData->gameState = InterfaceData::GameState::GameSetup;
	interfaceData->events.push(new Event(Event::Code::GameCreateOrJoin));
}