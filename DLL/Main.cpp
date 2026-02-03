#include "Patch.h"
#include "Logger.h"

BOOL APIENTRY DllMain(HMODULE handle,DWORD event,LPVOID) {
	if (event == DLL_PROCESS_ATTACH) {
		Patch(0x440550,{0x90,0xE9}); //CD check.
		Patch(0x43FF4D,{0xEB,0x53}); //Environment variables buffer overflow.
		Patch(0x539282,{0xEB}); //Remove the modem message when there are more than 4 players in game setup.

		Logger::Initialize();
		Logger::Log("WON DLL loaded.");
	}

	return true;
}