#pragma once
#include "AbstractResource.h"

#include <memory>
#include <functional>

#include "AsstDef.h"

class OcrLiteCaller;
namespace cv {
    class Mat;
}

namespace asst {
    class OcrPack final : public AbstractResource
    {
    public:
        OcrPack();
        virtual ~OcrPack();

        virtual bool load(const std::string& dir) override;
        void set_param(int gpu_index, int thread_number);

        std::vector<TextRect> recognize(const cv::Mat& image, const TextRectProc& pred = nullptr);
        std::vector<TextRect> recognize(const cv::Mat& image, const Rect& roi, const TextRectProc& pred = nullptr);

    private:

        std::unique_ptr<OcrLiteCaller> m_ocr_ptr;
    };
}
