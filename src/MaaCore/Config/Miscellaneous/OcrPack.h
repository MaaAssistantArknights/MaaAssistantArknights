#pragma once
#include "Config/AbstractResource.h"

#include <functional>

#include "Common/AsstTypes.h"

namespace cv
{
    class Mat;
}

namespace fastdeploy
{
    namespace vision::ocr
    {
        class DBDetector;
        class Recognizer;
    }

    namespace pipeline
    {
        class PPOCRv3;
    }

    struct RuntimeOption;
}

namespace asst
{
    class OcrPack : public AbstractResource
    {
    public:
        virtual ~OcrPack() override;

        virtual bool load(const std::filesystem::path& path) override;

        std::vector<TextRect> recognize(const cv::Mat& image, const TextRectProc& pred = nullptr,
                                        bool without_det = false, bool trim = true);
        std::vector<TextRect> recognize(const cv::Mat& image, const Rect& roi, const TextRectProc& pred = nullptr,
                                        bool without_det = false, bool trim = true);

    protected:
        OcrPack();

        std::unique_ptr<fastdeploy::vision::ocr::DBDetector> m_det;
        std::unique_ptr<fastdeploy::vision::ocr::Recognizer> m_rec;
        std::unique_ptr<fastdeploy::pipeline::PPOCRv3> m_ocr;
    };

    class WordOcr final : public SingletonHolder<WordOcr>, public OcrPack
    {
        friend class SingletonHolder<WordOcr>;
    };

    class CharOcr final : public SingletonHolder<CharOcr>, public OcrPack
    {
        friend class SingletonHolder<CharOcr>;
    };
}
