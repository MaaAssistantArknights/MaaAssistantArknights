#ifndef __OCR_UTILS_H__
#define __OCR_UTILS_H__

#include <opencv2/core.hpp>
#include "OcrStruct.h"

#include <sys/stat.h>
/*#define __ENABLE_CONSOLE__ true
#define Logger(format, ...) {\
  if(__ENABLE_CONSOLE__) printf(format,##__VA_ARGS__); \
}*/

namespace ocr {
    OCRLITE_EXPORT double getCurrentTime();

    OCRLITE_EXPORT inline bool isFileExists(const std::string& name) {
        struct stat buffer;
        return (stat(name.c_str(), &buffer) == 0);
    }

    OCRLITE_EXPORT std::wstring strToWstr(std::string str);

    OCRLITE_EXPORT ScaleParam getScaleParam(cv::Mat& src, const float scale);

    OCRLITE_EXPORT ScaleParam getScaleParam(cv::Mat& src, const int targetSize);

    OCRLITE_EXPORT std::vector<cv::Point2f> getBox(const cv::RotatedRect& rect);

    OCRLITE_EXPORT int getThickness(cv::Mat& boxImg);

    OCRLITE_EXPORT void drawTextBox(cv::Mat& boxImg, cv::RotatedRect& rect, int thickness);

    OCRLITE_EXPORT void drawTextBox(cv::Mat& boxImg, const std::vector<cv::Point>& box, int thickness);

    OCRLITE_EXPORT void drawTextBoxes(cv::Mat& boxImg, std::vector<TextBox>& textBoxes, int thickness);

    OCRLITE_EXPORT cv::Mat matRotateClockWise180(cv::Mat src);

    OCRLITE_EXPORT cv::Mat matRotateClockWise90(cv::Mat src);

    OCRLITE_EXPORT cv::Mat getRotateCropImage(const cv::Mat& src, std::vector<cv::Point> box);

    OCRLITE_EXPORT cv::Mat adjustTargetImg(cv::Mat& src, int dstWidth, int dstHeight);

    OCRLITE_EXPORT std::vector<cv::Point> getMinBoxes(const std::vector<cv::Point>& inVec, float& minSideLen, float& allEdgeSize);

    OCRLITE_EXPORT float boxScoreFast(const cv::Mat& inMat, const std::vector<cv::Point>& inBox);

    OCRLITE_EXPORT std::vector<cv::Point> unClip(const std::vector<cv::Point>& inBox, float perimeter, float unClipRatio);

    OCRLITE_EXPORT std::vector<int> getAngleIndexes(std::vector<Angle>& angles);

    OCRLITE_EXPORT void saveImg(cv::Mat& img, const char* imgPath);

    OCRLITE_EXPORT std::string getSrcImgFilePath(const char* path, const char* imgName);

    OCRLITE_EXPORT std::string getResultTxtFilePath(const char* path, const char* imgName);

    OCRLITE_EXPORT std::string getResultImgFilePath(const char* path, const char* imgName);

    OCRLITE_EXPORT std::string getDebugImgFilePath(const char* path, const char* imgName, int i, const char* tag);

    OCRLITE_EXPORT void printGpuInfo();
}

#endif //__OCR_UTILS_H__
