#pragma once
#include "AbstractResource.h"

#include <functional>

#include "AsstDef.h"

namespace cv
{
    class Mat;
}
struct paddle_ocr_t;

namespace asst
{
    class OcrPack final : public AbstractResource
    {
    public:
        using AbstractResource::AbstractResource;
        virtual ~OcrPack();

        virtual bool load(const std::string& dir) override;

        std::vector<TextRect> recognize(const cv::Mat image, const TextRectProc& pred = nullptr, bool without_det = false);
        std::vector<TextRect> recognize(const cv::Mat image, const Rect& roi, const TextRectProc& pred = nullptr, bool without_det = false);

    private:
        paddle_ocr_t* m_ocr = nullptr;
    };
}
