#include "MessageProcessor.h"
#include "InterfaceData.h"

using namespace Network::Won;

void MessageProcessor::Login(const char *data,int messageLength) {
	bool successful = *data;

	if (!successful) {
		//Send failed message instead of this later.
		throw std::exception("Another player is already logged in with this name.");
	}

	int MOTDLength = strlen(data + 1);
	char *MOTD = new char[MOTDLength + 1];
	strcpy_s(MOTD,MOTDLength + 1,data + 1);
	interfaceData->MOTD = MOTD;

	interfaceData->events.push(new Event(Event::Code::Login));
}