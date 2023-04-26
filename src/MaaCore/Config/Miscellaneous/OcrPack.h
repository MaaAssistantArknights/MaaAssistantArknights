#pragma once

#include "Common/AsstTypes.h"
#include "Config/AbstractResource.h"

#include <vector>

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
        struct Result
        {
            std::string to_string() const
            {
                return "{ " + text + ": " + rect.to_string() + ", score: " + std::to_string(score) + " }";
            }
            explicit operator std::string() const { return to_string(); }

            std::string text;
            Rect rect;
            double score = 0.0;
        };

        using ResultsVec = std::vector<Result>;

    public:
        virtual ~OcrPack() override;

        virtual bool load(const std::filesystem::path& path) override;

        ResultsVec recognize(const cv::Mat& image, bool without_det = false);

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
