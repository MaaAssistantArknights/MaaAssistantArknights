#include "Assistance.h"

int main(int argc, char** argv)
{
	using namespace MeoAssistance;

	Assistance ass;
	if (!ass.setSimulatorType(SimulatorType::BlueStacks)) {
		DebugTraceError("Find Simulator Error");
		getchar();
		return -1;
	}

	DebugTraceInfo("Start");
	ass.start();

	getchar();

	DebugTraceInfo("Stop");
	ass.stop();

	return 0;
}