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
		DebugTraceInfo("Find Simulator:", ret.value());
	}

	DebugTraceInfo("Start");
	asst.start("VisitBegin");

	getchar();

	DebugTraceInfo("Stop");
	asst.stop();

	return 0;
}