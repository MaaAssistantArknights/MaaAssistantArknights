#include "AsstCaller.h"
#include <stdio.h>

using namespace asst;

void debug_task(Assistance* ptr);
void test_swipe(Assistance* ptr);
void test_infrast(Assistance* ptr);

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
		test_infrast(ptr);
		//debug_task(ptr);

		ch = getchar();
	}

	if (ptr) {
		AsstDestory(ptr);
		ptr = nullptr;
	}

	return 0;
}

void debug_task(Assistance* ptr)
{
	AsstDebugTask(ptr);
}

void test_infrast(Assistance* ptr)
{
	AsstStartInfrast(ptr);
}