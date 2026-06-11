#include "Logger.h"

void Logger::Initialize() {
	std::remove("Logs\\WONInterface.txt");
}

void Logger::Log(const char *string) {
	OutputDebugStringA(std::format("{}\r\n",string).c_str());

	std::ofstream stream("Logs\\WONInterface.txt",std::ios_base::app);

	stream << std::chrono::system_clock::now() << ": " << string << "\n";
}