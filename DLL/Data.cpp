#include "MessageProcessor.h"
#include "InterfaceData.h"

void MessageProcessor::Data(const char *data,int messageLength) {
	interfaceData->events.push(new Network::Won::EventData(*std::bit_cast<int*>(data),data + 4,messageLength - 4));
}