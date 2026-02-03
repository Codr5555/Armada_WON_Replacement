#include "Logger.h"

void Logger::Initialize() {
	std::remove("Logs\\WONInterface.txt");
}

void Logger::Log(const char *string) {
	OutputDebugStringA(std::format("{}\r\n",string).c_str());

	std::ofstream stream("Logs\\WONInterface.txt",std::ios_base::app);

	const auto time = std::chrono::system_clock::now();
	stream << std::chrono::current_zone()->to_local(time) << ": " << string << "\n";
}