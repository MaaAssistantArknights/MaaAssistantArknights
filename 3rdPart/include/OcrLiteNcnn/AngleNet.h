#ifndef __OCR_ANGLENET_H__
#define __OCR_ANGLENET_H__

#include "OcrStruct.h"
#include <memory>

namespace ocr {
    class AngleNet {
    public:

        ~AngleNet();

        void setNumThread(int numOfThread);

        void setGpuIndex(int gpuIndex);

        bool initModel(const std::string& pathStr);

        std::vector<Angle> getAngles(std::vector<cv::Mat>& partImgs, const char* path,
            const char* imgName, bool doAngle, bool mostAngle);

    private:
        bool isOutputAngleImg = false;
        int numThread;
        std::shared_ptr<ncnn::Net> pNet = std::make_shared<ncnn::Net>();
        const float meanValues[3] = { 127.5, 127.5, 127.5 };
        const float normValues[3] = { 1.0 / 127.5, 1.0 / 127.5, 1.0 / 127.5 };

        const int dstWidth = 192;
        const int dstHeight = 32;

        Angle getAngle(cv::Mat& src);
    };
}

#endif //__OCR_ANGLENET_H__
