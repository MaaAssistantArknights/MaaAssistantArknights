#include "Vision/Roguelike/BlackFlow/RecognitionBridge.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

#include "Config/TemplResource.h"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

namespace asst::blackflow::perception
{
namespace
{

cv::Rect clamp_roi(const cv::Rect& roi, const cv::Size& size)
{
    return roi & cv::Rect(0, 0, size.width, size.height);
}

asst::Matcher::RawResult run_match(const cv::Mat& source, const cv::Mat& templ)
{
    asst::MatcherConfig::Params params;
    params.templs.emplace_back(templ);
    params.templ_thres.emplace_back(0.0);
    params.methods.emplace_back(asst::MatchMethod::Ccoeff);
    auto raw = asst::Matcher::preproc_and_match(source, params);
    if (raw.empty()) {
        throw std::runtime_error("Maa Matcher::preproc_and_match returned no result");
    }
    return std::move(raw.front());
}

cv::Rect
    score_search_rect(const RecognitionScoreAtlas& atlas, const cv::Rect& requested_roi, const cv::Size& template_size)
{
    if (atlas.source_roi.empty() || requested_roi.empty()) {
        return {};
    }
    const int atlas_right = atlas.source_roi.x + atlas.source_roi.width - template_size.width;
    const int atlas_bottom = atlas.source_roi.y + atlas.source_roi.height - template_size.height;
    const int request_right = requested_roi.x + requested_roi.width - template_size.width;
    const int request_bottom = requested_roi.y + requested_roi.height - template_size.height;
    const int left = std::max(atlas.source_roi.x, requested_roi.x);
    const int top = std::max(atlas.source_roi.y, requested_roi.y);
    const int right = std::min(atlas_right, request_right);
    const int bottom = std::min(atlas_bottom, request_bottom);
    if (right < left || bottom < top) {
        return {};
    }
    return { left - atlas.source_roi.x, top - atlas.source_roi.y, right - left + 1, bottom - top + 1 };
}

bool is_han(char32_t value)
{
    return value == 0x3007 || (value >= 0x3400 && value <= 0x4DBF) || (value >= 0x4E00 && value <= 0x9FFF) ||
           (value >= 0xF900 && value <= 0xFAFF) || (value >= 0x20000 && value <= 0x3134F);
}

std::u32string decode_utf8(std::string_view text)
{
    std::u32string output;
    for (std::size_t index = 0; index < text.size();) {
        const auto first = static_cast<unsigned char>(text[index]);
        char32_t value = 0;
        std::size_t length = 0;
        if (first < 0x80) {
            value = first;
            length = 1;
        }
        else if ((first & 0xE0) == 0xC0) {
            value = first & 0x1F;
            length = 2;
        }
        else if ((first & 0xF0) == 0xE0) {
            value = first & 0x0F;
            length = 3;
        }
        else if ((first & 0xF8) == 0xF0) {
            value = first & 0x07;
            length = 4;
        }
        else {
            ++index;
            continue;
        }
        if (index + length > text.size()) {
            break;
        }
        bool valid = true;
        for (std::size_t offset = 1; offset < length; ++offset) {
            const auto next = static_cast<unsigned char>(text[index + offset]);
            if ((next & 0xC0) != 0x80) {
                valid = false;
                break;
            }
            value = (value << 6) | (next & 0x3F);
        }
        if (!valid || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) {
            ++index;
            continue;
        }
        output.push_back(value);
        index += length;
    }
    return output;
}

std::string encode_utf8(const std::u32string& text)
{
    std::string output;
    for (const char32_t value : text) {
        if (value <= 0x7F) {
            output.push_back(static_cast<char>(value));
        }
        else if (value <= 0x7FF) {
            output.push_back(static_cast<char>(0xC0 | (value >> 6)));
            output.push_back(static_cast<char>(0x80 | (value & 0x3F)));
        }
        else if (value <= 0xFFFF) {
            output.push_back(static_cast<char>(0xE0 | (value >> 12)));
            output.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | (value & 0x3F)));
        }
        else {
            output.push_back(static_cast<char>(0xF0 | (value >> 18)));
            output.push_back(static_cast<char>(0x80 | ((value >> 12) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | (value & 0x3F)));
        }
    }
    return output;
}

std::pair<std::u32string, std::string> normalize_chinese(std::string_view text)
{
    std::u32string codepoints;
    for (const char32_t value : decode_utf8(text)) {
        if (is_han(value)) {
            codepoints.push_back(value);
        }
    }
    return { codepoints, encode_utf8(codepoints) };
}

std::size_t levenshtein_distance(const std::u32string& left, const std::u32string& right)
{
    std::vector<std::size_t> previous(right.size() + 1);
    std::vector<std::size_t> current(right.size() + 1);
    for (std::size_t column = 0; column <= right.size(); ++column) {
        previous[column] = column;
    }
    for (std::size_t row = 1; row <= left.size(); ++row) {
        current[0] = row;
        for (std::size_t column = 1; column <= right.size(); ++column) {
            const std::size_t substitution = previous[column - 1] + (left[row - 1] == right[column - 1] ? 0 : 1);
            current[column] = std::min({ previous[column] + 1, current[column - 1] + 1, substitution });
        }
        previous.swap(current);
    }
    return previous.back();
}

double normalized_similarity(const std::u32string& left, const std::u32string& right)
{
    const std::size_t length = std::max(left.size(), right.size());
    if (length == 0) {
        return 0.0;
    }
    return 1.0 - static_cast<double>(levenshtein_distance(left, right)) / static_cast<double>(length);
}

bool has_special_template_prefix(std::string_view name, const std::vector<std::string>& prefixes)
{
    return std::ranges::any_of(prefixes, [&](const std::string& prefix) { return name.starts_with(prefix); });
}

TemplateRole parse_role(
    const nlohmann::json& entry,
    const std::string& node_type,
    std::string_view name,
    const std::vector<std::string>& special_prefixes)
{
    const std::string role = entry.value("role", std::string());
    if (role == "node_marker") {
        return TemplateRole::NodeMarker;
    }
    if (role == "empty" || node_type == "empty") {
        return TemplateRole::Empty;
    }
    if (role == "current_marker" || node_type == "current_marker") {
        return TemplateRole::CurrentMarker;
    }
    if (role == "special" || node_type == "final" || node_type == "battle_boss" || node_type == "battle_mid_boss" ||
        has_special_template_prefix(name, special_prefixes)) {
        return TemplateRole::Special;
    }
    return TemplateRole::Ordinary;
}

}

bool RecognitionBridge::load(const std::filesystem::path& manifest_path, std::string& error)
{
    try {
        if (!std::filesystem::is_regular_file(manifest_path)) {
            error = "Template manifest is missing: " + manifest_path.string();
            return false;
        }
        std::ifstream input(manifest_path, std::ios::binary);
        if (!input) {
            error = "Template manifest cannot be opened: " + manifest_path.string();
            return false;
        }
        nlohmann::json manifest;
        input >> manifest;
        const int schema_version = manifest.value("schema_version", 0);
        if ((schema_version != 1 && schema_version != 2) || !manifest.contains("templates") ||
            !manifest["templates"].is_array()) {
            error = "Unsupported template manifest schema: " + manifest_path.string();
            return false;
        }

        m_empty_template = {};
        m_current_marker_template = {};
        m_node_marker_templates.clear();
        m_ordinary_templates.clear();
        m_special_templates.clear();
        m_templates.clear();
        m_ocr_labels.clear();
        m_special_template_prefixes.clear();

        std::string template_resource_root = manifest.at("template_root").get<std::string>();
        std::replace(template_resource_root.begin(), template_resource_root.end(), '\\', '/');
        while (template_resource_root.ends_with('/')) {
            template_resource_root.pop_back();
        }
        if (template_resource_root.empty()) {
            error = "Template manifest template_root must not be empty";
            return false;
        }
        if (manifest.contains("special_template_prefixes")) {
            if (!manifest["special_template_prefixes"].is_array()) {
                error = "Template manifest special_template_prefixes must be an array";
                return false;
            }
            for (const auto& prefix : manifest["special_template_prefixes"]) {
                m_special_template_prefixes.push_back(prefix.get<std::string>());
            }
        }

        const auto add_ocr_label = [&](const std::string& text,
                                       const std::string& display_name,
                                       const std::string& node_type) {
            auto [codepoints, normalized_name] = normalize_chinese(text);
            if (codepoints.empty()) {
                return;
            }
            const auto existing = std::find_if(m_ocr_labels.begin(), m_ocr_labels.end(), [&](const OcrLabel& label) {
                return label.normalized_name == normalized_name;
            });
            OcrLabel label { display_name, std::move(normalized_name), node_type, std::move(codepoints) };
            if (existing == m_ocr_labels.end()) {
                m_ocr_labels.push_back(std::move(label));
            }
            else {
                *existing = std::move(label);
            }
        };

        for (const auto& entry : manifest["templates"]) {
            const std::string relative_file = entry.at("file").get<std::string>();
            TemplateSpec spec;
            spec.name = std::filesystem::path(relative_file).filename().string();
            spec.node_type = entry.value("node_type", std::string());
            spec.marker_type = entry.value("marker_type", std::string());
            spec.resource_name = template_resource_root + "/" + relative_file;
            std::replace(spec.resource_name.begin(), spec.resource_name.end(), '\\', '/');
            spec.threshold = entry.value("threshold", 0.8);
            spec.seed_anchor = entry.value("seed_anchor", false);
            spec.display_name = entry.value("display_name", std::string());
            if (has_special_template_prefix(spec.name, m_special_template_prefixes)) {
                spec.node_type = "battle_boss";
                spec.display_name = "险路恶敌";
            }
            spec.role = parse_role(entry, spec.node_type, spec.name, m_special_template_prefixes);
            spec.visual_size = cv::Size(entry.value("width", 0), entry.value("height", 0));
            if (spec.role == TemplateRole::Empty) {
                m_empty_template = spec;
            }
            else if (spec.role == TemplateRole::CurrentMarker) {
                m_current_marker_template = spec;
            }
            else if (spec.role == TemplateRole::NodeMarker) {
                if (spec.marker_type.empty()) {
                    error = "Node marker template must define marker_type: " + spec.name;
                    return false;
                }
                m_node_marker_templates.push_back(spec);
            }
            else if (spec.role == TemplateRole::Special) {
                m_special_templates.push_back(spec);
            }
            else {
                m_ordinary_templates.push_back(spec);
                add_ocr_label(spec.display_name, spec.display_name, spec.node_type);
            }
        }

        if (manifest.contains("ocr_labels")) {
            if (!manifest["ocr_labels"].is_array()) {
                error = "Template manifest ocr_labels must be an array";
                return false;
            }
            for (const auto& entry : manifest["ocr_labels"]) {
                const std::string text = entry.at("text").get<std::string>();
                add_ocr_label(text, entry.value("display_name", text), entry.at("node_type").get<std::string>());
            }
        }

        if (m_empty_template.resource_name.empty() || m_current_marker_template.resource_name.empty()) {
            error = "Template manifest must define empty and current_marker templates";
            return false;
        }
        if (m_special_templates.empty() || m_ocr_labels.empty()) {
            error = "Template manifest must define special templates and ordinary OCR labels";
            return false;
        }

        std::vector<TemplateSpec> matched { m_empty_template, m_current_marker_template };
        matched.insert(matched.end(), m_node_marker_templates.begin(), m_node_marker_templates.end());
        matched.insert(matched.end(), m_special_templates.begin(), m_special_templates.end());
        for (auto& spec : matched) {
            cv::Mat image = TemplResource::get_instance().get_templ(spec.resource_name);
            if (image.empty()) {
                error = "Template cannot be decoded: " + spec.resource_name;
                return false;
            }
            if (spec.visual_size.empty()) {
                spec.visual_size = image.size();
            }
            m_templates.emplace(spec.name, std::move(image));
        }
        return true;
    }
    catch (const std::exception& exception) {
        error = "Template manifest cannot be loaded: " + std::string(exception.what());
        return false;
    }
    catch (...) {
        error = "Template manifest cannot be loaded: unknown exception";
        return false;
    }
}

const cv::Mat& RecognitionBridge::image_for(const TemplateSpec& spec) const
{
    const auto found = m_templates.find(spec.name);
    if (found == m_templates.end()) {
        throw std::runtime_error("Template was not loaded for matching: " + spec.name);
    }
    return found->second;
}

const TemplateScoreMap*
    RecognitionBridge::score_map_for(const RecognitionScoreAtlas& atlas, const TemplateSpec& spec) const
{
    const auto found = atlas.maps.find(spec.name);
    return found == atlas.maps.end() ? nullptr : &found->second;
}

RecognitionScoreAtlas RecognitionBridge::build_score_atlas(const cv::Mat& image, const cv::Rect& requested_roi) const
{
    RecognitionScoreAtlas atlas;
    if (image.empty()) {
        return atlas;
    }
    atlas.source_roi = clamp_roi(requested_roi, image.size());
    if (atlas.source_roi.empty()) {
        return atlas;
    }

    std::vector<TemplateSpec> matched { m_empty_template, m_current_marker_template };
    matched.insert(matched.end(), m_node_marker_templates.begin(), m_node_marker_templates.end());
    matched.insert(matched.end(), m_special_templates.begin(), m_special_templates.end());
    for (const auto& spec : matched) {
        const cv::Mat& templ = image_for(spec);
        if (atlas.source_roi.width < templ.cols || atlas.source_roi.height < templ.rows) {
            continue;
        }
        auto raw = run_match(image(atlas.source_roi), templ);
        if (raw.matched.empty()) {
            continue;
        }
        cv::Mat scores;
        if (raw.matched.type() == CV_32FC1) {
            scores = std::move(raw.matched);
        }
        else {
            raw.matched.convertTo(scores, CV_32FC1);
        }
        atlas.maps.emplace(spec.name, TemplateScoreMap { std::move(scores), templ.size() });
    }
    return atlas;
}

TemplateHit RecognitionBridge::query_best(
    const RecognitionScoreAtlas& atlas,
    const cv::Rect& requested_roi,
    const TemplateSpec& spec) const
{
    TemplateHit hit { spec.name, spec.node_type, spec.display_name };
    const TemplateScoreMap* map = score_map_for(atlas, spec);
    if (!map) {
        return hit;
    }
    const cv::Rect search = score_search_rect(atlas, requested_roi, map->template_size);
    if (search.empty()) {
        return hit;
    }
    double min_value = 0.0;
    double max_value = 0.0;
    cv::Point min_location;
    cv::Point max_location;
    cv::minMaxLoc(map->scores(search), &min_value, &max_value, &min_location, &max_location);
    const cv::Point top_left = max_location + search.tl() + atlas.source_roi.tl();
    hit.rect = cv::Rect(top_left, map->template_size);
    hit.score = max_value;
    return hit;
}

TemplateHit RecognitionBridge::query_best_near(
    const RecognitionScoreAtlas& atlas,
    const cv::Rect& requested_roi,
    const TemplateSpec& spec,
    cv::Point2f expected_center,
    double center_tolerance) const
{
    TemplateHit hit { spec.name, spec.node_type, spec.display_name };
    if (center_tolerance < 0.0) {
        return hit;
    }
    const TemplateScoreMap* map = score_map_for(atlas, spec);
    if (!map) {
        return hit;
    }
    const cv::Rect search = score_search_rect(atlas, requested_roi, map->template_size);
    if (search.empty()) {
        return hit;
    }

    const double tolerance_squared = center_tolerance * center_tolerance;
    double best_score = -std::numeric_limits<double>::infinity();
    cv::Point best_location;
    bool found = false;
    for (int y = search.y; y < search.y + search.height; ++y) {
        for (int x = search.x; x < search.x + search.width; ++x) {
            const cv::Point2f candidate_center(
                atlas.source_roi.x + x + map->template_size.width * 0.5F,
                atlas.source_roi.y + y + map->template_size.height * 0.5F);
            const cv::Point2f delta = candidate_center - expected_center;
            if (delta.dot(delta) > tolerance_squared) {
                continue;
            }
            const double score = map->scores.at<float>(y, x);
            if (!found || score > best_score) {
                found = true;
                best_score = score;
                best_location = { x, y };
            }
        }
    }
    if (!found) {
        return hit;
    }
    hit.rect = cv::Rect(atlas.source_roi.tl() + best_location, map->template_size);
    hit.score = best_score;
    return hit;
}

std::vector<TemplateHit> RecognitionBridge::query_multi(
    const RecognitionScoreAtlas& atlas,
    const cv::Rect& requested_roi,
    const TemplateSpec& spec,
    double threshold,
    int suppression_radius) const
{
    std::vector<TemplateHit> hits;
    const TemplateScoreMap* map = score_map_for(atlas, spec);
    if (!map) {
        return hits;
    }
    const cv::Rect search = score_search_rect(atlas, requested_roi, map->template_size);
    if (search.empty()) {
        return hits;
    }
    cv::Mat scores = map->scores(search).clone();
    const int radius =
        suppression_radius > 0 ? suppression_radius : std::max(map->template_size.width, map->template_size.height) / 2;
    while (true) {
        double min_value = 0.0;
        double max_value = 0.0;
        cv::Point min_location;
        cv::Point max_location;
        cv::minMaxLoc(scores, &min_value, &max_value, &min_location, &max_location);
        if (max_value < threshold) {
            break;
        }
        const cv::Point atlas_location = max_location + search.tl();
        hits.push_back(
            { spec.name,
              spec.node_type,
              spec.display_name,
              cv::Rect(atlas.source_roi.tl() + atlas_location, map->template_size),
              max_value });
        const int left = std::max(0, max_location.x - radius);
        const int top = std::max(0, max_location.y - radius);
        const cv::Rect suppress(
            left,
            top,
            std::min(scores.cols - left, radius * 2 + 1),
            std::min(scores.rows - top, radius * 2 + 1));
        scores(suppress).setTo(-1.0F);
    }
    return hits;
}

RecognitionBridge::OcrMatch RecognitionBridge::match_ocr_text(
    const std::string& text,
    double minimum_similarity,
    double minimum_margin,
    int short_exact_length) const
{
    OcrMatch output;
    const std::u32string input_codepoints = normalize_chinese(text).first;
    if (input_codepoints.empty()) {
        return output;
    }

    const OcrLabel* best = nullptr;
    const OcrLabel* second = nullptr;
    for (const auto& label : m_ocr_labels) {
        const double similarity = normalized_similarity(input_codepoints, label.codepoints);
        if (!best || similarity > output.similarity) {
            second = best;
            output.runner_up_similarity = output.similarity;
            best = &label;
            output.similarity = similarity;
        }
        else if (!second || similarity > output.runner_up_similarity) {
            second = &label;
            output.runner_up_similarity = similarity;
        }
    }
    if (!best) {
        return output;
    }
    output.matched_name = best->display_name;
    output.node_type = best->node_type;
    output.runner_up_name = second ? second->display_name : std::string();
    output.exact = input_codepoints == best->codepoints;
    const bool short_label = static_cast<int>(best->codepoints.size()) <= std::max(0, short_exact_length);
    output.accepted = output.exact || (!short_label && output.similarity >= minimum_similarity &&
                                       output.similarity - output.runner_up_similarity >= minimum_margin);
    return output;
}

OcrHit RecognitionBridge::match_ocr_hit(
    OcrHit hit,
    double minimum_similarity,
    double minimum_margin,
    int short_exact_length) const
{
    const OcrMatch match = match_ocr_text(hit.raw_text, minimum_similarity, minimum_margin, short_exact_length);
    hit.matched_name = match.matched_name;
    hit.runner_up_name = match.runner_up_name;
    hit.node_type = match.accepted ? match.node_type : std::string();
    hit.similarity = match.similarity;
    hit.runner_up_similarity = match.runner_up_similarity;
    hit.exact_match = match.exact;
    return hit;
}

std::vector<OcrHit> RecognitionBridge::recognize_row(const cv::Mat& image, const cv::Rect& requested_roi) const
{
    std::vector<OcrHit> hits;
    if (image.empty()) {
        return hits;
    }
    const cv::Rect roi = clamp_roi(requested_roi, image.size());
    if (roi.empty()) {
        return hits;
    }

    asst::OCRer analyzer(image, asst::Rect(roi.x, roi.y, roi.width, roi.height));
    analyzer.set_without_det(false);
    analyzer.set_use_raw(true);
    const auto results = analyzer.analyze();
    if (!results) {
        return hits;
    }
    for (const auto& result : *results) {
        hits.push_back(
            { result.text,
              normalize_chinese(result.text).second,
              {},
              {},
              {},
              cv::Rect(result.rect.x, result.rect.y, result.rect.width, result.rect.height),
              result.score,
              0.0,
              0.0,
              false });
    }
    return hits;
}

cv::Size RecognitionBridge::visual_size_for(const std::string& node_type) const
{
    for (const auto& spec : m_ordinary_templates) {
        if (spec.node_type == node_type) {
            return spec.visual_size;
        }
    }
    for (const auto& spec : m_special_templates) {
        if (spec.node_type == node_type) {
            return spec.visual_size;
        }
    }
    return {};
}

std::string RecognitionBridge::display_name_for(const std::string& node_type) const
{
    for (const auto& spec : m_ordinary_templates) {
        if (spec.node_type == node_type) {
            return spec.display_name;
        }
    }
    for (const auto& spec : m_special_templates) {
        if (spec.node_type == node_type) {
            return spec.display_name;
        }
    }
    return {};
}

}
