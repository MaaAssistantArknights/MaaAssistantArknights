#pragma once

#include "Vision/Roguelike/BlackFlow/GridReconstructor.h"
#include "Vision/Roguelike/BlackFlow/PerceptionNode.h"
#include "Vision/Roguelike/BlackFlow/RecognitionBridge.h"
#include <opencv2/core.hpp>
#include <optional>
#include <string>
#include <vector>

namespace asst::blackflow::perception
{

enum class GridRefinementMode
{
    Legacy,
    GuardRing,
    FixedGrid
};

inline const char* to_string(GridRefinementMode value)
{
    switch (value) {
    case GridRefinementMode::GuardRing:
        return "guard-ring";
    case GridRefinementMode::FixedGrid:
        return "fixed-grid";
    default:
        return "legacy";
    }
}

struct GridCandidateDiagnostic
{
    int shift_row = 0;
    int shift_column = 0;
    double origin_x = 0.0;
    double origin_y = 0.0;
    double score = 0.0;
    double inside_score = 0.0;
    double guard_score = 0.0;
    int inside_present = 0;
    int guard_present = 0;
};

struct NodeDetectorConfig
{
    cv::Rect map_roi { 79, 95, 1022, 504 };
    GridSearchConfig grid;
    double seed_empty_threshold = 0.80;
    double cell_empty_threshold = 0.50;
    double large_type_threshold = 0.55;
    double large_center_tolerance = 14.0;
    int cell_roi_size = 44;
    int large_roi_offset_x = -40;
    int large_roi_offset_y = -38;
    int large_roi_width = 84;
    int large_roi_height = 78;
    double bright_delta_threshold = 14.0;
    double current_marker_threshold = 0.60;
    double current_marker_grid_tolerance = 58.0;
    std::size_t minimum_anchor_count = 3;
    double maximum_grid_residual = 8.0;
    GridRefinementMode refinement_mode = GridRefinementMode::FixedGrid;
    int guard_ring_shift_radius = 1;
    double guard_ring_outside_weight = 1.25;
    double guard_ring_min_score_gain = 0.15;
    std::size_t guard_ring_minimum_anchor_count = 2;
    int fixed_grid_translation_limit = 4;
    int empty_multi_suppression_radius = 12;
    double fixed_grid_hit_tolerance = 25.0;
    int ocr_column_width = 77;
    int ocr_row_center_offset_y = 29;
    int ocr_row_height = 40;
    double ocr_grid_tolerance = 25.0;
    int ocr_merge_max_gap = 16;
    double ocr_merge_min_vertical_overlap = 0.50;
    double ocr_merge_max_center_y_delta = 8.0;
    double ocr_similarity_threshold = 0.72;
    double ocr_similarity_margin = 0.12;
    int ocr_short_exact_length = 2;
};

struct NodeDetectionResult
{
    GridGeometry grid;
    GridGeometry seed_grid;
    std::vector<cv::Point2f> anchors;
    std::vector<Node> nodes;
    std::vector<OcrHit> ocr_diagnostics;
    std::vector<GridCandidateDiagnostic> grid_candidates;
    GridRefinementMode refinement_mode = GridRefinementMode::Legacy;
    int selected_shift_row = 0;
    int selected_shift_column = 0;
    double selected_grid_score = 0.0;
    int observed_frames = 1;
    int current_marker_node_id = -1;
    double current_marker_score = 0.0;
    bool map_valid = false;
    std::string rejection_reason;
};

class NodeDetector
{
public:
    NodeDetector(const RecognitionBridge& bridge, NodeDetectorConfig config);
    NodeDetectionResult detect(const cv::Mat& image, int rows, int columns) const;
    cv::Mat draw_overlay(const cv::Mat& image, const NodeDetectionResult& result) const;

    static std::optional<GridGeometry> fixed_grid(int rows, int columns);

private:
    struct CellEvaluation
    {
        Node node;
        double presence_score = 0.0;
    };

    static cv::Rect centered_roi(cv::Point2f center, int width, int height, const cv::Size& bounds);
    static double radial_ring_score(
        const cv::Mat& gray,
        cv::Point2f center,
        double inner_radius,
        double ring_min,
        double ring_max);
    static double brightness_delta(const cv::Mat& gray, cv::Point2f center, NodeKind kind);
    static GridGeometry translate_grid(const GridGeometry& seed, double offset_x, double offset_y);
    static double median(std::vector<double> values);
    CellEvaluation evaluate_cell(
        const cv::Mat& gray,
        cv::Point2f predicted,
        const std::optional<TemplateHit>& empty,
        const std::optional<OcrHit>& ocr,
        const std::optional<TemplateHit>& special,
        int row,
        int column,
        int id) const;

    const RecognitionBridge& m_bridge;
    NodeDetectorConfig m_config;
};

}
