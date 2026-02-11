#include "MessageProcessor.h"
#include "InterfaceData.h"

void MessageProcessor::Connect(const char *data,int messageLength) {
	if (messageLength != 5) {
		throw new std::exception("Unexpected Connect message format from the server.");
	}

	interfaceData->communicator.IPAddress = *std::bit_cast<int*>(data);

	if (data[4]) {
		interfaceData->useUDP = true;
		interfaceData->communicator.EnableUDP();
		return;
	}

	interfaceData->SuccessfulConnection();
}