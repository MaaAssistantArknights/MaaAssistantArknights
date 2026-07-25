#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace asst::blackflow::perception
{

enum class NodeKind
{
    Small,
    Large,
    Unknown
};
enum class VisualState
{
    Bright,
    Dim,
    Unknown
};

inline const char* to_string(NodeKind value)
{
    switch (value) {
    case NodeKind::Small:
        return "small";
    case NodeKind::Large:
        return "large";
    default:
        return "unknown";
    }
}

inline const char* to_string(VisualState value)
{
    switch (value) {
    case VisualState::Bright:
        return "bright";
    case VisualState::Dim:
        return "dim";
    default:
        return "unknown";
    }
}

struct Node
{
    int id = -1;
    int row = -1;
    int column = -1;
    cv::Point2f center;
    cv::Point2f visual_center;
    float visual_half_width = 10.0F;
    float visual_half_height = 10.0F;
    bool visual_geometry_from_template = false;
    bool exists = false;
    double existence_confidence = 0.0;
    std::string existence_source = "none";
    NodeKind kind = NodeKind::Unknown;
    VisualState state = VisualState::Unknown;
    std::string type = "null";
    std::string display_name;
    double confidence = 0.0;
    double empty_score = 0.0;
    double large_score = 0.0;
    double ring_score = 0.0;
    double brightness_delta = 0.0;
    double empty_offset_x = 0.0;
    double empty_offset_y = 0.0;
    double large_offset_x = 0.0;
    double large_offset_y = 0.0;
    std::string ocr_raw_text;
    std::string ocr_normalized_text;
    std::string ocr_runner_up;
    double ocr_score = 0.0;
    double ocr_similarity = 0.0;
    double ocr_runner_up_similarity = 0.0;
    bool ocr_exact_match = false;
    bool current_marker = false;
    double current_marker_score = 0.0;
    int observed_frames = 1;
    int presence_frame_hits = 0;
    int empty_frame_hits = 0;
    int large_frame_hits = 0;
    std::vector<std::string> evidence;
};

}
