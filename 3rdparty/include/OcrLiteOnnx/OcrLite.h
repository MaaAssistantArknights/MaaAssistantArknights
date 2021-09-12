#ifndef __OCR_LITE_H__
#define __OCR_LITE_H__

#include "opencv2/core.hpp"
#include "onnxruntime_cxx_api.h"
#include "OcrStruct.h"
#include "DbNet.h"
#include "AngleNet.h"
#include "CrnnNet.h"

class OcrLite {
public:
    OcrLite();

    ~OcrLite();

    void setNumThread(int numOfThread);

    void initLogger(bool isConsole, bool isPartImg, bool isResultImg);

    void enableResultTxt(const char *path, const char *imgName);

    void initModels(const std::string &detPath, const std::string &clsPath,
                    const std::string &recPath, const std::string &keysPath);

    void Logger(const char *format, ...);

    OcrResult detect(const char *path, const char *imgName,
                     int padding, int maxSideLen,
                     float boxScoreThresh, float boxThresh, float unClipRatio, bool doAngle, bool mostAngle);


	OcrResult detect(const cv::Mat& mat,
		int padding, int maxSideLen,
		float boxScoreThresh, float boxThresh, float unClipRatio, bool doAngle, bool mostAngle);

private:
    bool isOutputConsole = false;
    bool isOutputPartImg = false;
    bool isOutputResultTxt = false;
    bool isOutputResultImg = false;
    FILE *resultTxt;
    DbNet dbNet;
    AngleNet angleNet;
    CrnnNet crnnNet;

    std::vector<cv::Mat> getPartImages(cv::Mat &src, std::vector<TextBox> &textBoxes,
                                       const char *path, const char *imgName);

    OcrResult detect(const char *path, const char *imgName,
                     cv::Mat &src, cv::Rect &originRect, ScaleParam &scale,
                     float boxScoreThresh = 0.6f, float boxThresh = 0.3f,
                     float unClipRatio = 2.0f, bool doAngle = true, bool mostAngle = true);
};

#endif //__OCR_LITE_H__
