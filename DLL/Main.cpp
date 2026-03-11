#include "Patch.h"
#include "Logger.h"

BOOL APIENTRY DllMain(HMODULE handle,DWORD event,LPVOID) {
	if (event == DLL_PROCESS_ATTACH) {
		Patch(0x440550,{0x90,0xE9}); //CD check.
		Patch(0x43FF4D,{0xEB,0x53}); //Environment variables buffer overflow.
		Patch(0x539282,{0xEB}); //Remove the modem message when there are more than 4 players in game setup.

		//Change the logic when enter is pressed on the change password dialog.
		//The existing behavior is that it jumps to the WON lobby dialog despite not being logged in and is strange.
		Patch(0x5268D5,{0xE9,0xC9,0,0,0});

		Logger::Initialize();
		Logger::Log("WON DLL loaded.");
	}

	return true;
}