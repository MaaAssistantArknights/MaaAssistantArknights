#include "Assistance.h"
#include "AsstAux.h"
#include "Logger.hpp"

int main(int argc, char** argv)
{
	using namespace asst;

	Assistance asst;

	auto ret = asst.set_emulator();
	if (!ret) {
		DebugTraceError("Can't Find Emulator or Permission denied.");
		getchar();
		return -1;
	}
	else {
		DebugTraceInfo("Find Emulator:", ret.value());
	}

	char ch = 0;
	while (ch != EOF) {
		ch = getchar();
		auto time_str = StringReplaceAll(StringReplaceAll(GetFormatTimeString(), " ", "_"), ":", "-");
		std::string filename = GetCurrentDir() + "screenshot\\" + time_str + ".png";
		asst.print_window(filename);
	}

	return 0;
}