#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "Vision/Roguelike/BlackFlow/PerceptionEdge.h"
#include "Vision/Roguelike/BlackFlow/PerceptionNode.h"

namespace asst::blackflow::perception
{

struct EdgeDetectionResult
{
    std::vector<Edge> edges;
    std::vector<int> inferred_existing_node_ids;
    int existing_node_count = 0;
    int connected_components_before_constraint = 0;
    int connected_components_after_constraint = 0;
    bool connectivity_constraint_applied = false;
    bool graph_connected = false;
    bool model_ready = false;
    std::string error;
};

class EdgeDetector
{
public:
    EdgeDetector();
    ~EdgeDetector();
    EdgeDetector(EdgeDetector&&) noexcept;
    EdgeDetector& operator=(EdgeDetector&&) noexcept;
    EdgeDetector(const EdgeDetector&) = delete;
    EdgeDetector& operator=(const EdgeDetector&) = delete;

    bool load(const std::filesystem::path& runtime_manifest_path, std::string& error);
    bool ready() const noexcept;

    EdgeDetectionResult
        detect(const std::vector<cv::Mat>& frames, std::vector<Node>& nodes, int rows, int columns) const;

    cv::Mat draw_overlay(const cv::Mat& image, const std::vector<Node>& nodes, const EdgeDetectionResult& result) const;

    void write_diagnostics(
        const std::filesystem::path& directory,
        const cv::Mat& frame,
        const std::vector<Node>& nodes,
        int rows,
        int columns) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
