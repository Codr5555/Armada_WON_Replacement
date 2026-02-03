#pragma once

namespace Network::Won {
	struct __declspec(dllexport) GameSummaryData {
		int flags;
		char mapName[12];
	};
}