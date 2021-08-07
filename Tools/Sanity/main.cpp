#include "AsstCaller.h"
#include <stdio.h>

int main(int argc, char** argv)
{
	asst::Assistance * ptr = AsstCreate();

	auto ret = AsstCatchEmulator(ptr);
	if (!ret) {
		getchar();
		if (ptr) {
			AsstDestory(ptr);
			ptr = nullptr;
		}
		return -1;
	}

	AsstStart(ptr, "SanityBegin");

	getchar();

	AsstStop(ptr);

	if (ptr) {
		AsstDestory(ptr);
		ptr = nullptr;
	}

	return 0;
}