#include "MessageProcessor.h"
#include "Logger.h"
#include "InterfaceData.h"

using namespace Network::Won;

void MessageProcessor::Chat(const char *data,int length) {
	int sourceIPAddress = *std::bit_cast<int*>(data);

	Logger::Log(std::format("MessageProcessor::Chat called (State: {:d}) (Length: {}): {:.{}}",std::to_underlying(interfaceData->gameState.load()),length,data,length).c_str());

	Network::Won::Event *event;
	if (interfaceData->gameState == InterfaceData::GameState::InGame) {
		event = new EventGameChat(sourceIPAddress,interfaceData->GetPlayerName(sourceIPAddress),data + 4,length - 4);
	}
	else if (interfaceData->gameState == InterfaceData::GameState::GameSetup) {
		event = new EventGameSetupChat(sourceIPAddress,data + 4,length - 4);
	}
	else {
		event = new EventChat(sourceIPAddress,interfaceData->GetPlayerName(sourceIPAddress),data + 4,length - 4);
	}

	interfaceData->events.push(event);
}