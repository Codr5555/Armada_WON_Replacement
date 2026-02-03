#include "MessageProcessor.h"
#include "InterfaceData.h"

using namespace Network::Won;

void MessageProcessor::LeaveGame(const char *data,int length) {
	if (length == 0) {
		interfaceData->AddGenericErrorEvent();
		return;
	}

	ProcessUpdatedPlayerData(data);

	interfaceData->gameState = InterfaceData::GameState::GameSetup;
	interfaceData->AddGenericOKEvent();
}