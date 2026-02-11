#pragma once

#include "GameSummaryData.h"

class HostnameResolver {
	addrinfo *result;

public:
	HostnameResolver(const std::string &hostName);
	~HostnameResolver() {
		freeaddrinfo(result);
	}

	const sockaddr *GetAddress() const {
		return result->ai_addr;
	}
	const int GetAddressLength() const {
		return static_cast<int>(result->ai_addrlen);
	}
};

namespace Network::Won {
	class InterfaceData;
}

class ServerCommunicator {
	//The order of this enum must correspond to the order of the handlers array in the server's code, though Connect is not included.
	enum class TCPMessageID {
		Connect,
		Login,
		JoinRoom,
		GetGameList,
		CreateRoom,
		GetRooms,
		Chat,
		PrivateChat,
		GroupChat,
		CreateGame,
		LeaveGame,
		JoinGame,
		RenameGame,
		GameSummaryUpdate,
		Data,
		DataTargetted,

		//Special case that's obviously excluded from ordering.
		PingResponse = 255,
	};

	enum class UDPMessageID {
		Connect,
		Data,
		DataTargetted,
	};

	static constexpr int socketTimeout = 30000;

	const unsigned char token[64] = {0xcf,0x83,0xe1,0x35,0x7e,0xef,0xb8,0xbd,0xf1,0x54,0x28,0x50,0xd6,0x6d,0x80,0x07,0xd6,0x20,0xe4,0x05,0x0b,0x57,0x15,0xdc,0x83,0xf4,0xa9,0x21,0xd3,0x6c,0xe9,0xce,0x47,0xd0,0xd1,0x3c,0x5d,0x85,0xf2,0xb0,0xff,0x83,0x18,0xd2,0x87,0x7e,0xec,0x2f,0x63,0xb9,0x31,0xbd,0x47,0x41,0x7a,0x81,0xa5,0x38,0x32,0x7a,0xf9,0x27,0xda,0x3e};

	Network::Won::InterfaceData *interfaceData;

	void RoomJoinCreate(TCPMessageID ID,const char *name,const char *password) const;

public:
	SOCKET TCPSocket;
	SOCKET UDPSocket;
	sockaddr_in serverAddress;
	std::thread TCPMessageProcessor;
	std::thread UDPMessageProcessor;
	int IPAddress;

	ServerCommunicator(Network::Won::InterfaceData *interfaceData);
	~ServerCommunicator();

	void Connect();
	void Login(const char *accountName,const char *password) const;
	void GetGameList() const;
	void JoinRoom(const char *name,const char *password) const;
	void CreateRoom(const char *name,const char *password) const;
	void GetRooms() const;
	void GetPlayers() const;
	void PingResponse() const;
	void Chat(const char *text) const;
	void PrivateChat(const char *text,int recipientIPAddress) const;
	void GroupChat(const char *text,std::span<const int> targets) const;
	void CreateGame(const char *name,const char *password,const Network::Won::GameSummaryData &data) const;
	void LeaveGame() const;
	void JoinGame(const char *IPAddress,const char *password) const;
	void Data(const void *data,int length) const;
	void DataTargetted(int targetIPAddress,const void *data,int length) const;
	void RenameGame(const char *name);
	void GameSummaryUpdate(const Network::Won::GameSummaryData &data);

	void TCPSend(TCPMessageID ID) const;
	void TCPSend(TCPMessageID ID,std::span<const char> buffer) const;

	void EnableUDP();
	void UDPSend(UDPMessageID ID) const;
	void UDPSend(UDPMessageID ID,std::span<const char> buffer) const;
};