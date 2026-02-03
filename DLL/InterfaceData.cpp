#include "InterfaceData.h"

namespace Network::Won {

//Armada's import requires that this be defined as taking an std::list by name, but the list internals have changed since Visual Studio 6.  (Armada was compiled with VS6.)
//As such, we use the modern std::list for the name, then immediately cast it to the minimal reimplementation of the VC6 list.
InterfaceData::InterfaceData(int CRC,const std::list<const char *> &serverList) : communicator(this),emptyPlayerList(this),players(this) {
	const VC6List<const char *> &list = reinterpret_cast<const VC6List<const char *>&>(serverList);

	int value1 = (CRC & 0xFF0000) >> 8;
	int value2 = (CRC & 0xFF00 ^ value1) >> 8;
	this->CRC = CRC ^ (CRC & 0xFF000000) >> 24 ^ value2;

	currentRoom[0] = 0;
	playerName[0] = 0;
}

const char *InterfaceData::GetPlayerName(int IPAddress) {
	std::scoped_lock<std::mutex> lock(playersMutex);

	for (const auto &player: players) {
		if (player.IPAddress == IPAddress) {
			return player.name.c_str();
		}
	}

	return "";
}

}