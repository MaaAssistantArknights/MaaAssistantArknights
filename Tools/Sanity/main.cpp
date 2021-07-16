#include "Assistance.h"

int main(int argc, char** argv)
{
	using namespace asst;

	Assistance asst;

	auto ret = asst.setSimulator();
	if (!ret) {
		DebugTraceError("Can't Find Simulator or Permission denied.");
		getchar();
		return -1;
	}
	else {
		DebugTraceInfo("Find Simulator: %s", ret->c_str());
	}

	DebugTraceInfo("Start");
	asst.start("SanityBegin");

	getchar();

	DebugTraceInfo("Stop");
	asst.stop();

	return 0;
}