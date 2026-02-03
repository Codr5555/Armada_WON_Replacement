#pragma once

#include "VC6String.h"
#include "VC6List.h"
#include "MultiThreadedQueue.h"
#include "ServerCommunicator.h"
#include "GameSummaryData.h"
#include "Logger.h"

namespace Network {
	namespace Won {
		class InterfaceData;

		class Event {
		public:
			enum class Code : int {
				Error = 0,
				Connect = 2,
				Login = 7,
				OK = 10,
				GameCreateOrJoin = 0x12,
				LobbyChat = 0x14,
				Data = 0x18,
				PlayerMessage = 0x1A,
			};

			Code messageCode;

			Event(Code code) {
				messageCode = code;
			}
			virtual ~Event() {
			}

			__declspec(dllexport) int Delete();
		};

		class EventData : public Event {
		protected:
			int IPAddress;
			const char *data;
			int length;

			EventData(Event::Code code,int IPAddress) : Event(code) {
				this->IPAddress = IPAddress;
				data = nullptr;
				length = 0;
			}

		public:
			EventData(Event::Code code,int IPAddress,const char *data,int length) : Event(code) {
				auto buffer = new char[length];
				memcpy_s(buffer,length,data,length);
				this->data = buffer;
				this->length = length;
				this->IPAddress = IPAddress;
			}
			EventData(int IPAddress,const char *data,int length) : EventData(Event::Code::Data,IPAddress,data,length) {
			}
			~EventData() {
				delete[] data;
			}
		};

		//This must use this type name, as Armada dynamic_casts it.
		class EventChat : public Event {
			int sourceIPAddress;
			std::unique_ptr<char[]> playerName;
			bool privateMessage; //This is not implemented properly by Armada and can be ignored.

		public:
			std::unique_ptr<const char[]> message;

			EventChat(int sourceIPAddress,const char *playerName,const char *text,int length) : Event(Event::Code::LobbyChat) {
				this->sourceIPAddress = sourceIPAddress;

				int playerNameLength = strlen(playerName) + 1;
				this->playerName = std::make_unique<char[]>(playerNameLength);
				strcpy_s(this->playerName.get(),playerNameLength,playerName);

				int textLength = length + 1;
				auto buffer = new char[textLength];
				strncpy_s(buffer,textLength,text,length);
				buffer[length] = 0;
				message.reset(buffer);
			}
		};

		class EventGameSetupChat : public EventData {
		public:
			EventGameSetupChat(int IPAddress,const char *message,int messageLength) : EventData(Event::Code::PlayerMessage,IPAddress) {
				int bufferMessageLength = messageLength + 1;
				length = 3 + bufferMessageLength;
				auto data = new char[length];
				this->data = data;
				*data = 5;
				data[1] = 2;
				data[2] = messageLength;
				strncpy_s(data + 3,bufferMessageLength,message,messageLength);
			}
		};

		class EventGameChat : public EventData {
		public:
			EventGameChat(int IPAddress,const char *playerName,const char *message,int messageLength) : EventData(Event::Code::PlayerMessage,IPAddress) {
				Logger::Log(std::format("EventGameChat constructor called.  Player({})",playerName,messageLength).c_str());

				int playerNameLength = strlen(playerName) + 1;

				int bufferMessageLength = messageLength + 1;
				length = 3 + bufferMessageLength + playerNameLength;
				auto data = new char[length];
				this->data = data;
				*data = 5;
				data[1] = 4;
				data[2] = bufferMessageLength;
				strncpy_s(data + 3,bufferMessageLength,message,messageLength);
				strcpy_s(data + 3 + bufferMessageLength,playerNameLength,playerName);

				Logger::Log(std::format("EventGameChat created.  Length: {}",length).c_str());
			}
		};

		class EventFailure : public Event {
		public:
			enum Code {
				General = 0,
				UnknownUser = 4,
				BadPassword = 5,
				UnableToLogIn = 18, //Applies to other message IDs as well, but this works.
			};

		private:
			Code errorCode;

		public:
			EventFailure(Code errorCode) : Event(Event::Code::Error) {
				this->errorCode = errorCode;
			}

			__declspec(dllexport) int GetAdditionalCode() const;

			__declspec(dllexport) Code GetCode() const;
		};

		class Game {
		public:
			VC6String name,IPAddress;
			GameSummaryData summaryData;

			Game() {
				name = "";
				IPAddress = "";
				summaryData.flags = 0;
				*summaryData.mapName = 0;
			}
			Game(const char *name,const char *description,const GameSummaryData &summaryData) {
				this->name = name;
				this->IPAddress = description;
				this->summaryData = summaryData;
			}
			Game(const char *name,const char *description,const int flags,const char *map) {
				name = name;
				this->IPAddress = description;
				summaryData.flags = flags;
				strcpy_s(summaryData.mapName,12,map);
			}
			Game(const Game &other) {
				name = other.name;
				IPAddress = other.IPAddress;
				summaryData = other.summaryData;
			}
			Game(Game &&other) noexcept {
				name = std::move(other.name);
				IPAddress = std::move(other.IPAddress);
				summaryData = other.summaryData;
			}
		};

		class GameList : public VC6List<Game> {
		public:
			GameList() {
			}

			void AddGame(const Game &game) {
				push_back(game);
			}
			void AddGame(Game &&game) {
				push_back(std::move(game));
			}

			__declspec(dllexport) int Delete();
		};

		//This is an Armada-defined structure, so the contents must remain in the order they are.
		class Player {
		public:
			int IPAddress;
			VC6String name;
			std::atomic<int> ping;
			int unknown[4];
			int unknownIdentifier;
			int unknown2[2];

			Player() {
				memset(unknown,0,sizeof(unknown));
				unknown2[0] = 0;
				unknown2[1] = 0;
			}
			Player(const Player &other) {
				name = other.name;
				ping = other.ping.load();
				IPAddress = other.IPAddress;
				unknownIdentifier = other.unknownIdentifier;
			}
			Player(Player &&other) noexcept : name(std::move(other.name)) {
				ping = other.ping.load();
				IPAddress = other.IPAddress;
				unknownIdentifier = other.unknownIdentifier;
			}

			Player &operator=(Player &&other) noexcept {
				name = std::move(other.name);
				ping = other.ping.load();
				IPAddress = other.IPAddress;
				unknownIdentifier = other.unknownIdentifier;

				return *this;
			}
		};

		class PlayerList : public VC6List<Player> {
			InterfaceData *data;

		public:
			PlayerList(InterfaceData *data) {
				this->data = data;
			}
			PlayerList(PlayerList &other) : VC6List<Player>(other) {
				data = other.data;
			}
			PlayerList(PlayerList &&other) noexcept : VC6List<Player>(std::move(static_cast<VC6List<Player>>(other))) {
				data = other.data;
			}

			void AddPlayer(const Player &player) {
				push_back(player);
			}
			void AddPlayer(Player &&player) {
				push_back(std::move(player));
			}

			__declspec(dllexport) int Delete();
		};

		class Room {
		public:
			VC6String name;
			bool hasPassword;
			int playerCount;

			Room() {
			}
			Room(const char *name,bool hasPassword,int playerCount) {
				this->name = name;
				this->hasPassword = hasPassword;
				this->playerCount = playerCount;
			}
			Room(const Room &other) {
				name = other.name;
				hasPassword = other.hasPassword;
				playerCount = other.playerCount;
			}
			Room(Room &&other) noexcept {
				name = std::move(other.name);
				hasPassword = other.hasPassword;
				playerCount = other.playerCount;
			}
		};

		class RoomList : public VC6List<Room> {
		public:
			void AddRoom(const Room &room) {
				push_back(room);
			}
			void AddRoom(Room &&room) {
				push_back(std::move(room));
			}

			__declspec(dllexport) int Delete();
		};

		class InterfaceData;
		class Interface {
			InterfaceData *data;

			template <typename ListType,typename FunctionType>
			bool UpdateList(std::atomic<ListType*> &listVariable,unsigned long long lastUpdate,FunctionType CommunicatorFunction,const char *logListText);
			template <typename ListType,typename FunctionType>
			ListType *GetList(std::atomic<ListType*> &listVariable,unsigned long long &lastUpdateVariable,FunctionType CommunicatorFunction,const char *logListText);

		public:
			//Armada can intelligently create multiple of these without proper destruction, so the current one needs to be tracked.
			//I think this was only the case before destruction was implemented in Disconnect.
			//inline static Interface *current = nullptr;

			__declspec(dllexport) Interface(HWND windowHandle,int,const std::list<const char *> &list,const char *string1,const char *string2,const char *string3,const char *string4);
			__declspec(dllexport) virtual ~Interface();

			__declspec(dllexport) void Connect();

			__declspec(dllexport) int GetLocalAddress();

			__declspec(dllexport) void AccountChangePassword(const char *newPassword);

			__declspec(dllexport) void AccountCreate(const char *,const char *,const char *);

			__declspec(dllexport) void AccountLogon(const char *userName,const char *password);

			__declspec(dllexport) void Cancel();

			__declspec(dllexport) void Chat(const char *);

			__declspec(dllexport) void Chat(const char *,const int *,int);

			__declspec(dllexport) void Chat(const char *,int);

			__declspec(dllexport) void ContestResult(const GUID *,void *,int,void (*)(bool,void *),void *);

			__declspec(dllexport) void Data(const void *,int);

			__declspec(dllexport) void Data(const void *,int,int);

			__declspec(dllexport) void Disconnect();

			__declspec(dllexport) void GameCreate(const char *,const char *,const GameSummaryData &hue);

			__declspec(dllexport) void GameJoin(const char *,const char *);

			__declspec(dllexport) void GameLaunch(int,int);

			__declspec(dllexport) void GameLeave();

			__declspec(dllexport) void GameRename(const char *);

			__declspec(dllexport) void GameSummaryDataUpdate(const GameSummaryData &);

			__declspec(dllexport) const char *GetCurrentRoom();

			__declspec(dllexport) Event *GetEvent();

			__declspec(dllexport) GameList *GetGames();

			__declspec(dllexport) const char *GetMessageOfTheDay();

			__declspec(dllexport) const char *GetPlayerName(int);

			__declspec(dllexport) PlayerList *GetPlayers();

			__declspec(dllexport) RoomList *GetRooms();

			__declspec(dllexport) const char *GetValidVersion();

			__declspec(dllexport) void RoomCreate(const char *,const char *);

			__declspec(dllexport) void RoomJoin(const char *,const char *);

			__declspec(dllexport) void RoomRefresh();
		};
	}
}