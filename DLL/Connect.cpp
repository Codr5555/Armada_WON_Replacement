#include "MessageProcessor.h"
#include "InterfaceData.h"

void MessageProcessor::Connect(const char *data,int messageLength) {
	interfaceData->communicator.ConnectUDP();

	interfaceData->communicator.IPAddress = *std::bit_cast<int*>(data);
}