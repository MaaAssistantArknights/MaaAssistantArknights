#include "AsstCaller.h"
#include <stdio.h>

int main(int argc, char** argv)
{
	asst::Assistance * ptr = AsstCreate();

	auto ret = AsstCatchEmulator(ptr);
	if (!ret) {
		getchar();
		return -1;
	}

	AsstStart(ptr, "SanityBegin");

	getchar();

	AsstStop(ptr);

	return 0;
}