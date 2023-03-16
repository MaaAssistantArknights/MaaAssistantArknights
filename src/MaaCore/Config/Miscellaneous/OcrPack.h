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
        struct Result
        {
            explicit operator Rect() const noexcept { return rect; }
            explicit operator std::string() const { return to_string(); }

            std::string to_string() const
            {
                return "{ " + text + ": " + rect.to_string() + ", score: " + std::to_string(score) + " }";
            }

            Rect rect;
            std::string text;
            double score = 0.0;
        };
        using ResultVector = std::vector<Result>;
        using ResultProc = std::function<bool(Result&)>;

    public:
        virtual ~OcrPack() override;

        virtual bool load(const std::filesystem::path& path) override;

        ResultVector recognize(const cv::Mat& image, const ResultProc& pred = nullptr,
                                        bool without_det = false, bool trim = true);
        ResultVector recognize(const cv::Mat& image, const Rect& roi, const ResultProc& pred = nullptr,
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
