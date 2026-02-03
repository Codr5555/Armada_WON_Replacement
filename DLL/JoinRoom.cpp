#include "MessageProcessor.h"
#include "InterfaceData.h"

void MessageProcessor::JoinRoom(const char *data,int length) {
	if (!*data) {
		interfaceData->AddGenericErrorEvent();
		return;
	}

	int nameLength = strlen(data);
	interfaceData->SetCurrentRoom(data);

	ProcessUpdatedPlayerData(data + nameLength + 1);

	//Force a game list update.
	interfaceData->lastGameListUpdate = 0;

	interfaceData->AddGenericOKEvent();
}