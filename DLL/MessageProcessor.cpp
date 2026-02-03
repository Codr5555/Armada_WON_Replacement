#include "MessageProcessor.h"
#include "Logger.h"
#include "InterfaceData.h"

MessageProcessor::MessageProcessor(Network::Won::InterfaceData *data) {
	interfaceData = data;
}

void MessageProcessor::ExecuteTCP() {
	constexpr int bufferLength = 1024;
	auto buffer = std::make_unique<char[]>(bufferLength);
	auto bufferSpan = std::span<char>(buffer.get(),bufferLength);
	SOCKET &socket = interfaceData->communicator.TCPSocket;

	while (socket && !interfaceData->shutdown) {
		if (!ReadTCP(bufferSpan,5)) {
			continue;
		}

		unsigned char messageID = *buffer.get();
		if (messageID >= MessageHandlers.size()) {
			Logger::Log("Received an unexpected TCP message from the server.");
			socket = 0;
			continue;
		}

		int messageLength = *std::bit_cast<int*>(buffer.get() + 1);
		Logger::Log(std::format("Received TCP message {}, length {}.",messageID,messageLength).c_str());
		if (!ReadTCP(bufferSpan,messageLength)) {
			continue;
		}

		try {
			Logger::Log(std::format("TCP message data read.  Calling handler \"{}\".",MessageHandlers[messageID].name).c_str());
			(this->*MessageHandlers[messageID].Function)(buffer.get(),messageLength);
		}
		catch (std::exception exception) {
			Logger::Log(std::format("An exception occurred while processing a handler ({}): {}",MessageHandlers[messageID].name,exception.what()).c_str());
			continue;
		}
	}

	//Handle scenarios where a modal dialog is displayed.
	if (!interfaceData->shutdown) {
		interfaceData->AddGenericErrorEvent();
	}
}

void MessageProcessor::ExecuteUDP() {
	constexpr int bufferLength = 1024;
	auto buffer = std::make_unique<char[]>(bufferLength);
	auto bufferSpan = std::span<char>(buffer.get(),bufferLength);
	SOCKET &socket = interfaceData->communicator.UDPSocket;

	while (socket && !interfaceData->shutdown) {
		if (!WaitForData(socket)) {
			continue;
		}

		int recvResult = recvfrom(socket,buffer.get(),bufferLength,0,nullptr,nullptr);
		if (HandleInvalidResult(socket,recvResult,true)) {
			break;
		}

		Logger::Log(std::format("Received a UDP message.  Length: {}",recvResult).c_str());

		if (*std::bit_cast<int*>(buffer.get()) == 1) {
			interfaceData->events.push(new Network::Won::Event(Network::Won::Event::Code::Connect));
			continue;
		}

		//UDP is only used for Data messages except for the connection confirmation.
		try {
			Data(buffer.get(),recvResult);
		}
		catch (std::exception exception) {
			Logger::Log(std::format("An exception occurred while processing the Data handler (UDP): {}",exception.what()).c_str());
			continue;
		}
	}
}

const char *MessageProcessor::GetSocketType(SOCKET socket) {
	int type;
	int size = sizeof(type);
	if (getsockopt(socket,SOL_SOCKET,SO_TYPE,std::bit_cast<char*>(&type),&size) == SOCKET_ERROR) {
		Logger::Log(std::format("Attempting to retrieve the type of a socket failed. ({})  Logging may report the socket as TCP erroneously.",WSAGetLastError()).c_str());
	}
	return type == SOCK_DGRAM ? "UDP" : "TCP";
}

bool MessageProcessor::HandleInvalidResult(SOCKET &socket,int result,bool forRecv) {
	if (forRecv && result == 0) {
		Logger::Log(std::format("The server closed the connection.  ({}: {})",GetSocketType(socket),forRecv ? "recv" : "select").c_str());
		socket = 0;
		return true;
	}
	if (result == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAETIMEDOUT) {
			OutputDebugStringA(std::format("Socket timeout. ({})\r\n",forRecv ? "recv" : "select").c_str());
			return false;
		}

		Logger::Log(std::format("Communication with the server failed.  ({}: {})  Code: {}",GetSocketType(socket),forRecv ? "recv" : "select",WSAGetLastError()).c_str());
		socket = 0;
		return true;
	}
	if (result < 0 && result != SOCKET_ERROR) {
		Logger::Log(std::format("Communication with the server failed. ({}: {})  Code unavailable; result: {}",GetSocketType(socket),forRecv ? "recv" : "select",result).c_str());
		socket = 0;
		return true;
	}
	return false;
}

bool MessageProcessor::WaitForData(SOCKET &socket) {
	static TIMEVAL selectWaitTime = {0,500000};

	fd_set readSet = {1,socket};
	int value = 1;
	while (!interfaceData->shutdown && (value = select(0,&readSet,nullptr,nullptr,&selectWaitTime)) == 0) {
		readSet.fd_count = 1;
		readSet.fd_array[0] = socket;
	}

	if (interfaceData->shutdown) {
		return false;
	}

	return !HandleInvalidResult(socket,value,false);
}

bool MessageProcessor::ReadTCP(std::span<char> buffer,int length) {
	if (length <= 0) {
		return true;
	}

	if (length >= static_cast<int>(buffer.size())) {
		throw new std::exception("The requested length exceeds the buffer size for reads.");
	}

	SOCKET &socket = interfaceData->communicator.TCPSocket;

	if (!WaitForData(socket)) {
		return false;
	}

	int recvValue;
	int readLength = 0;
	while (!interfaceData->shutdown && readLength < length) {
		recvValue = recv(socket,buffer.data() + readLength,length - readLength,0);
		if (recvValue == SOCKET_ERROR) {
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
				break;
			}
			Sleep(0);
			continue;
		}

		readLength += recvValue;
	}
	if (interfaceData->shutdown) {
		return false;
	}

	return !HandleInvalidResult(socket,recvValue,true);
}

void MessageProcessor::ProcessUpdatedPlayerData(const char *data) {
	std::scoped_lock lock(interfaceData->playersMutex);
	interfaceData->players.clear();

	int count = *std::bit_cast<int*>(data);
	data += 4;
	for (int index = 0;index < count;index++) {
		interfaceData->players.AddPlayer(ParsePlayerData(data));
	}
}

void MessageProcessor::TCPThread(Network::Won::InterfaceData *data) {
	try {
		MessageProcessor processor(data);
		processor.ExecuteTCP();
	}
	catch (std::exception exception) {
		Logger::Log(std::format("An unhandled standard exception occurred in the TCP message processor: {}",exception.what()).c_str());
	}
	catch (...) {
		Logger::Log("An unhandled non-standard exception occurred in the TCP message processor.  Information is unavailable.");
	}
}

void MessageProcessor::UDPThread(Network::Won::InterfaceData *data) {
	try {
		MessageProcessor processor(data);
		processor.ExecuteUDP();
	}
	catch (std::exception exception) {
		Logger::Log(std::format("An unhandled standard exception occurred in the UDP message processor: {}",exception.what()).c_str());
	}
	catch (...) {
		Logger::Log("An unhandled non-standard exception occurred in the UDP message processor.  Information is unavailable.");
	}
}