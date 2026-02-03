#include "MessageProcessor.h"
#include "InterfaceData.h"

void MessageProcessor::Ping(const char *data,int length) {
	interfaceData->communicator.PingResponse();
}