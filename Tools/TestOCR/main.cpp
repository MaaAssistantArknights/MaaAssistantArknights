#include <OcrLiteNcnn\OcrLite.h>
#include <AsstAux.h>
#include <iostream>

int main(int argc, char** argv)
{
	using namespace ocr;
	using namespace asst;

	const std::string filename = "test.png";

	OcrLite ocr_lite;
	ocr_lite.setNumThread(4);
	ocr_lite.enableResultTxt(GetCurrentDir().c_str(), filename.c_str());
	ocr_lite.setGpuIndex(0);

	const std::string models_dir = GetResourceDir() + "OcrLiteNcnn\\models\\";

	bool init_ret = ocr_lite.initModels(models_dir + "dbnet_op", models_dir + "angle_op", models_dir + "crnn_lite_op", models_dir + "keys.txt");
	if (!init_ret) {
		std::cerr << "initModels failed" << std::endl;
	}

	OcrResult result = ocr_lite.detect(GetCurrentDir().c_str(), filename.c_str(),
		50, 1024,
		0.6f, 0.3f,
		2.0f, true, true);

	std::cout << result.strRes << std::endl;

	return 0;
}