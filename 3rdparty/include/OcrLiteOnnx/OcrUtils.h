#ifndef __OCR_UTILS_H__
#define __OCR_UTILS_H__

#include <opencv2/core.hpp>
#include "OcrStruct.h"
#include "onnxruntime_cxx_api.h"
#include <numeric>
#include <sys/stat.h>

template<typename T, typename... Ts>
static std::unique_ptr<T> makeUnique(Ts &&... params) {
    return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
}

template<typename T>
static double getMean(std::vector<T> &input) {
    auto sum = accumulate(input.begin(), input.end(), 0.0);
    return sum / input.size();
}

template<typename T>
static double getStdev(std::vector<T> &input, double mean) {
    if (input.size() <= 1) return 0;
    double accum = 0.0;
    for_each(input.begin(), input.end(), [&](const double d) {
        accum += (d - mean) * (d - mean);
    });
    double stdev = sqrt(accum / (input.size() - 1));
    return stdev;
}

double getCurrentTime();

inline bool isFileExists(const std::string &name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

#ifdef _WIN32
#define my_strtol wcstol
#define my_strrchr wcsrchr
#define my_strcasecmp _wcsicmp
#define my_strdup _strdup
#else
#define my_strtol strtol
#define my_strrchr strrchr
#define my_strcasecmp strcasecmp
#define my_strdup strdup
#endif

std::wstring strToWstr(std::string str);

ScaleParam getScaleParam(cv::Mat &src, const float scale);

ScaleParam getScaleParam(cv::Mat &src, const int targetSize);

std::vector<cv::Point2f> getBox(const cv::RotatedRect &rect);

int getThickness(cv::Mat &boxImg);

void drawTextBox(cv::Mat &boxImg, cv::RotatedRect &rect, int thickness);

void drawTextBox(cv::Mat &boxImg, const std::vector<cv::Point> &box, int thickness);

void drawTextBoxes(cv::Mat &boxImg, std::vector<TextBox> &textBoxes, int thickness);

cv::Mat matRotateClockWise180(cv::Mat src);

cv::Mat matRotateClockWise90(cv::Mat src);

cv::Mat getRotateCropImage(const cv::Mat &src, std::vector<cv::Point> box);

cv::Mat adjustTargetImg(cv::Mat &src, int dstWidth, int dstHeight);

std::vector<cv::Point> getMinBoxes(const std::vector<cv::Point> &inVec, float &minSideLen, float &allEdgeSize);

float boxScoreFast(const cv::Mat &inMat, const std::vector<cv::Point> &inBox);

std::vector<cv::Point> unClip(const std::vector<cv::Point> &inBox, float perimeter, float unClipRatio);

std::vector<float> substractMeanNormalize(cv::Mat &src, const float *meanVals, const float *normVals);

std::vector<int> getAngleIndexes(std::vector<Angle> &angles);

std::vector<char *> getInputNames(Ort::Session *session);

std::vector<char *> getOutputNames(Ort::Session *session);

void getInputName(Ort::Session *session, char *&inputName);

void getOutputName(Ort::Session *session, char *&outputName);

void saveImg(cv::Mat &img, const char *imgPath);

std::string getSrcImgFilePath(const char *path, const char *imgName);

std::string getResultTxtFilePath(const char *path, const char *imgName);

std::string getResultImgFilePath(const char *path, const char *imgName);

std::string getDebugImgFilePath(const char *path, const char *imgName, int i, const char *tag);

#endif //__OCR_UTILS_H__
