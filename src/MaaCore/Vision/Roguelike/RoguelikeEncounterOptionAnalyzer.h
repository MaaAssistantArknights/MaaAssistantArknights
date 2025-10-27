#pragma once
#include "Vision/VisionHelper.h"

namespace asst
{
class RoguelikeEncounterOptionAnalyzer final : public VisionHelper
{
public:
    struct Option
    {
        bool enabled;
        cv::Mat templ;
        std::string text;
    };

    using Result = std::vector<Option>;
    using ResultOpt = std::optional<Result>;

    using VisionHelper::VisionHelper;
    virtual ~RoguelikeEncounterOptionAnalyzer() override = default;

    bool analyze();

    [[nodiscard]] const Result& get_result() const { return m_result; };

    [[nodiscard]] const cv::Mat& get_img() const { return m_image; };

    void set_theme(const std::string& theme);
    std::optional<int> merge_image(const cv::Mat& new_img);

    using VisionHelper::save_img;

private:
    void set_last_option_y(int last_option_y);
    static cv::Mat binarize_for_ocr(const cv::Mat& image);
    static bool save_img(const cv::Mat& image, std::string_view description = "image");

    std::string m_theme;
    int m_last_option_y = 0;
    size_t m_num_options = 0;
    Result m_result;
};
} // namespace asst
