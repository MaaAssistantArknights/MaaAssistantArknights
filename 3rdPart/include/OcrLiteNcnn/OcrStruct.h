#ifndef __OCR_STRUCT_H__
#define __OCR_STRUCT_H__

#include <vector>
#include <string>
#include <memory>

namespace cv
{
    class Mat;
    class RotatedRect;

    template<typename _Tp> class Point_;
    typedef Point_<int> Point2i;
    typedef Point_<float> Point2f;
    typedef Point2i Point;

    template<typename _Tp> class Rect_;
    typedef Rect_<int> Rect2i;
    typedef Rect2i Rect;
}

namespace ncnn
{
    class Net;
}

#ifdef __C_DLL__
#define __API_EXPORT__ __declspec(dllexport)
#else 
#define __API_EXPORT__
#endif // __C_DLL__

struct __API_EXPORT__ ScaleParam {
    int srcWidth;
    int srcHeight;
    int dstWidth;
    int dstHeight;
    float ratioWidth;
    float ratioHeight;
};

struct __API_EXPORT__ TextBox {
    std::vector<cv::Point> boxPoint;
    float score;
};

struct __API_EXPORT__ Angle {
    int index;
    float score;
    double time;
};

struct __API_EXPORT__ TextLine {
    std::string text;
    std::vector<float> charScores;
    double time;
};

struct __API_EXPORT__ TextBlock {
    std::vector<cv::Point> boxPoint;
    float boxScore;
    int angleIndex;
    float angleScore;
    double angleTime;
    std::string text;
    std::vector<float> charScores;
    double crnnTime;
    double blockTime;
};

struct __API_EXPORT__ OcrResult {
    double dbNetTime;
    std::vector<TextBlock> textBlocks;
    std::shared_ptr<cv::Mat> pBoxImg = std::make_shared<cv::Mat>();
    double detectTime;
    std::string strRes;
};

#endif //__OCR_STRUCT_H__
