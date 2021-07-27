#ifndef __OCR_STRUCT_H__
#define __OCR_STRUCT_H__

#include "opencv2/core.hpp"
#include <vector>

#ifdef __C_API__
#define OCRLITE_EXPORT __declspec(dllexport)
#else
#define OCRLITE_EXPORT
#endif

namespace ncnn {
    class Net;
}

namespace ocr {
    struct OCRLITE_EXPORT ScaleParam {
        int srcWidth;
        int srcHeight;
        int dstWidth;
        int dstHeight;
        float ratioWidth;
        float ratioHeight;
    };

    struct OCRLITE_EXPORT TextBox {
        std::vector<cv::Point> boxPoint;
        float score;
    };

    struct OCRLITE_EXPORT Angle {
        int index;
        float score;
        double time;
    };

    struct OCRLITE_EXPORT TextLine {
        std::string text;
        std::vector<float> charScores;
        double time;
    };

    struct OCRLITE_EXPORT TextBlock {
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

    struct OCRLITE_EXPORT OcrResult {
        double dbNetTime;
        std::vector<TextBlock> textBlocks;
        cv::Mat boxImg;
        double detectTime;
        std::string strRes;
    };
}

#endif //__OCR_STRUCT_H__
