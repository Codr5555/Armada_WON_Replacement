#include "ServerCommunicator.h"
#include "Logger.h"
#include "MessageProcessor.h"
#include "InterfaceData.h"

HostnameResolver::HostnameResolver(const std::string &hostName) {
	addrinfo information = {
		0,
		AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP,
	};

	result = nullptr;
	auto getResult = getaddrinfo(hostName.c_str(),"1701",&information,&result);
	if (getResult != 0) {
		throw std::exception(std::format("Unable to resolve the server address: {}",getResult).c_str());
	}
}

ServerCommunicator::ServerCommunicator(Network::Won::InterfaceData *interfaceData) {
	WSAData data;
	WSAStartup(0x0202,&data);

	TCPSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	if (!TCPMessageProcessor.joinable()) {
		TCPMessageProcessor = std::thread(MessageProcessor::TCPThread,interfaceData);
	}

	int timeout = socketTimeout;
	setsockopt(TCPSocket,SOL_SOCKET,SO_RCVTIMEO,std::bit_cast<char *>(&timeout),4);

	IPAddress = 0;

	this->interfaceData = interfaceData;
}

ServerCommunicator::~ServerCommunicator() {
	WSACleanup();
	TCPMessageProcessor.join();

	if (interfaceData->useUDP) {
		UDPMessageProcessor.join();
	}
}

void ServerCommunicator::Connect() {
	std::ifstream serverFile("Internet Server.txt");
	std::string contents{std::istreambuf_iterator<char>(serverFile),std::istreambuf_iterator<char>()};

	HostnameResolver resolver(contents);
	memcpy_s(&serverAddress,sizeof(serverAddress),resolver.GetAddress(),resolver.GetAddressLength());

	auto connectResult = connect(TCPSocket,std::bit_cast<sockaddr*>(&serverAddress),sizeof(serverAddress));
	if (connectResult == SOCKET_ERROR) {
		throw std::exception(std::format("Failed to connect to the server.  Code: {}",static_cast<int>(WSAGetLastError())).c_str());
	}

	unsigned long argument = 1;
	ioctlsocket(TCPSocket,FIONBIO,&argument);

	//Some degree of protection, though this is not by any means secure.
	char buffer[65];
	*buffer = (char)TCPMessageID::Connect;
	memcpy_s(buffer + 1,64,token,64);
	send(TCPSocket,buffer,65,0);
}

void ServerCommunicator::Login(const char *accountName,const char *password) const {
	int length = strlen(accountName);
	int passwordLength = strlen(password);
	int totalLength = 4 + length + 4 + passwordLength;
	auto buffer = std::make_unique<char[]>(totalLength);
	*std::bit_cast<int*>(buffer.get()) = length;
	memcpy_s(buffer.get() + 4,length,accountName,length);
	*std::bit_cast<int*>(buffer.get() + 4 + length) = passwordLength;
	memcpy_s(buffer.get() + 4 + length + 4,passwordLength,password,passwordLength);
	TCPSend(TCPMessageID::Login,std::span(buffer.get(),totalLength));
}

void ServerCommunicator::GetGameList() const {
	TCPSend(TCPMessageID::GetGameList);
}

void ServerCommunicator::RoomJoinCreate(TCPMessageID ID,const char *name,const char *password) const {
	int nameLength = strlen(name);
	int passwordLength = strlen(password);

	std::array<char,70> buffer;
	*std::bit_cast<int*>(buffer.data()) = nameLength;
	strncpy_s(buffer.data() + 4,31,name,nameLength);
	*std::bit_cast<int*>(buffer.data() + 4 + nameLength) = passwordLength;
	strncpy_s(buffer.data() + 4 + nameLength + 4,31,password,passwordLength);
	TCPSend(ID,std::span<char>(buffer.data(),4 + nameLength + 4 + passwordLength));
}

void ServerCommunicator::JoinRoom(const char *name,const char *password) const {
	RoomJoinCreate(TCPMessageID::JoinRoom,name,password);
}
void ServerCommunicator::CreateRoom(const char *name,const char *password) const {
	RoomJoinCreate(TCPMessageID::CreateRoom,name,password);
}

void ServerCommunicator::GetRooms() const {
	TCPSend(TCPMessageID::GetRooms);
}

void ServerCommunicator::PingResponse() const {
	TCPSend(TCPMessageID::PingResponse);
}

void ServerCommunicator::JoinGame(const char *IPAddress,const char *password) const {
	std::array<char,4 + 4 + 32> buffer;

	in_addr address;
	inet_pton(AF_INET,IPAddress,&address);
	*std::bit_cast<int*>(buffer.data()) = address.S_un.S_addr;

	int passwordLength = strlen(password);
	*std::bit_cast<int*>(buffer.data() + 4) = passwordLength;
	strncpy_s(buffer.data() + 4 + 4,32,password,passwordLength);

	TCPSend(TCPMessageID::JoinGame,buffer);
}

void ServerCommunicator::Data(const void *data,int length) const {
	auto span = std::span<const char>(std::bit_cast<const char*>(data),length);
	if (interfaceData->useUDP && interfaceData->gameState == Network::Won::InterfaceData::GameState::InGame) {
		UDPSend(UDPMessageID::Data,span);
	}
	else {
		TCPSend(TCPMessageID::Data,span);
	}
}

void ServerCommunicator::DataTargetted(int targetIPAddress,const void *data,int length) const {
	auto buffer = std::make_unique<char[]>(4 + length);

	*std::bit_cast<int*>(buffer.get()) = targetIPAddress;
	memcpy_s(buffer.get() + 4,length,data,length);

	auto span = std::span<const char>(buffer.get(),4 + length);
	if (interfaceData->useUDP && interfaceData->gameState == Network::Won::InterfaceData::GameState::InGame) {
		UDPSend(UDPMessageID::DataTargetted,span);
	}
	else {
		TCPSend(TCPMessageID::DataTargetted,span);
	}
}

void ServerCommunicator::Chat(const char *text) const {
	std::array<char,256> buffer;
	int length = strlen(text);

	Logger::Log(std::format("ServerCommunicator::Chat being called with a message of {} characters: {}",length,text).c_str());

	*std::bit_cast<int*>(buffer.data()) = length;
	memcpy_s(&buffer[4],buffer.size() - 4,text,length);
	TCPSend(TCPMessageID::Chat,std::span<const char>(buffer.data(),length + 4));
}

void ServerCommunicator::PrivateChat(const char *text,int recipientIPAddress) const {
	std::array<char,256> buffer;
	*std::bit_cast<int*>(buffer.data()) = recipientIPAddress;
	int length = strlen(text);
	memcpy_s(&buffer[4],buffer.size() - 4,text,length);
	TCPSend(TCPMessageID::PrivateChat,std::span<const char>(buffer.data(),4 + length));
}

void ServerCommunicator::GroupChat(const char *text,std::span<const int> targets) const {
	int textLength = strlen(text);

	Logger::Log(std::format("ServerCommunicator::GroupChat being called with a message of {} characters and {} targets: {}",textLength,targets.size(),text).c_str());

	int bufferLength = 4 + targets.size() * 4 + textLength;
	auto buffer = std::make_unique<char[]>(bufferLength);
	*std::bit_cast<int*>(buffer.get()) = targets.size();

	char *pointer = buffer.get() + 4;
	for (auto &target: targets) {
		*std::bit_cast<int*>(pointer) = target;
		pointer += 4;
	}

	memcpy_s(buffer.get() + 4 + 4 * targets.size(),textLength,text,textLength);
	TCPSend(TCPMessageID::GroupChat,std::span<const char>(buffer.get(),bufferLength));
}

void ServerCommunicator::CreateGame(const char *name,const char *password,const Network::Won::GameSummaryData &data) const {
	std::array<char,59> buffer;

	char *position = buffer.data();

	char nameLength = static_cast<char>(strlen(name));
	*position = nameLength;
	position++;
	strncpy_s(position,20,name,nameLength);
	position += nameLength;

	char passwordLength = static_cast<char>(strlen(password));
	*position = passwordLength;
	position++;
	strncpy_s(position,20,password,passwordLength);
	position += passwordLength;

	*std::bit_cast<int*>(position) = data.flags;
	position += 4;

	char mapNameLength = static_cast<char>(strlen(data.mapName));
	*position = mapNameLength;
	position++;
	strncpy_s(position,12,data.mapName,mapNameLength);
	position += mapNameLength;

	TCPSend(TCPMessageID::CreateGame,std::span<const char>(buffer.data(),position - buffer.data()));
}

void ServerCommunicator::LeaveGame() const {
	TCPSend(TCPMessageID::LeaveGame);
}

void ServerCommunicator::RenameGame(const char *name) {
	TCPSend(TCPMessageID::RenameGame,std::span<const char>(name,strlen(name)));
}

void ServerCommunicator::GameSummaryUpdate(const Network::Won::GameSummaryData &data) {
	TCPSend(TCPMessageID::GameSummaryUpdate,std::span<const char>(std::bit_cast<const char*>(&data),sizeof(Network::Won::GameSummaryData)));
}


void ServerCommunicator::TCPSend(TCPMessageID ID) const {
	TCPSend(ID,std::span<const char>());
}
void ServerCommunicator::TCPSend(TCPMessageID ID,std::span<const char> data) const {
	if (TCPSocket == 0) {
		OutputDebugStringA("A TCPSend call was made with a null socket.\r\n");
		return;
	}

	Logger::Log(std::format("TCPSend called for message ID {}.",std::to_underlying(ID)).c_str());

	int length = 1 + 4 + data.size();
	auto buffer = std::make_unique<char[]>(length);
	*buffer.get() = (int)ID;
	*std::bit_cast<int*>(buffer.get() + 1) = data.size();
	memcpy_s(buffer.get() + 1 + 4,data.size(),data.data(),data.size());
	int result = send(TCPSocket,buffer.get(),length,0);
	if (result < 0) {
		throw std::exception(std::format("A TCP send error ({}) occurred on the socket.",WSAGetLastError()).c_str());
	}
}

void ServerCommunicator::EnableUDP() {
	UDPSocket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (!UDPMessageProcessor.joinable()) {
		UDPMessageProcessor = std::thread(MessageProcessor::UDPThread,interfaceData);
	}
	UDPSend(UDPMessageID::Connect);
}

void ServerCommunicator::UDPSend(UDPMessageID ID) const {
	UDPSend(ID,std::span<const char>());
}
void ServerCommunicator::UDPSend(UDPMessageID ID,std::span<const char> data) const {
	if (UDPSocket == 0) {
		OutputDebugStringA("A UDPSend call was made with a null socket.\r\n");
		return;
	}

	Logger::Log(std::format("UDPSend called for message ID {}.",std::to_underlying(ID)).c_str());

	int length = 1 + data.size();
	auto buffer = std::make_unique<char[]>(length);
	*buffer.get() = (int)ID;
	memcpy_s(buffer.get() + 1,data.size(),data.data(),data.size());
	int result = sendto(UDPSocket,buffer.get(),length,0,std::bit_cast<sockaddr*>(&serverAddress),sizeof(serverAddress));
	if (result < 0) {
		throw std::exception(std::format("A UDP send error ({}) occurred on the socket.",WSAGetLastError()).c_str());
	}
}