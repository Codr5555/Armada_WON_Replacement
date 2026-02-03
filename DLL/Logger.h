#pragma once

class Logger {
public:
	static void Initialize();
	static void Log(const char *string);
};