#include "Vision/Roguelike/BlackFlow/NodeDetector.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <opencv2/imgproc.hpp>
#include <sstream>
#include <stdexcept>

namespace asst::blackflow::perception
{
namespace
{

double mean_annulus(const cv::Mat& gray, cv::Point2f center, double min_radius, double max_radius)
{
    double sum = 0.0;
    std::size_t count = 0;
    const int min_x = std::max(0, static_cast<int>(std::floor(center.x - max_radius)));
    const int max_x = std::min(gray.cols - 1, static_cast<int>(std::ceil(center.x + max_radius)));
    const int min_y = std::max(0, static_cast<int>(std::floor(center.y - max_radius)));
    const int max_y = std::min(gray.rows - 1, static_cast<int>(std::ceil(center.y + max_radius)));
    const double min2 = min_radius * min_radius;
    const double max2 = max_radius * max_radius;
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            const double dx = x - center.x;
            const double dy = y - center.y;
            const double radius2 = dx * dx + dy * dy;
            if (radius2 >= min2 && radius2 <= max2) {
                sum += gray.at<std::uint8_t>(y, x);
                ++count;
            }
        }
    }
    return count ? sum / static_cast<double>(count) : 0.0;
}

GridGeometry make_grid(int rows, int columns, double origin_x, double origin_y, double spacing_x, double spacing_y)
{
    GridGeometry grid;
    grid.rows = rows;
    grid.columns = columns;
    grid.origin_x = origin_x;
    grid.origin_y = origin_y;
    grid.spacing_x = spacing_x;
    grid.spacing_y = spacing_y;
    grid.residual_x = 0.0;
    grid.residual_y = 0.0;
    grid.centers.reserve(static_cast<std::size_t>(rows * columns));
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            grid.centers.emplace_back(
                static_cast<float>(origin_x + column * spacing_x),
                static_cast<float>(origin_y + row * spacing_y));
        }
    }
    return grid;
}

std::optional<std::size_t> nearest_cell(
    const GridGeometry& grid,
    cv::Point2f point,
    double tolerance,
    std::optional<int> required_row = std::nullopt)
{
    double best_distance = std::numeric_limits<double>::infinity();
    std::optional<std::size_t> best_index;
    for (int row = 0; row < grid.rows; ++row) {
        if (required_row && row != *required_row) {
            continue;
        }
        for (int column = 0; column < grid.columns; ++column) {
            const std::size_t index = static_cast<std::size_t>(row * grid.columns + column);
            const double distance =
                required_row ? std::abs(static_cast<double>(point.x) - static_cast<double>(grid.centers[index].x))
                             : cv::norm(point - grid.centers[index]);
            if (distance < best_distance) {
                best_distance = distance;
                best_index = index;
            }
        }
    }
    if (!best_index || best_distance > tolerance) {
        return std::nullopt;
    }
    return best_index;
}

double vertical_overlap_ratio(const cv::Rect& left, const cv::Rect& right)
{
    const int overlap = std::max(0, std::min(left.y + left.height, right.y + right.height) - std::max(left.y, right.y));
    const int minimum_height = std::min(left.height, right.height);
    return minimum_height > 0 ? static_cast<double>(overlap) / minimum_height : 0.0;
}

bool can_merge_ocr_hits(
    const OcrHit& left,
    const OcrHit& right,
    int maximum_gap,
    double minimum_vertical_overlap,
    double maximum_center_y_delta)
{
    const int horizontal_gap = right.rect.x - (left.rect.x + left.rect.width);
    if (horizontal_gap < 0 || horizontal_gap > maximum_gap) {
        return false;
    }
    const double center_y_delta =
        std::abs(static_cast<double>(left.center().y) - static_cast<double>(right.center().y));
    return vertical_overlap_ratio(left.rect, right.rect) >= minimum_vertical_overlap ||
           center_y_delta <= maximum_center_y_delta;
}

void merge_ocr_hit(OcrHit& target, const OcrHit& fragment)
{
    target.raw_text += fragment.raw_text;
    target.normalized_text += fragment.normalized_text;
    target.rect = target.rect | fragment.rect;
    target.score = std::min(target.score, fragment.score);
    target.matched_name.clear();
    target.runner_up_name.clear();
    target.node_type.clear();
    target.similarity = 0.0;
    target.runner_up_similarity = 0.0;
    target.exact_match = false;
}

}

NodeDetector::NodeDetector(const RecognitionBridge& bridge, NodeDetectorConfig config) :
    m_bridge(bridge),
    m_config(std::move(config))
{
}

cv::Rect NodeDetector::centered_roi(cv::Point2f center, int width, int height, const cv::Size& bounds)
{
    const int x = static_cast<int>(std::lround(center.x)) - width / 2;
    const int y = static_cast<int>(std::lround(center.y)) - height / 2;
    return cv::Rect(x, y, width, height) & cv::Rect(0, 0, bounds.width, bounds.height);
}

double NodeDetector::radial_ring_score(
    const cv::Mat& gray,
    cv::Point2f center,
    double inner_radius,
    double ring_min,
    double ring_max)
{
    return mean_annulus(gray, center, ring_min, ring_max) - mean_annulus(gray, center, 0.0, inner_radius);
}

double NodeDetector::brightness_delta(const cv::Mat& gray, cv::Point2f center, NodeKind kind)
{
    if (kind == NodeKind::Large) {
        return mean_annulus(gray, center, 27.0, 37.0) - mean_annulus(gray, center, 41.0, 49.0);
    }
    return mean_annulus(gray, center, 6.0, 10.0) - mean_annulus(gray, center, 14.0, 21.0);
}

std::optional<GridGeometry> NodeDetector::fixed_grid(int rows, int columns)
{
    if (rows == 3 && columns == 5) {
        return make_grid(rows, columns, 438.0, 242.0, 101.0, 101.0);
    }
    if (rows == 4 && columns == 5) {
        return make_grid(rows, columns, 438.0, 194.0, 101.0, 101.0);
    }
    if (rows == 5 && columns == 7) {
        return make_grid(rows, columns, 337.5, 144.0, 100.75, 101.0);
    }
    if (rows == 5 && columns == 8) {
        return make_grid(rows, columns, 286.75, 145.0, 100.75, 99.5);
    }
    if (rows == 5 && columns == 10) {
        return make_grid(rows, columns, 121.0, 141.0, 100.75, 100.25);
    }
    return std::nullopt;
}

GridGeometry NodeDetector::translate_grid(const GridGeometry& seed, double offset_x, double offset_y)
{
    GridGeometry translated = seed;
    translated.origin_x += offset_x;
    translated.origin_y += offset_y;
    for (auto& center : translated.centers) {
        center.x += static_cast<float>(offset_x);
        center.y += static_cast<float>(offset_y);
    }
    return translated;
}

double NodeDetector::median(std::vector<double> values)
{
    if (values.empty()) {
        return 0.0;
    }
    std::ranges::sort(values);
    const std::size_t middle = values.size() / 2;
    if (values.size() % 2 != 0) {
        return values[middle];
    }
    return 0.5 * (values[middle - 1] + values[middle]);
}

NodeDetector::CellEvaluation NodeDetector::evaluate_cell(
    const cv::Mat& gray,
    cv::Point2f predicted,
    const std::optional<TemplateHit>& empty,
    const std::optional<OcrHit>& ocr,
    const std::optional<TemplateHit>& special,
    int row,
    int column,
    int id) const
{
    CellEvaluation result;
    Node& node = result.node;
    node.id = id;
    node.row = row;
    node.column = column;
    node.center = predicted;
    node.visual_center = predicted;
    node.ring_score = radial_ring_score(gray, predicted, 3.5, 6.0, 10.0);

    if (empty) {
        node.empty_score = empty->score;
        const cv::Point2f offset = empty->center() - predicted;
        node.empty_offset_x = offset.x;
        node.empty_offset_y = offset.y;
    }
    if (special) {
        node.large_score = special->score;
        const cv::Point2f offset = special->center() - predicted;
        node.large_offset_x = offset.x;
        node.large_offset_y = offset.y;
    }
    else if (ocr) {
        node.large_score = ocr->score;
        node.ocr_raw_text = ocr->raw_text;
        node.ocr_normalized_text = ocr->normalized_text;
        node.ocr_runner_up = ocr->runner_up_name;
        node.ocr_score = ocr->score;
        node.ocr_similarity = ocr->similarity;
        node.ocr_runner_up_similarity = ocr->runner_up_similarity;
        node.ocr_exact_match = ocr->exact_match;
        const cv::Point2f offset = ocr->center() - predicted;
        node.large_offset_x = offset.x;
        node.large_offset_y = offset.y;
    }

    if (special) {
        node.exists = true;
        node.existence_confidence = special->score;
        node.existence_source = "special_template";
        node.kind = NodeKind::Large;
        node.type = special->node_type;
        node.display_name = special->display_name;
        node.confidence = special->score;
        node.visual_center = special->center();
        node.visual_half_width = 0.5F * static_cast<float>(special->rect.width);
        node.visual_half_height = 0.5F * static_cast<float>(special->rect.height);
        node.visual_geometry_from_template = true;
        node.evidence.push_back("maa_special_template_fixed_grid");
    }
    else if (ocr) {
        node.exists = true;
        node.existence_confidence = ocr->score;
        node.existence_source = "ocr";
        node.kind = NodeKind::Large;
        node.type = ocr->node_type;
        node.display_name = ocr->matched_name;
        node.confidence = ocr->score;
        const cv::Size visual_size = m_bridge.visual_size_for(node.type);
        if (!visual_size.empty()) {
            node.visual_half_width = 0.5F * static_cast<float>(visual_size.width);
            node.visual_half_height = 0.5F * static_cast<float>(visual_size.height);
        }
        else {
            node.visual_half_width = 25.0F;
            node.visual_half_height = 20.0F;
        }
        node.evidence.push_back(ocr->exact_match ? "maa_row_ocr_exact" : "maa_row_ocr_chinese_similarity");
        if (empty) {
            node.evidence.push_back("ocr_overrode_empty_template");
        }
    }
    else if (empty) {
        node.exists = true;
        node.existence_confidence = empty->score;
        node.existence_source = "empty_template";
        node.kind = NodeKind::Small;
        node.type = "empty";
        node.display_name.clear();
        node.confidence = empty->score;
        node.visual_center = empty->center();
        node.visual_half_width = 0.5F * static_cast<float>(empty->rect.width);
        node.visual_half_height = 0.5F * static_cast<float>(empty->rect.height);
        node.visual_geometry_from_template = true;
        node.evidence.push_back("maa_empty_multimatcher_fixed_grid");
    }
    else {
        node.kind = NodeKind::Unknown;
        node.type = "null";
        node.display_name.clear();
        node.confidence = 0.0;
        node.visual_half_width = 10.0F;
        node.visual_half_height = 10.0F;
        node.evidence.push_back("fixed_grid_null_without_valid_evidence");
    }

    node.brightness_delta = brightness_delta(gray, predicted, node.kind);
    if (std::abs(node.brightness_delta) < 2.0) {
        node.state = VisualState::Unknown;
    }
    else {
        node.state = node.brightness_delta >= m_config.bright_delta_threshold ? VisualState::Bright : VisualState::Dim;
    }
    node.presence_frame_hits = node.exists ? 1 : 0;
    node.empty_frame_hits = node.kind == NodeKind::Small ? 1 : 0;
    node.large_frame_hits = node.kind == NodeKind::Large ? 1 : 0;
    result.presence_score = node.existence_confidence;
    return result;
}

NodeDetectionResult NodeDetector::detect(const cv::Mat& image, int rows, int columns) const
{
    NodeDetectionResult output;
    output.refinement_mode = GridRefinementMode::FixedGrid;
    if (image.empty()) {
        throw std::invalid_argument("Node detection input is empty");
    }

    const auto baseline = fixed_grid(rows, columns);
    if (!baseline) {
        throw std::invalid_argument("No fixed grid is defined for the requested floor dimensions");
    }
    output.seed_grid = *baseline;

    const cv::Rect map_roi = m_config.map_roi & cv::Rect(0, 0, image.cols, image.rows);
    const RecognitionScoreAtlas atlas = m_bridge.build_score_atlas(image, map_roi);
    const std::vector<TemplateHit> empty_hits = m_bridge.query_multi(
        atlas,
        map_roi,
        m_bridge.empty_template(),
        m_config.cell_empty_threshold,
        m_config.empty_multi_suppression_radius);

    const std::vector<TemplateHit> translation_empty_hits = m_bridge.query_multi(
        atlas,
        map_roi,
        m_bridge.empty_template(),
        m_config.seed_empty_threshold,
        m_config.empty_multi_suppression_radius);
    std::vector<std::optional<TemplateHit>> seed_empty(output.seed_grid.centers.size());
    for (const auto& hit : translation_empty_hits) {
        const auto index = nearest_cell(output.seed_grid, hit.center(), m_config.fixed_grid_hit_tolerance);
        if (!index) {
            continue;
        }
        if (!seed_empty[*index] || hit.score > seed_empty[*index]->score) {
            seed_empty[*index] = hit;
        }
    }

    std::vector<double> offsets_x;
    std::vector<double> offsets_y;
    for (std::size_t index = 0; index < seed_empty.size(); ++index) {
        if (!seed_empty[index]) {
            continue;
        }
        const cv::Point2f offset = seed_empty[index]->center() - output.seed_grid.centers[index];
        offsets_x.push_back(offset.x);
        offsets_y.push_back(offset.y);
    }
    const double translation_x = std::clamp(
        median(std::move(offsets_x)),
        -static_cast<double>(m_config.fixed_grid_translation_limit),
        static_cast<double>(m_config.fixed_grid_translation_limit));
    const double translation_y = std::clamp(
        median(std::move(offsets_y)),
        -static_cast<double>(m_config.fixed_grid_translation_limit),
        static_cast<double>(m_config.fixed_grid_translation_limit));
    output.grid = translate_grid(output.seed_grid, translation_x, translation_y);

    std::vector<std::optional<TemplateHit>> cell_empty(output.grid.centers.size());
    for (const auto& hit : empty_hits) {
        const auto index = nearest_cell(output.grid, hit.center(), m_config.fixed_grid_hit_tolerance);
        if (!index) {
            continue;
        }
        if (!cell_empty[*index] || hit.score > cell_empty[*index]->score) {
            cell_empty[*index] = hit;
        }
    }
    for (const auto& hit : cell_empty) {
        if (hit) {
            output.anchors.push_back(hit->center());
        }
    }

    std::vector<std::optional<OcrHit>> cell_ocr(output.grid.centers.size());
    const double half_column_width = 0.5 * m_config.ocr_column_width;
    for (int row = 0; row < rows; ++row) {
        const cv::Point2f first = output.grid.centers[static_cast<std::size_t>(row * columns)];
        const cv::Point2f last = output.grid.centers[static_cast<std::size_t>(row * columns + columns - 1)];
        const int left = static_cast<int>(std::lround(first.x - half_column_width));
        const int right = static_cast<int>(std::lround(last.x + half_column_width));
        const int top =
            static_cast<int>(std::lround(first.y + m_config.ocr_row_center_offset_y - 0.5 * m_config.ocr_row_height));
        const cv::Rect row_roi =
            cv::Rect(left, top, right - left, m_config.ocr_row_height) & cv::Rect(0, 0, image.cols, image.rows);

        std::vector<std::vector<OcrHit>> column_fragments(static_cast<std::size_t>(columns));
        for (auto hit : m_bridge.recognize_row(image, row_roi)) {
            output.ocr_diagnostics.push_back(hit);
            if (hit.normalized_text.empty()) {
                continue;
            }
            const auto index = nearest_cell(output.grid, hit.center(), m_config.ocr_grid_tolerance, row);
            if (!index) {
                continue;
            }
            const std::size_t column = *index - static_cast<std::size_t>(row * columns);
            column_fragments[column].push_back(std::move(hit));
        }

        for (int column = 0; column < columns; ++column) {
            auto& fragments = column_fragments[static_cast<std::size_t>(column)];
            std::ranges::sort(fragments, [](const OcrHit& left_hit, const OcrHit& right_hit) {
                if (left_hit.rect.x != right_hit.rect.x) {
                    return left_hit.rect.x < right_hit.rect.x;
                }
                return left_hit.rect.y < right_hit.rect.y;
            });

            std::vector<OcrHit> merged_hits;
            for (const auto& fragment : fragments) {
                if (!merged_hits.empty() && can_merge_ocr_hits(
                                                merged_hits.back(),
                                                fragment,
                                                m_config.ocr_merge_max_gap,
                                                m_config.ocr_merge_min_vertical_overlap,
                                                m_config.ocr_merge_max_center_y_delta)) {
                    merge_ocr_hit(merged_hits.back(), fragment);
                }
                else {
                    merged_hits.push_back(fragment);
                }
            }

            const std::size_t index = static_cast<std::size_t>(row * columns + column);
            for (auto& merged : merged_hits) {
                OcrHit matched = m_bridge.match_ocr_hit(
                    std::move(merged),
                    m_config.ocr_similarity_threshold,
                    m_config.ocr_similarity_margin,
                    m_config.ocr_short_exact_length);
                if (matched.node_type.empty()) {
                    continue;
                }
                if (!cell_ocr[index] || matched.score > cell_ocr[index]->score) {
                    cell_ocr[index] = std::move(matched);
                }
            }
        }
    }

    std::vector<std::optional<TemplateHit>> cell_special(output.grid.centers.size());
    for (const auto& spec : m_bridge.special_templates()) {
        for (const auto& hit : m_bridge.query_multi(atlas, map_roi, spec, spec.threshold, 36)) {
            const auto index = nearest_cell(output.grid, hit.center(), m_config.fixed_grid_hit_tolerance);
            if (!index) {
                continue;
            }
            if (!cell_special[*index] || hit.score > cell_special[*index]->score) {
                cell_special[*index] = hit;
            }
        }
    }
    GridCandidateDiagnostic diagnostic;
    diagnostic.origin_x = output.grid.origin_x;
    diagnostic.origin_y = output.grid.origin_y;
    for (const auto& hit : cell_empty) {
        if (!hit) {
            continue;
        }
        diagnostic.inside_score += hit->score;
        ++diagnostic.inside_present;
    }
    diagnostic.score = diagnostic.inside_score;
    output.grid_candidates.push_back(diagnostic);
    output.selected_grid_score = diagnostic.score;

    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    output.nodes.reserve(output.grid.centers.size());
    int id = 0;
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const std::size_t index = static_cast<std::size_t>(row * columns + column);
            output.nodes.push_back(evaluate_cell(
                                       gray,
                                       output.grid.centers[index],
                                       cell_empty[index],
                                       cell_ocr[index],
                                       cell_special[index],
                                       row,
                                       column,
                                       id++)
                                       .node);
        }
    }

    const TemplateHit marker_hit = m_bridge.query_best(atlas, map_roi, m_bridge.current_marker_template());
    if (marker_hit.score >= m_config.current_marker_threshold && marker_hit.rect.area() > 0) {
        double best_distance = std::numeric_limits<double>::infinity();
        std::size_t best_index = 0;
        for (std::size_t index = 0; index < output.grid.centers.size(); ++index) {
            const double distance = cv::norm(marker_hit.center() - output.grid.centers[index]);
            if (distance < best_distance) {
                best_distance = distance;
                best_index = index;
            }
        }
        if (best_distance <= m_config.current_marker_grid_tolerance) {
            Node& node = output.nodes[best_index];
            node.current_marker = true;
            node.current_marker_score = marker_hit.score;
            node.evidence.push_back("maa_current_marker_score_atlas");
            if (!node.exists || marker_hit.score > node.existence_confidence) {
                node.exists = true;
                node.existence_confidence = marker_hit.score;
                node.existence_source = "current_marker_template";
            }
            if (node.kind == NodeKind::Unknown) {
                node.kind = NodeKind::Small;
                node.type = "empty";
                node.confidence = marker_hit.score;
                node.presence_frame_hits = 1;
                node.empty_frame_hits = 1;
            }
            output.current_marker_node_id = node.id;
            output.current_marker_score = marker_hit.score;
        }
    }
    return output;
}

cv::Mat NodeDetector::draw_overlay(const cv::Mat& image, const NodeDetectionResult& result) const
{
    cv::Mat overlay = image.clone();
    for (const auto& node : result.nodes) {
        const cv::Scalar color = node.current_marker            ? cv::Scalar(255, 0, 255)
                                 : node.kind == NodeKind::Large ? cv::Scalar(255, 180, 0)
                                 : node.kind == NodeKind::Small ? cv::Scalar(0, 220, 0)
                                                                : cv::Scalar(0, 140, 255);
        cv::ellipse(
            overlay,
            node.visual_center,
            cv::Size(
                static_cast<int>(std::lround(node.visual_half_width)),
                static_cast<int>(std::lround(node.visual_half_height))),
            0.0,
            0.0,
            360.0,
            color,
            2,
            cv::LINE_AA);
        cv::circle(overlay, node.visual_center, 2, color, cv::FILLED, cv::LINE_AA);
        std::ostringstream label;
        label << node.row << ',' << node.column << ' ' << node.type << ' ' << to_string(node.state);
        if (node.current_marker) {
            label << " current";
        }
        cv::putText(
            overlay,
            label.str(),
            node.center + cv::Point2f(8.0F, -10.0F),
            cv::FONT_HERSHEY_SIMPLEX,
            0.34,
            color,
            1,
            cv::LINE_AA);
    }
    std::ostringstream refinement;
    refinement << to_string(result.refinement_mode) << " shift=" << result.selected_shift_column << ','
               << result.selected_shift_row;
    cv::putText(
        overlay,
        refinement.str(),
        cv::Point(40, 70),
        cv::FONT_HERSHEY_SIMPLEX,
        0.55,
        cv::Scalar(255, 255, 255),
        1,
        cv::LINE_AA);
    return overlay;
}

}
