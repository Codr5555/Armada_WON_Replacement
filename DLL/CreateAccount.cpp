#include "MessageProcessor.h"
#include "InterfaceData.h"

using namespace Network::Won;

void MessageProcessor::CreateAccount(const char *data,int length) {
	if (length < 1) {
		throw std::exception("Unexpected data from the server.");
	}

	int result = *data;

	if (result == 2) {
		interfaceData->events.push(new EventFailure(EventFailure::Code::General));
		return;
	}
	if (result == 3) {
		interfaceData->events.push(new EventFailure(EventFailure::Code::AccountExists));
		return;
	}

	if (length < 2) {
		throw std::exception("Unexpected data from the server.");
	}

	SetMOTD(data + 1);

	interfaceData->SetPlayerName("test");

	interfaceData->events.push(new Event(Event::Code::OK));
}