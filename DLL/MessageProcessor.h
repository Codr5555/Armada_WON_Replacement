#pragma once

#include "WON.h"

class MessageProcessor {
	struct Handler {
		void (MessageProcessor::*Function)(const char *data,int messageLength);
		const char *name;
	};

	Network::Won::InterfaceData *interfaceData;

#define HandlerDefinition(name) Handler{&MessageProcessor::name,#name}
	//The order of this list must match the message ID in the server code.
	const std::array<Handler,15> MessageHandlers = {
		HandlerDefinition(Connect),
		HandlerDefinition(Login),
		HandlerDefinition(JoinRoom),
		HandlerDefinition(GetGameList),
		HandlerDefinition(CreateRoom),
		HandlerDefinition(GetRooms),
		HandlerDefinition(AddPlayers),
		HandlerDefinition(Ping),
		HandlerDefinition(Chat),
		HandlerDefinition(CreateGame),
		HandlerDefinition(LeaveGame),
		HandlerDefinition(JoinGame),
		HandlerDefinition(UpdatePing),
		HandlerDefinition(RemovePlayers),
		HandlerDefinition(Data),
	};
#undef HandlerDefinition

	static const char *GetSocketType(SOCKET socket);

	bool ReadTCP(std::span<char> buffer,int length);
	bool WaitForData(SOCKET &socket);
	bool HandleInvalidResult(SOCKET &socket,int result,bool forRecv);
	Network::Won::Player ParsePlayerData(const char *&data);
	void ProcessUpdatedPlayerData(const char *data);

	//Message handlers.
	void Connect(const char *data,int messageLength);
	void Login(const char *data,int messageLength);
	void JoinRoom(const char *data,int messageLength);
	void GetGameList(const char *data,int messageLength);
	void CreateRoom(const char *data,int messageLength);
	void GetRooms(const char *data,int messageLength);
	void AddPlayers(const char *data,int messageLength);
	void Ping(const char *data,int messageLength);
	void Chat(const char *data,int messageLength);
	void CreateGame(const char *data,int length);
	void LeaveGame(const char *data,int length);
	void JoinGame(const char *data,int length);
	void Data(const char *data,int length);
	void UpdatePing(const char *data,int length);
	void RemovePlayers(const char *data,int length);

public:
	static void TCPThread(Network::Won::InterfaceData *data);
	static void UDPThread(Network::Won::InterfaceData *data);

	MessageProcessor(Network::Won::InterfaceData *data);

	void ExecuteTCP();
	void ExecuteUDP();
};