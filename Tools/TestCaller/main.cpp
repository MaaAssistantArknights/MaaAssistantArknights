#include "AsstCaller.h"
#include <stdio.h>

using namespace asst;

void test_ocr(Assistance* ptr);
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
		//test_swipe(ptr);

		ch = getchar();
	}

	if (ptr) {
		AsstDestory(ptr);
		ptr = nullptr;
	}

	return 0;
}

void test_swipe(Assistance* ptr)
{
	AsstTestSwipe(ptr, 2500, 300, 100, 300);
}

void test_ocr(Assistance* ptr)
{
	const char* text_array[] = { "×¢ÒâÁ¦" };

	AsstTestOcr(ptr, text_array, sizeof(text_array) / sizeof(char*), true);
}

void test_infrast(Assistance* ptr)
{
	AsstStartInfrast(ptr);
}