#include "AsstCaller.h"
#include <stdio.h>

int main(int argc, char** argv)
{
	using namespace asst;

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
	const char* text_array[] = { "×¢ÒâÁ¦" };

	char ch = 0;
	while (ch != 'q') {
		AsstTestOcr(ptr, text_array, sizeof(text_array) / sizeof(char*), true);
		ch = getchar();
	}

	if (ptr) {
		AsstDestory(ptr);
		ptr = nullptr;
	}

	return 0;
}