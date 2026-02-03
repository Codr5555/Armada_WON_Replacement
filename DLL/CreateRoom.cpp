#include "MessageProcessor.h"
#include "InterfaceData.h"

void MessageProcessor::CreateRoom(const char *data,int length) {
	if (!*data) {
		interfaceData->AddGenericErrorEvent();
		return;
	}

	int nameLength = strlen(data);
	interfaceData->SetCurrentRoom(data);

	ProcessUpdatedPlayerData(data + nameLength + 1);

	interfaceData->AddGenericOKEvent();
}