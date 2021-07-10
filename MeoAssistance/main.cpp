#include "Assistance.h"
#include "Identify.h"

#include <iostream>

int main(int argc, char** argv)
{
	using namespace MeoAssistance;

	Assistance ass;
	if (!ass.setSimulatorType(SimulatorType::BlueStacks)) {
		std::cerr << "failed" << std::endl;
		return -1;
	}

	std::cout << "start" << std::endl;
	ass.start();

	getchar();

	std::cout << "stop" << std::endl;
	ass.stop();

	return 0;
}