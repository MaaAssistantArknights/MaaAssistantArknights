#pragma once

#include <memory>
#include <string>

#include "OcrLitePort.h"
#include "OcrStruct.h"

namespace cv
{
	class Mat;
}
class OcrLite;

class OCRLITE_PORT OcrLiteCaller
{
public:
	OcrLiteCaller();
	~OcrLiteCaller();
	OcrLiteCaller(const OcrLite&) = delete;
	OcrLiteCaller(OcrLite&&) = delete;

	void setNumThread(int numOfThread);
	bool initModels(const std::string& detPath, const std::string& clsPath,
		const std::string& recPath, const std::string& keysPath);

	OcrResult detect(const cv::Mat& mat,
		int padding, int maxSideLen,
		float boxScoreThresh, float boxThresh, float unClipRatio, bool doAngle, bool mostAngle);

	OcrResult detect(const std::string& dir, const std::string& file,
		int padding, int maxSideLen,
		float boxScoreThresh, float boxThresh, float unClipRatio, bool doAngle, bool mostAngle);

	OcrLiteCaller& operator=(const OcrLiteCaller&) = delete;
	OcrLiteCaller& operator=(OcrLiteCaller&&) = delete;
private:
	std::unique_ptr<OcrLite> m_ocrlite_ptr;
};