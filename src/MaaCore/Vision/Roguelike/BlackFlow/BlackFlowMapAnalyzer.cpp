#include "Vision/Roguelike/BlackFlow/BlackFlowMapAnalyzer.h"

#include <chrono>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace asst::blackflow::perception
{
namespace
{
nlohmann::json read_json(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Cannot open JSON: " + path.string());
    }
    nlohmann::json output;
    input >> output;
    return output;
}

NodeDetectorConfig parse_node_config(const nlohmann::json& json)
{
    NodeDetectorConfig config;
    const auto roi = json.at("map_roi");
    config.map_roi = cv::Rect(roi.at(0).get<int>(), roi.at(1).get<int>(), roi.at(2).get<int>(), roi.at(3).get<int>());
    config.grid.spacing_min = json.value("spacing_min", config.grid.spacing_min);
    config.grid.spacing_max = json.value("spacing_max", config.grid.spacing_max);
    config.grid.spacing_step = json.value("spacing_step", config.grid.spacing_step);
    config.grid.spacing_hint = json.value("spacing_hint", config.grid.spacing_hint);
    config.seed_empty_threshold = json.value("seed_empty_threshold", config.seed_empty_threshold);
    config.cell_empty_threshold = json.value("cell_empty_threshold", config.cell_empty_threshold);
    config.large_type_threshold = json.value("large_type_threshold", config.large_type_threshold);
    config.large_center_tolerance = json.value("large_center_tolerance", config.large_center_tolerance);
    config.cell_roi_size = json.value("cell_roi_size", config.cell_roi_size);
    config.large_roi_offset_x = json.value("large_roi_offset_x", config.large_roi_offset_x);
    config.large_roi_offset_y = json.value("large_roi_offset_y", config.large_roi_offset_y);
    config.large_roi_width = json.value("large_roi_width", config.large_roi_width);
    config.large_roi_height = json.value("large_roi_height", config.large_roi_height);
    config.bright_delta_threshold = json.value("bright_delta_threshold", config.bright_delta_threshold);
    config.current_marker_threshold = json.value("current_marker_threshold", config.current_marker_threshold);
    config.current_marker_grid_tolerance =
        json.value("current_marker_grid_tolerance", config.current_marker_grid_tolerance);
    config.minimum_anchor_count = json.value("minimum_anchor_count", config.minimum_anchor_count);
    config.maximum_grid_residual = json.value("maximum_grid_residual", config.maximum_grid_residual);
    config.refinement_mode = GridRefinementMode::FixedGrid;
    config.guard_ring_shift_radius = json.value("guard_ring_shift_radius", config.guard_ring_shift_radius);
    config.guard_ring_outside_weight = json.value("guard_ring_outside_weight", config.guard_ring_outside_weight);
    config.guard_ring_min_score_gain = json.value("guard_ring_min_score_gain", config.guard_ring_min_score_gain);
    config.guard_ring_minimum_anchor_count =
        json.value("guard_ring_minimum_anchor_count", config.guard_ring_minimum_anchor_count);
    config.fixed_grid_translation_limit =
        json.value("fixed_grid_translation_limit", config.fixed_grid_translation_limit);
    config.empty_multi_suppression_radius =
        json.value("empty_multi_suppression_radius", config.empty_multi_suppression_radius);
    config.fixed_grid_hit_tolerance = json.value("fixed_grid_hit_tolerance", config.fixed_grid_hit_tolerance);
    config.ocr_column_width = json.value("ocr_column_width", config.ocr_column_width);
    config.ocr_row_center_offset_y = json.value("ocr_row_center_offset_y", config.ocr_row_center_offset_y);
    config.ocr_row_height = json.value("ocr_row_height", config.ocr_row_height);
    config.ocr_grid_tolerance = json.value("ocr_grid_tolerance", config.ocr_grid_tolerance);
    config.ocr_merge_max_gap = json.value("ocr_merge_max_gap", config.ocr_merge_max_gap);
    config.ocr_merge_min_vertical_overlap =
        json.value("ocr_merge_min_vertical_overlap", config.ocr_merge_min_vertical_overlap);
    config.ocr_merge_max_center_y_delta =
        json.value("ocr_merge_max_center_y_delta", config.ocr_merge_max_center_y_delta);
    config.ocr_similarity_threshold = json.value("ocr_similarity_threshold", config.ocr_similarity_threshold);
    config.ocr_similarity_margin = json.value("ocr_similarity_margin", config.ocr_similarity_margin);
    config.ocr_short_exact_length = json.value("ocr_short_exact_length", config.ocr_short_exact_length);
    return config;
}
} // namespace

FloorProfile floor_profile(int floor)
{
    switch (floor) {
    case 1:
        return { 1, 3, 5 };
    case 2:
        return { 2, 4, 5 };
    case 3:
        return { 3, 5, 7 };
    case 4:
        return { 4, 5, 8 };
    case 5:
        return { 5, 5, 10 };
    default:
        throw std::runtime_error("floor must be an integer from 1 to 5");
    }
}

bool BlackFlowMapAnalyzer::load(const std::filesystem::path& resource_root, std::string& error)
{
    try {
        const auto template_manifest = resource_root / "templates" / "manifest.json";
        const auto node_config = resource_root / "edge.json";
        const auto model_manifest = resource_root / "corridor_net.runtime.json";
        if (!std::filesystem::is_regular_file(node_config)) {
            error = "BlackFlow map node config does not exist: " + node_config.string();
            return false;
        }
        if (!m_bridge.load(template_manifest, error)) {
            return false;
        }
        m_node_detector = std::make_unique<NodeDetector>(m_bridge, parse_node_config(read_json(node_config)));
        if (!m_edge_detector.load(model_manifest, error)) {
            m_node_detector.reset();
            return false;
        }
        m_loaded = true;
        return true;
    }
    catch (const std::exception& exception) {
        error = "BlackFlow map perception initialization failed: " + std::string(exception.what());
        m_node_detector.reset();
        m_loaded = false;
        return false;
    }
    catch (...) {
        error = "BlackFlow map perception initialization failed: unknown exception";
        m_node_detector.reset();
        m_loaded = false;
        return false;
    }
}

MapRecognitionResult BlackFlowMapAnalyzer::recognize(const cv::Mat& image, int floor) const
{
    MapRecognitionResult result;
    result.floor = floor;
    std::chrono::steady_clock::time_point normalization_start;
    std::chrono::steady_clock::time_point recognition_start;
    bool normalization_started = false;
    bool normalization_finished = false;
    bool recognition_started = false;
    try {
        if (!m_loaded || m_node_detector == nullptr) {
            throw std::runtime_error("BlackFlow map perception is not loaded");
        }
        if (image.empty() || image.type() != CV_8UC3) {
            throw std::runtime_error("BlackFlow map perception requires a non-empty BGR8 image");
        }
        const FloorProfile profile = floor_profile(floor);
        result.rows = profile.rows;
        result.columns = profile.columns;
        result.captured_bgr = image.clone();

        normalization_start = std::chrono::steady_clock::now();
        normalization_started = true;
        FrameNormalizationInfo normalization;
        std::string normalization_error;
        if (!m_normalizer.normalize(image, result.normalized_bgr, normalization, normalization_error)) {
            throw std::runtime_error("Image normalization failed: " + normalization_error);
        }
        if (result.normalized_bgr.data == image.data) {
            result.normalized_bgr = image.clone();
        }
        result.normalization_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                      std::chrono::steady_clock::now() - normalization_start)
                                      .count();
        normalization_finished = true;

        recognition_start = std::chrono::steady_clock::now();
        recognition_started = true;
        result.node_detection = m_node_detector->detect(result.normalized_bgr, profile.rows, profile.columns);
        result.node_overlay_bgr = m_node_detector->draw_overlay(result.normalized_bgr, result.node_detection);
        if (!result.node_detection.map_valid) {
            throw std::runtime_error("Node map rejected: " + result.node_detection.rejection_reason);
        }
        result.edge_detection = m_edge_detector.detect(
            std::vector<cv::Mat> { result.normalized_bgr },
            result.node_detection.nodes,
            profile.rows,
            profile.columns);
        result.edge_overlay_bgr =
            m_edge_detector.draw_overlay(result.normalized_bgr, result.node_detection.nodes, result.edge_detection);
        if (!result.edge_detection.error.empty()) {
            throw std::runtime_error(result.edge_detection.error);
        }
        result.ok = true;
    }
    catch (const std::exception& exception) {
        result.error = exception.what();
    }
    catch (...) {
        result.error = "BlackFlow map recognition failed: unknown exception";
    }
    if (normalization_started && !normalization_finished) {
        result.normalization_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                      std::chrono::steady_clock::now() - normalization_start)
                                      .count();
    }
    if (recognition_started) {
        result.recognition_us =
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - recognition_start)
                .count();
    }
    return result;
}

bool BlackFlowMapAnalyzer::loaded() const noexcept
{
    return m_loaded;
}
} // namespace asst::blackflow::perception
