#include "MessageProcessor.h"
#include "InterfaceData.h"

using namespace Network::Won;

void MessageProcessor::Login(const char *data,int messageLength) {
	if (messageLength < 1) {
		throw std::exception("Unexpected data.");
	}

	int result = *data;

	if (result == 0) {
		interfaceData->events.push(new EventFailure(EventFailure::Code::General));
		throw std::exception("Another player is already logged in with this name.");
	}
	if (result == 2) {
		interfaceData->events.push(new EventFailure(EventFailure::Code::AccountDoesNotExist));
		return;
	}
	if (result == 3) {
		interfaceData->events.push(new EventFailure(EventFailure::Code::InvalidPassword));
		return;
	}

	SetMOTD(data + 1);

	interfaceData->events.push(new Event(Event::Code::Login));
}