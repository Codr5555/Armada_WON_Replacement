#include "MessageProcessor.h"
#include "InterfaceData.h"

using namespace Network::Won;

void MessageProcessor::JoinGame(const char *data,int length) {
	if (length == 0) {
		interfaceData->AddGenericErrorEvent();
		return;
	}

	interfaceData->gameHostIPAddress = *std::bit_cast<int*>(data);

	ProcessUpdatedPlayerData(data + 4);

	interfaceData->gameState = InterfaceData::GameState::GameSetup;
	interfaceData->events.push(new Event(Event::Code::GameCreateOrJoin));
}