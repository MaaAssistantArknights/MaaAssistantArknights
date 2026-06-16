#pragma once

#include "Common/AsstTypes.h"
#include "Config/AbstractResource.h"

#include <memory>
#include <optional>
#include <vector>

namespace cv
{
class Mat;
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

    ResultsVec recognize(const cv::Mat& image, bool without_det = false, const std::optional<Rect>& base_roi = {});

protected:
    OcrPack();

    bool check_and_load() const;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
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
