#include <OcrLiteNcnn\OcrLite.h>

#include <iostream>
#include <fstream>
#include <chrono>

#include "Assistance.h"
#include "AsstAux.h"
#include "Logger.hpp"

void raw_ocr_test();
void asst_ocr_test();

int main(int argc, char** argv)
{
	//raw_ocr_test();;

	asst_ocr_test();


	return 0;
}

void asst_ocr_test()
{
	using namespace asst;
	Assistance asst;

	auto ret = asst.catch_emulator();
	if (!ret) {
		DebugTraceError("Can't Find Emulator or Permission denied.");
		getchar();
		return;
	}
	else {
		DebugTraceInfo("Find Emulator:", ret.value());
	}
	char c = 0;
	while (c != 'q') {
		asst.start_open_recruit({ 3, 4, 5 });
		c = getchar();
	}
}

void raw_ocr_test()
{
	using namespace ocr;
	using namespace asst;

	const std::string filename = "test.png";

	OcrLite ocr_lite;
	ocr_lite.setNumThread(4);

	//ocr_lite.initLogger(
	//	true,//isOutputConsole
	//	false,//isOutputPartImg
	//	false);//isOutputResultImg

	ocr_lite.enableResultTxt(GetCurrentDir().c_str(), filename.c_str());
	ocr_lite.setGpuIndex(-1);

	const std::string models_dir = GetResourceDir() + "OcrLiteNcnn\\models\\";

	bool init_ret = ocr_lite.initModels(models_dir + "dbnet_op", models_dir + "angle_op", models_dir + "crnn_lite_op", models_dir + "keys.txt");
	if (!init_ret) {
		std::cerr << "initModels failed" << std::endl;
	}

	auto start = std::chrono::system_clock::now();
	OcrResult result = ocr_lite.detect(GetCurrentDir().c_str(), filename.c_str(),
		50, 1024,
		0.6f, 0.3f,
		2.0f, true, true);

	auto end = std::chrono::system_clock::now();
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
}