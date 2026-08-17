#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <opencv2/core.hpp>

namespace asst::blackflow::perception
{

enum class TemplateRole
{
    Empty,
    CurrentMarker,
    NodeMarker,
    Ordinary,
    Special
};

struct TemplateSpec
{
    std::string name;
    std::string node_type;
    std::string marker_type;
    std::string resource_name;
    double threshold = 0.8;
    bool seed_anchor = false;
    std::string display_name;
    TemplateRole role = TemplateRole::Ordinary;
    cv::Size visual_size;
};

struct TemplateHit
{
    std::string template_name;
    std::string node_type;
    std::string display_name;
    cv::Rect rect;
    double score = 0.0;

    cv::Point2f center() const { return { rect.x + rect.width * 0.5F, rect.y + rect.height * 0.5F }; }
};

struct OcrHit
{
    std::string raw_text;
    std::string normalized_text;
    std::string matched_name;
    std::string runner_up_name;
    std::string node_type;
    cv::Rect rect;
    double score = 0.0;
    double similarity = 0.0;
    double runner_up_similarity = 0.0;
    bool exact_match = false;

    cv::Point2f center() const { return { rect.x + rect.width * 0.5F, rect.y + rect.height * 0.5F }; }
};

struct TemplateScoreMap
{
    cv::Mat scores;
    cv::Size template_size;
};

struct RecognitionScoreAtlas
{
    cv::Rect source_roi;
    std::unordered_map<std::string, TemplateScoreMap> maps;
};

class RecognitionBridge
{
public:
    bool load(const std::filesystem::path& manifest_path, std::string& error);

    const TemplateSpec& empty_template() const noexcept { return m_empty_template; }

    const TemplateSpec& current_marker_template() const noexcept { return m_current_marker_template; }

    const std::vector<TemplateSpec>& node_marker_templates() const noexcept { return m_node_marker_templates; }

    const std::vector<TemplateSpec>& special_templates() const noexcept { return m_special_templates; }

    RecognitionScoreAtlas build_score_atlas(const cv::Mat& image, const cv::Rect& roi) const;
    TemplateHit query_best(const RecognitionScoreAtlas& atlas, const cv::Rect& roi, const TemplateSpec& spec) const;
    TemplateHit query_best_near(
        const RecognitionScoreAtlas& atlas,
        const cv::Rect& roi,
        const TemplateSpec& spec,
        cv::Point2f expected_center,
        double center_tolerance) const;
    std::vector<TemplateHit> query_multi(
        const RecognitionScoreAtlas& atlas,
        const cv::Rect& roi,
        const TemplateSpec& spec,
        double threshold,
        int suppression_radius) const;

    std::vector<OcrHit> recognize_row(const cv::Mat& image, const cv::Rect& roi) const;
    OcrHit match_ocr_hit(OcrHit hit, double minimum_similarity, double minimum_margin, int short_exact_length) const;

    cv::Size visual_size_for(const std::string& node_type) const;
    std::string display_name_for(const std::string& node_type) const;

private:
    struct OcrLabel
    {
        std::string display_name;
        std::string normalized_name;
        std::string node_type;
        std::u32string codepoints;
    };

    struct OcrMatch
    {
        std::string matched_name;
        std::string runner_up_name;
        std::string node_type;
        double similarity = 0.0;
        double runner_up_similarity = 0.0;
        bool exact = false;
        bool accepted = false;
    };

    const cv::Mat& image_for(const TemplateSpec& spec) const;
    const TemplateScoreMap* score_map_for(const RecognitionScoreAtlas& atlas, const TemplateSpec& spec) const;
    OcrMatch match_ocr_text(
        const std::string& text,
        double minimum_similarity,
        double minimum_margin,
        int short_exact_length) const;

    TemplateSpec m_empty_template;
    TemplateSpec m_current_marker_template;
    std::vector<TemplateSpec> m_node_marker_templates;
    std::vector<TemplateSpec> m_ordinary_templates;
    std::vector<TemplateSpec> m_special_templates;
    std::unordered_map<std::string, cv::Mat> m_templates;
    std::vector<OcrLabel> m_ocr_labels;
    std::vector<std::string> m_special_template_prefixes;
};

}
