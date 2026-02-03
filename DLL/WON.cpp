#include <sstream>
#include "WON.h"
#include "ServerCommunicator.h"
#include "Logger.h"
#include "InterfaceData.h"

namespace Network {
	namespace Won {
		std::string GetByteStrings(const void *data,int length) {
			std::stringstream stream;
			for (int index = 0;index < length;index++) {
				stream << std::format("{:02X}",std::bit_cast<unsigned char *>(data)[index]) << " ";
			}
			return stream.str();
		}

		int Event::Delete() {
			delete this;
			return 0;
		}

		int EventFailure::GetAdditionalCode() const {
			return 0;
		}

		EventFailure::Code EventFailure::GetCode() const {
			return errorCode;
		}

		int GameList::Delete() {
			return 0;
		}

		int PlayerList::Delete() {
			if (this != &data->emptyPlayerList) {
				data->playersMutex.unlock();
			}
			return 0;
		}

		int RoomList::Delete() {
			return 0;
		}

		Interface::Interface(HWND windowHandle,int encryptionBase,const std::list<const char *> &serverList,const char *string1,const char *string2,const char *string3,const char *string4) {
			data = new InterfaceData(encryptionBase,serverList);

			OutputDebugStringA("Created interface.\r\n");
		}
		Interface::~Interface() {
			Logger::Log("Destroying interface.");
			delete data;
		}

		void Interface::Connect() {
			try {
				data->communicator.Connect();
			}
			catch (std::exception exception) {
				Logger::Log(std::format("An error occurred while trying to connect to the server: {}",exception.what()).c_str());
				data->AddGenericErrorEvent();
			}
		}

		void Interface::Disconnect() {
			delete this;
		}

		int Interface::GetLocalAddress() {
			return data->communicator.IPAddress;
		}

		void Interface::AccountChangePassword(const char *newPassword) {
		}

		void Interface::AccountCreate(const char *,const char *,const char *) {
		}

		void Interface::AccountLogon(const char *accountName,const char *password) {
			try {
				data->SetActionTime();
				data->communicator.Login(accountName,password);
			}
			catch (std::exception exception) {
				Logger::Log(std::format("An exception occurred when trying to log in: {}",exception.what()).c_str());
				data->AddGenericErrorEvent();
				return;
			}
			data->SetPlayerName(accountName);
		}

		void Interface::Cancel() {
			if (data->CanCancel()) {
				data->AddGenericErrorEvent();
			}
		}

		void Interface::Chat(const char *message) {
			if (*message == 0) {
				return;
			}

			data->communicator.Chat(message);
		}

		void Interface::Chat(const char *message,const int *targetIPAddresses,int targetCount) {
			data->communicator.GroupChat(message,std::span<const int>(targetIPAddresses,targetCount));
		}

		void Interface::Chat(const char *message,int recipientIPAddress) {
			if (data->gameState == InterfaceData::GameState::Lobby) {
				constexpr std::string_view output = "Private messaging in the lobby isn't supported by Armada.  Your message above was not sent.";
				data->events.push(new Network::Won::EventChat(data->communicator.IPAddress,"System",output.data(),output.size()));
				return;
			}

			data->communicator.PrivateChat(message,recipientIPAddress);
		}

		//Armada always sends an empty function pointer for the function parameter.  The last parameter is always 0 and thus useless.
		//However, this may not even be called in internet mode.
		void Interface::ContestResult(const GUID *GUID,void *data,int length,void (*)(bool,void *),void *) {
		}

		void Interface::Data(const void *data,int length) {
			this->data->communicator.Data(data,length);
		}

		//Data targetted at a specific client.
		void Interface::Data(const void *data,int length,int IPAddress) {
			this->data->communicator.DataTargetted(IPAddress,data,length);
		}

		void Interface::GameCreate(const char *name,const char *password,const GameSummaryData &summaryData) {
			data->SetActionTime();
			data->communicator.CreateGame(name,password,summaryData);
		}

		void Interface::GameJoin(const char *IPAddress,const char *password) {
			data->SetActionTime();
			data->communicator.JoinGame(IPAddress,password);
		}

		void Interface::GameLaunch(int numberOfHumans,int numberOfAI) {
			data->gameState = InterfaceData::GameState::InGame;
		}

		void Interface::GameLeave() {
			data->communicator.LeaveGame();
		}

		void Interface::GameRename(const char *name) {
			data->communicator.RenameGame(name);
		}

		void Interface::GameSummaryDataUpdate(const GameSummaryData &summaryData) {
			data->communicator.GameSummaryUpdate(summaryData);
		}

		const char *Interface::GetCurrentRoom() {
			return data->GetCurrentRoom();
		}

		Event *Interface::GetEvent() {
			if (data->events.size() == 0) {
				return nullptr;
			}

			return data->events.pop();
		}

		GameList *Interface::GetGames() {
			if (data->communicator.IPAddress == 0) {
				return &data->emptyGameList;
			}

			return GetList(data->gameList,data->lastGameListUpdate,[](Network::Won::InterfaceData *data){ data->communicator.GetGameList(); },"the game list");
		}

		const char *Interface::GetMessageOfTheDay() {
			return data->MOTD;
		}

		//This is not deallocated by Armada.
		const char *Interface::GetPlayerName(int IPAddress) {
			return data->GetPlayerName(IPAddress);
		}

		PlayerList *Interface::GetPlayers() {
			//Armada intelligently calls this even before logging in and expects a valid list.
			if (*data->GetCurrentRoom() == 0 || data->gameState == InterfaceData::GameState::InGame) {
				return &data->emptyPlayerList;
			}

			//This is unlocked in the PlayerList::Delete call from Armada, which indicates when it's done copying the list.
			data->playersMutex.lock();
			return &data->players;
		}

		//This can be called without RoomRefresh being called first.
		RoomList *Interface::GetRooms() {
			return GetList(data->roomList,data->lastRoomListUpdate,[](Network::Won::InterfaceData *data){ data->communicator.GetRooms(); },"the room list");
		}

		const char *Interface::GetValidVersion() {
			return "v1.2";
		}

		void Interface::RoomCreate(const char *name,const char *password) {
			try {
				data->SetActionTime();
				data->communicator.CreateRoom(name,password);
			}
			catch (const std::exception &exception) {
				Logger::Log(std::format("An error occurred when attempting to create a room: {}",exception.what()).c_str());
				data->AddGenericErrorEvent();
			}
		}

		void Interface::RoomJoin(const char *name,const char *password) {
			try {
				data->SetActionTime();
				data->communicator.JoinRoom(name,password);
			}
			catch (const std::exception &exception) {
				Logger::Log(std::format("An error occurred when attempting to retrieve the room list: {}",exception.what()).c_str());
				data->AddGenericErrorEvent();
			}
		}

		//Not needed with this implementation.
		void Interface::RoomRefresh() {
		}

		template <typename ListType,typename FunctionType>
		bool Interface::UpdateList(std::atomic<ListType*> &listVariable,unsigned long long lastUpdate,FunctionType CommunicatorFunction,const char *logListText) {
			if (data->communicator.IPAddress == 0) {
				return false;
			}

			if (listVariable != nullptr && GetTickCount64() - lastUpdate < 2000) {
				return false;
			}

			delete listVariable;
			listVariable = nullptr;

			try {
				CommunicatorFunction(data);
			}
			catch (const std::exception &exception) {
				Logger::Log(std::format("An error occurred when attempting to retrieve {}: {}",logListText,exception.what()).c_str());

				listVariable = new ListType();
				return false;
			}

			ListType *current = nullptr;
			auto result = WaitOnAddress(&listVariable,&current,4,5000);
			if (!result) {
				if (GetLastError() == ERROR_TIMEOUT) {
					Logger::Log(std::format("Attempting to retrieve {} timed out after 5 seconds.",logListText).c_str());
				}
				else {
					Logger::Log(std::format("An error occurred while waiting for {} to be retrieved.  Code: {}",logListText,GetLastError()).c_str());
				}
				listVariable = new ListType();
				return false;
			}

			return true;
		}

		template <typename ListType,typename FunctionType>
		ListType *Interface::GetList(std::atomic<ListType*> &listVariable,unsigned long long &lastUpdateVariable,FunctionType CommunicatorFunction,const char *logListText) {
			if (UpdateList(listVariable,lastUpdateVariable,CommunicatorFunction,logListText)) {
				lastUpdateVariable = GetTickCount64();
			}

			return listVariable;
		}
	}
}