#include "AsstCaller.h"
#include <stdio.h>

using namespace asst;

void test_dorm(Assistance* ptr);
void test_swipe(Assistance* ptr);

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

		test_dorm(ptr);
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
	AsstTestSwipe(ptr, 1000, 300, 500, 300);
}

void test_dorm(Assistance* ptr)
{
	const char* text_array[] = { "×¢ÒâÁ¦" };

	AsstTestOcr(ptr, text_array, sizeof(text_array) / sizeof(char*), true);
}