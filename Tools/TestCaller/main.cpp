#include "AsstCaller.h"
#include <stdio.h>

using namespace asst;

int main(int argc, char** argv)
{
	Assistance* ptr = AsstCreate();
	auto ret = AsstCatchEmulator(ptr);
	if (!ret) {
		getchar();
		if (ptr) {
			AsstDestory(ptr);
			ptr = nullptr;
		}
		return -1;
	}

	char ch = 0;
	while (ch != 'q') {
		//AsstStartInfrast(ptr);
		AsstStartDebugTask(ptr);

		ch = getchar();
	}

	if (ptr) {
		AsstDestory(ptr);
		ptr = nullptr;
	}

	return 0;
}