#pragma once

#include "WON.h"

namespace Network::Won {

class InterfaceData {
	unsigned long long actionTime = 0;
	std::list<const char *> serverList;
	std::array<char,32> currentRoom;
	std::array<char,21> playerName; //Armada limits this to 20 characters.
	char CRC;

public:
	enum class GameState : char {
		Lobby,
		GameSetup,
		InGame,
	};

	ServerCommunicator communicator;
	MultiThreadedQueue<Event*> events;
	GameList emptyGameList; //Armada can request a game list before a connection is even established.
	PlayerList emptyPlayerList; //Used in-game for reducing network traffic and game processing, as player updates aren't needed.
	std::atomic<GameList*> gameList = nullptr;
	std::atomic<RoomList*> roomList = nullptr;
	std::atomic<bool> shutdown = false;
	std::atomic<GameState> gameState = GameState::Lobby;
	std::mutex playersMutex;
	PlayerList players;
	unsigned int gameHostIPAddress = 0;
	unsigned long long lastGameListUpdate = 0;
	unsigned long long lastRoomListUpdate = 0;
	const char *MOTD = nullptr;

	InterfaceData(int CRC,const std::list<const char *> &serverList);
	~InterfaceData() {
		shutdown.store(true);
	}

	const std::list<const char *> &GetServerList() const {
		return serverList;
	}

	char GetCRC() const {
		return CRC;
	}

	const char *GetCurrentRoom() const {
		return currentRoom.data();
	}
	void SetCurrentRoom(const char *room) {
		strcpy_s(currentRoom.data(),currentRoom.size(),room);
	}

	void AddGenericOKEvent() {
		events.push(new Network::Won::Event(Event::Code::OK));
	}
	void AddGenericErrorEvent() {
		events.push(new EventFailure(EventFailure::Code::General));
	}

	void SetPlayerName(const char *name) {
		strcpy_s(playerName.data(),playerName.size(),name);
	}
	const char *GetPlayerName(int IPAddress);

	void SetActionTime() {
		actionTime = GetTickCount64();
	}
	bool CanCancel() const {
		return GetTickCount64() - actionTime >= 5000;
	}
};

}