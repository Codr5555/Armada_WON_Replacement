#include "Patch.h"
#include "Logger.h"

BOOL APIENTRY DllMain(HMODULE handle,DWORD event,LPVOID) {
	if (event == DLL_PROCESS_ATTACH) {
		//Remove the CD check.
		Patch(0x440550,{0x90,0xE9});

		//Fix the environment variables buffer overflow.
		Patch(0x43FF4D,{0xEB,0x53});

		//Remove the modem message when there are more than 4 players in game setup.
		Patch(0x539282,{0xEB});

		//Change the logic when enter is pressed on the change password dialog.
		//The existing behavior is that it jumps to the WON lobby dialog despite not being logged in and is strange.
		Patch(0x5268D5,{0xE9,0xC9,0,0,0});

		//Disable mouse cursor locking when the game isn't active.  (Taken from GOG and slightly adjusted to put the new function closer to the source for better caching.)
		Patch(0x477322,{0x8A,0x05,0xB0,0x37,0x63,0x00,0x56,0x85,0xC0,0xE9,0x4F,0x01,0x00,0x00});
		Patch(0x477479,{0xE9,0xA4,0xFE,0xFF,0xFF,0x90});

		//Fix the crash when Armada isn't focused at loading screen completion.
		Patch(0x5BA04C,{0xEB,0x4F});
		Patch(0x5BA09D,{0x8B,0x0D,0xA0,0x4A,0x6D,0x00,0xE8,0x98,0x13,0xFD,0xFF,0x5E,0x5B,0xEB,0xA2});

		//Remove outdated hardware checks for z-buffer support.
		Patch(0x586D7A,{0x90,0x90});

		//Enable all available 32-bit resolutions.
		Patch(0x5441D2,{0xB}); //Reduces the height of the resolution combobox items to fit more into the list.
		Patch(0x58AB6F,0x90,90); //Remove explicit resolution checks for filtering out possible options for selection.
		Patch(0x58ABD1,0x90,25); //^
		Patch(0x58ABCE,{0x20,0x75}); //Allow 32 bits per pixel formats only.

		Logger::Initialize();
		Logger::Log("WON DLL loaded.");
	}

	return true;
}