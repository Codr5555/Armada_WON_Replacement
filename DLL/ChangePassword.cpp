#include "MessageProcessor.h"
#include "InterfaceData.h"

using namespace Network::Won;

void MessageProcessor::ChangePassword(const char *data,int length) {
	if (length < 1) {
		throw std::exception("Unexpected data.");
	}

	int result = *data;

	if (result == 0) {
		interfaceData->events.push(new EventFailure(EventFailure::Code::General));
		return;
	}

	SetMOTD(data + 1);

	interfaceData->events.push(new Event(Event::Code::OK));
}