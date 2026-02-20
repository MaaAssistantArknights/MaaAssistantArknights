#pragma once

#include "Common/AsstTypes.h"
#include "Config/AbstractResource.h"

#include <optional>
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
    using Result = TextRect;
    using ResultsVec = std::vector<Result>;

public:
    virtual ~OcrPack() override;

    virtual bool load(const std::filesystem::path& path) override;

    void use_cpu() { m_gpu_id = std::nullopt; }

    void use_gpu(int gpu_id) { m_gpu_id = gpu_id; }

    ResultsVec recognize(const cv::Mat& image, bool without_det = false, const Rect& roi = Rect());

protected:
    OcrPack();

    bool check_and_load();

    std::unique_ptr<fastdeploy::vision::ocr::DBDetector> m_det;
    std::unique_ptr<fastdeploy::vision::ocr::Recognizer> m_rec;
    std::unique_ptr<fastdeploy::pipeline::PPOCRv3> m_ocr;

    std::filesystem::path m_det_model_path;
    std::filesystem::path m_rec_model_path;
    std::filesystem::path m_rec_label_path;

    std::optional<int> m_gpu_id = std::nullopt;
};

class WordOcr final : public MAA_NS::SingletonHolder<WordOcr>, public OcrPack
{
    friend class MAA_NS::SingletonHolder<WordOcr>;
};

class CharOcr final : public MAA_NS::SingletonHolder<CharOcr>, public OcrPack
{
    friend class MAA_NS::SingletonHolder<CharOcr>;
};
}
