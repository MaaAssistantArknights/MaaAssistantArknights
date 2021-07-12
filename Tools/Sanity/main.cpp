#include "Assistance.h"

int main(int argc, char** argv)
{
	using namespace asst;

	Assistance asst;
	if (!asst.setSimulatorType(SimulatorType::BlueStacks)) {
		DebugTraceError("Can't Find Simulator or Permission denied.");
		getchar();
		return -1;
	}

	DebugTraceInfo("Start");
	asst.start();

	getchar();

	DebugTraceInfo("Stop");
	asst.stop();

	return 0;
}