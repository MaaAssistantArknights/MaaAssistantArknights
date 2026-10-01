#include "Vision/Roguelike/BlackFlow/BlackFlowMapAnalyzer.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "Task/Roguelike/BlackFlow/BlackFlowMapTemplateMatcher.h"
#include "Utils/Logger.hpp"

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
    config.marker_grid_tolerance = json.value("marker_grid_tolerance", config.marker_grid_tolerance);
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

const MapTemplate* match_recognized_map(const MapRecognitionResult& result)
{
    MapObservationBatch observation;
    observation.floor = result.floor;
    std::unordered_map<int, GridPosition> positions;
    for (const auto& node : result.node_detection.nodes) {
        positions.emplace(node.id, GridPosition { node.row, node.column });
    }
    for (const auto& edge : result.edge_detection.edges) {
        if (edge.connected) {
            ObservedEdge imported;
            imported.first = positions.at(edge.node_a);
            imported.second = positions.at(edge.node_b);
            imported.knowledge = EdgeKnowledge::Confirmed;
            observation.edges.emplace_back(std::move(imported));
        }
    }
    const auto matched = match_map_template(observation, BlackFlowMapTemplates.templates());
    return matched;
}
} // namespace

bool BlackFlowMapAnalyzer::load(
    const std::filesystem::path& template_manifest_path,
    const std::filesystem::path& edge_config_path,
    const std::filesystem::path& runtime_manifest_path,
    std::string& error)
{
    try {
        if (!std::filesystem::is_regular_file(edge_config_path)) {
            error = "BlackFlow map node config does not exist: " + edge_config_path.string();
            return false;
        }
        if (!m_bridge.load(template_manifest_path, error)) {
            return false;
        }
        m_node_detector = std::make_unique<NodeDetector>(m_bridge, parse_node_config(read_json(edge_config_path)));
        if (!m_edge_detector.load(runtime_manifest_path, error)) {
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

bool trim_empty_map_borders(MapRecognitionResult& result)
{
    auto& detection = result.node_detection;
    int top = result.rows;
    int left = result.columns;
    int bottom = -1;
    int right = -1;
    for (const auto& node : detection.nodes) {
        if (node.exists) {
            top = std::min(top, node.row);
            left = std::min(left, node.column);
            bottom = std::max(bottom, node.row);
            right = std::max(right, node.column);
        }
    }
    if (bottom < top || right < left) {
        result.error = "map recognition produced no existing nodes";
        return false;
    }
    const int rows = bottom - top + 1;
    const int columns = right - left + 1;
    std::unordered_map<int, int> ids;
    std::vector<Node> nodes;
    nodes.reserve(static_cast<std::size_t>(rows * columns));
    for (const auto& original : detection.nodes) {
        if (original.row < top || original.row > bottom || original.column < left || original.column > right) {
            continue;
        }
        Node node = original;
        node.row -= top;
        node.column -= left;
        node.id = node.row * columns + node.column;
        ids.emplace(original.id, node.id);
        nodes.emplace_back(std::move(node));
    }
    std::vector<Edge> edges;
    for (const auto& original : result.edge_detection.edges) {
        const auto first = ids.find(original.node_a);
        const auto second = ids.find(original.node_b);
        if (first == ids.end() || second == ids.end()) {
            continue;
        }
        Edge edge = original;
        edge.id = static_cast<int>(edges.size());
        edge.node_a = first->second;
        edge.node_b = second->second;
        edges.emplace_back(std::move(edge));
    }
    const auto marker = ids.find(detection.current_marker_node_id);
    detection.current_marker_node_id = marker == ids.end() ? -1 : marker->second;
    auto& inferred = result.edge_detection.inferred_existing_node_ids;
    std::erase_if(inferred, [&](int id) { return !ids.contains(id); });
    for (int& id : inferred) {
        id = ids.at(id);
    }
    const auto update_grid = [&](GridGeometry& grid) {
        // 两份网格分别裁取原坐标，保留初始网格与平移后网格的差异。
        std::vector<cv::Point2f> centers;
        centers.reserve(static_cast<std::size_t>(rows * columns));
        for (int row = top; row <= bottom; ++row) {
            const auto first = grid.centers.begin() + row * grid.columns + left;
            centers.insert(centers.end(), first, first + columns);
        }
        grid.rows = rows;
        grid.columns = columns;
        grid.origin_x += left * grid.spacing_x;
        grid.origin_y += top * grid.spacing_y;
        grid.centers = std::move(centers);
    };
    update_grid(detection.grid);
    update_grid(detection.seed_grid);
    detection.nodes = std::move(nodes);
    result.edge_detection.edges = std::move(edges);
    result.rows = rows;
    result.columns = columns;
    return true;
}

MapRecognitionResult BlackFlowMapAnalyzer::recognize(const cv::Mat& image, int floor, bool render_overlay) const
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
        const auto profile = floor_profile(floor);
        if (!profile.has_value()) {
            throw std::runtime_error("floor must be an integer from 1 to 5");
        }
        result.rows = profile->rows;
        result.columns = profile->columns;
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
        bool recognized = false;
        int matching_grids = 0;
        for (const auto& grid : floor_profiles(floor)) {
            MapRecognitionResult candidate;
            candidate.floor = floor;
            candidate.rows = grid.rows;
            candidate.columns = grid.columns;
            candidate.node_detection = m_node_detector->detect(result.normalized_bgr, grid.rows, grid.columns);
            candidate.edge_detection = m_edge_detector.detect(
                std::vector<cv::Mat> { result.normalized_bgr },
                candidate.node_detection.nodes,
                grid.rows,
                grid.columns);
            if (!candidate.edge_detection.error.empty()) {
                result.error = candidate.edge_detection.error;
                LogInfo << __FUNCTION__ << "Map grid" << grid.rows << grid.columns << "recognition failed"
                        << candidate.edge_detection.error;
                continue;
            }
            if (floor == 5 && !trim_empty_map_borders(candidate)) {
                result.error = candidate.error;
                continue;
            }
            // 先检查地图是否有效，再参与候选选择和模板匹配计数。
            if (!is_supported_floor_grid(floor, candidate.rows, candidate.columns)) {
                candidate.error = "map recognition produced unsupported grid dimensions";
            }
            else if (candidate.node_detection.current_marker_node_id < 0) {
                candidate.error = "map recognition did not locate the current marker";
            }
            if (!candidate.error.empty()) {
                result.error = candidate.error;
                LogInfo << __FUNCTION__ << "Map grid" << grid.rows << grid.columns << "recognition failed"
                        << candidate.error;
                continue;
            }
            const auto* matched = floor == 5 ? match_recognized_map(candidate) : nullptr;
            if (matched != nullptr) {
                ++matching_grids;
            }
            if (!recognized || (matched != nullptr && matching_grids == 1)) {
                result.rows = candidate.rows;
                result.columns = candidate.columns;
                result.node_detection = std::move(candidate.node_detection);
                result.edge_detection = std::move(candidate.edge_detection);
                recognized = true;
            }
        }
        // 无匹配或多份网格均匹配时保留识别地图，但不使用本次匹配补全终点。
        result.allow_exit_supplement = floor != 5 || matching_grids == 1;
        if (recognized) {
            result.error.clear();
            result.ok = true;
            if (render_overlay) {
                result.overlay_bgr = draw_overlay(result);
            }
        }
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

cv::Mat BlackFlowMapAnalyzer::draw_overlay(const MapRecognitionResult& result) const
{
    if (result.normalized_bgr.empty() || m_node_detector == nullptr) {
        return result.captured_bgr.clone();
    }
    cv::Mat overlay = m_node_detector->draw_overlay(result.normalized_bgr, result.node_detection);
    return m_edge_detector.draw_overlay(overlay, result.node_detection.nodes, result.edge_detection);
}

bool BlackFlowMapAnalyzer::loaded() const noexcept
{
    return m_loaded;
}
} // namespace asst::blackflow::perception
