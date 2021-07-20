#include "Assistance.h"
#include "Updater.h"

int main(int argc, char** argv)
{
	using namespace asst;

	auto up = Updater::instance().has_new_version();

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
	asst.start("SanityBegin");

	getchar();

	DebugTraceInfo("Stop");
	asst.stop();

	return 0;
}