#pragma once
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"

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

    void set_theme(const std::string& theme);
    std::optional<int> merge_image(const cv::Mat& new_img);

    [[nodiscard]] static Matcher::ResultOpt
        match_option(const std::string& theme, const cv::Mat& image, const cv::Mat& option_templ);

    using VisionHelper::save_img;

private:
    [[nodiscard]] MultiMatcher::ResultsVecOpt analyze_options(const cv::Mat& image) const;
    [[nodiscard]] int get_last_option_y(const cv::Mat& image) const;
    void set_last_option_y(int last_option_y);
    static cv::Mat binarize_for_ocr(const cv::Mat& image);
    static bool save_img(const cv::Mat& image, std::string_view description = "image");

    std::string m_theme;
    int m_last_option_y = -1;
    Result m_result;
};
} // namespace asst
