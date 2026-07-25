#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include <opencv2/core.hpp>

#include "Vision/Roguelike/BlackFlow/EdgeDetector.h"
#include "Vision/Roguelike/BlackFlow/FrameNormalizer.h"
#include "Vision/Roguelike/BlackFlow/NodeDetector.h"
#include "Vision/Roguelike/BlackFlow/RecognitionBridge.h"

namespace asst::blackflow::perception
{
struct FloorProfile
{
    int floor = 0;
    int rows = 0;
    int columns = 0;
};

struct MapRecognitionResult
{
    bool ok = false;
    std::string error;
    int floor = 0;
    int rows = 0;
    int columns = 0;
    cv::Mat captured_bgr;
    cv::Mat normalized_bgr;
    cv::Mat node_overlay_bgr;
    cv::Mat edge_overlay_bgr;
    NodeDetectionResult node_detection;
    EdgeDetectionResult edge_detection;
    std::int64_t normalization_us = 0;
    std::int64_t recognition_us = 0;
};

class BlackFlowMapAnalyzer
{
public:
    bool load(const std::filesystem::path& resource_root, std::string& error);
    [[nodiscard]] MapRecognitionResult recognize(const cv::Mat& image, int floor) const;
    [[nodiscard]] bool loaded() const noexcept;

private:
    RecognitionBridge m_bridge;
    std::unique_ptr<NodeDetector> m_node_detector;
    EdgeDetector m_edge_detector;
    FrameNormalizer m_normalizer;
    bool m_loaded = false;
};

[[nodiscard]] FloorProfile floor_profile(int floor);
} // namespace asst::blackflow::perception
