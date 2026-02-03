#include "MessageProcessor.h"
#include "InterfaceData.h"

void MessageProcessor::GetRooms(const char *data,int length) {
	auto list = new Network::Won::RoomList();

	int count = *std::bit_cast<short*>(data);
	const char *position = data + 2;
	for (int index = 0;index < count;index++) {
		const char *name = position;
		position += strlen(position) + 1;

		bool hasPassword = *position;
		position++;

		list->AddRoom(Network::Won::Room(name,hasPassword,*std::bit_cast<int*>(position)));
		position += 4;
	}

	interfaceData->roomList = list;
	WakeByAddressSingle(&interfaceData->roomList);
}