#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "BlackFlowModel.h"

namespace asst::blackflow
{
struct PerceptionNodeObservation
{
    int temporary_id = -1;
    GridPosition position;
    bool exists = false;
    std::string type;
    std::string displayed_name;
    double confidence = 0.0;
    std::string existence_source;
    Rect icon_rect;
    std::optional<Rect> text_rect;
    bool badged = false;
    std::optional<GridPosition> transfer_target;
};

struct PerceptionEdgeObservation
{
    int temporary_first = -1;
    int temporary_second = -1;
    bool connected = false;
    bool cnn_connected = false;
    bool forced_by_connectivity_constraint = false;
    double probability = 0.0;
    std::string decision_source;
};

struct BlackFlowMapObservation
{
    std::string observation_id;
    std::uint64_t sequence = 0;
    std::uint64_t viewport_revision = 0;
    int floor = 0;
    bool floor_from_ocr = false;
    std::optional<int> hud_action_points;
    ObservationCoverage coverage = ObservationCoverage::FullMap;
    std::vector<GridPosition> covered_positions;
    bool recognition_ok = false;
    bool graph_connected = false;
    int current_marker_temporary_id = -1;
    double current_marker_score = 0.0;
    std::vector<PerceptionNodeObservation> nodes;
    std::vector<PerceptionEdgeObservation> edges;
    std::int64_t screenshot_us = 0;
    std::int64_t recognition_us = 0;
    std::int64_t artifact_io_us = 0;
    int attempt_count = 0;
    int retry_count = 0;
};

struct PerceptionSummary
{
    std::string observation_id;
    int floor = 0;
    bool floor_from_ocr = false;
    std::size_t node_count = 0;
    std::size_t confirmed_edge_count = 0;
    std::size_t forced_edge_count = 0;
    std::size_t unclassified_count = 0;
    NodeId current_node = InvalidNodeId;
    std::int64_t screenshot_us = 0;
    std::int64_t recognition_us = 0;
    int attempt_count = 0;
    int retry_count = 0;
};

struct NormalizedPerceptionObservation
{
    MapObservationBatch map;
    std::vector<NodeObservation> viewport;
    NodeId current_node = InvalidNodeId;
    std::optional<int> hud_action_points;
    std::uint64_t viewport_revision = 0;
    PerceptionSummary summary;
};

class BlackFlowObservationAdapter
{
public:
    [[nodiscard]] std::optional<NormalizedPerceptionObservation>
        normalize(const BlackFlowMapObservation& source, std::string* error = nullptr) const;

    [[nodiscard]] static std::optional<NodeType> map_node_type(std::string_view perception_type) noexcept;
    [[nodiscard]] static std::vector<GridPosition> expected_grid_positions(int floor);
};
} // namespace asst::blackflow
