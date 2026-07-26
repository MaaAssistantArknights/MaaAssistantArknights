#include "BlackFlowMapObservationSource.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string_view>

#include <opencv2/imgcodecs.hpp>

#include "Config/Roguelike/BlackFlow/BlackFlowMapPerceptionResource.h"
#include "Utils/WorkingDir.hpp"

namespace asst::blackflow
{
namespace
{
void set_error(std::string* error, std::string message)
{
    if (error != nullptr) {
        *error = std::move(message);
    }
}

Rect visual_rect(const perception::Node& node, const cv::Size& bounds)
{
    const int left = static_cast<int>(std::floor(node.visual_center.x - node.visual_half_width));
    const int top = static_cast<int>(std::floor(node.visual_center.y - node.visual_half_height));
    const int right = static_cast<int>(std::ceil(node.visual_center.x + node.visual_half_width));
    const int bottom = static_cast<int>(std::ceil(node.visual_center.y + node.visual_half_height));
    const cv::Rect clipped = cv::Rect(left, top, std::max(1, right - left), std::max(1, bottom - top)) &
                             cv::Rect(0, 0, bounds.width, bounds.height);
    return { clipped.x, clipped.y, clipped.width, clipped.height };
}

bool valid_artifact_set_id(std::string_view id)
{
    return !id.empty() && std::ranges::all_of(id, [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '-' || character == '_';
    });
}

bool write_image_if_present(const std::filesystem::path& path, const cv::Mat& image, std::string* error)
{
    if (image.empty()) {
        return true;
    }
    if (!cv::imwrite(path.string(), image)) {
        set_error(error, "failed to write BlackFlow diagnostic image: " + path.string());
        return false;
    }
    return true;
}
} // namespace

bool BlackFlowMapObservationSource::prepare(std::string* error)
{
    if (m_analyzer != nullptr) {
        return true;
    }
    if (!m_initialization_error.empty()) {
        set_error(error, m_initialization_error);
        return false;
    }
    try {
        std::string current_error;
        m_analyzer = BlackFlowMapPerception.acquire(current_error);
        if (m_analyzer == nullptr) {
            m_initialization_error = current_error.empty() ? "BlackFlow map perception is unavailable" : current_error;
            set_error(error, m_initialization_error);
            return false;
        }
        m_initialization_error.clear();
        return true;
    }
    catch (const std::exception& exception) {
        m_initialization_error = "BlackFlow map perception initialization failed: " + std::string(exception.what());
    }
    catch (...) {
        m_initialization_error = "BlackFlow map perception initialization failed: unknown exception";
    }
    set_error(error, m_initialization_error);
    return false;
}

void BlackFlowMapObservationSource::release() noexcept
{
    m_analyzer.reset();
    m_initialization_error.clear();
    m_last_result = {};
    m_last_attempt_id.clear();
    m_accumulated_screenshot_us = 0;
    m_accumulated_recognition_us = 0;
    m_last_attempt_count = 0;
    m_last_retry_count = 0;
}

bool BlackFlowMapObservationSource::recognize(
    const cv::Mat& image,
    const BlackFlowObservationRequest& request,
    BlackFlowMapObservation& observation,
    FactStore& observed_facts,
    std::string* error)
{
    observed_facts.clear();
    const int floor = request.floor;
    const int attempt_count = std::max(1, request.attempt_count);
    if (attempt_count == 1) {
        m_accumulated_screenshot_us = 0;
        m_accumulated_recognition_us = 0;
    }

    BlackFlowMapObservation next;
    next.sequence = ++m_sequence;
    next.viewport_revision = next.sequence;
    next.observation_id = "BF-O" + std::to_string(next.sequence);
    next.floor = floor;
    next.floor_from_ocr = true;
    next.coverage = ObservationCoverage::FullMap;
    next.covered_positions = BlackFlowObservationAdapter::expected_grid_positions(floor);
    next.attempt_count = attempt_count;
    next.retry_count = attempt_count - 1;
    m_last_attempt_id = next.observation_id;
    m_last_attempt_count = next.attempt_count;
    m_last_retry_count = next.retry_count;

    perception::MapRecognitionResult result;
    result.floor = floor;
    bool timing_recorded = false;
    const auto record_timing = [&]() {
        if (timing_recorded) {
            return;
        }
        m_accumulated_screenshot_us += std::max<std::int64_t>(0, request.capture_us) + result.normalization_us;
        m_accumulated_recognition_us += result.recognition_us;
        next.screenshot_us = m_accumulated_screenshot_us;
        next.recognition_us = m_accumulated_recognition_us;
        timing_recorded = true;
    };

    try {
        if (m_analyzer == nullptr && !prepare(error)) {
            result.error = m_initialization_error;
            result.captured_bgr = image.clone();
            record_timing();
            m_last_result = std::move(result);
            observation = std::move(next);
            return false;
        }

        result = m_analyzer->recognize(image, floor, m_diagnostics.level != DiagnosticLevel::Normal);
        record_timing();
        if (!result.ok) {
            set_error(error, result.error);
            m_last_result = std::move(result);
            observation = std::move(next);
            return false;
        }

        next.recognition_ok = true;
        next.graph_connected = result.edge_detection.graph_connected;
        next.current_marker_temporary_id = result.node_detection.current_marker_node_id;
        next.current_marker_score = result.node_detection.current_marker_score;

        next.nodes.reserve(result.node_detection.nodes.size());
        for (const auto& node : result.node_detection.nodes) {
            next.nodes.emplace_back(
                PerceptionNodeObservation {
                    node.id,
                    { node.row, node.column },
                    node.exists,
                    node.type,
                    node.display_name,
                    std::max(node.confidence, node.existence_confidence),
                    node.existence_source,
                    visual_rect(node, result.normalized_bgr.size()),
                    std::nullopt,
                    false,
                    std::nullopt,
                });
        }

        next.edges.reserve(result.edge_detection.edges.size());
        for (const auto& edge : result.edge_detection.edges) {
            next.edges.emplace_back(
                PerceptionEdgeObservation {
                    edge.node_a,
                    edge.node_b,
                    edge.connected,
                    edge.cnn_connected,
                    edge.forced_by_connectivity_constraint,
                    edge.calibrated_probability,
                    edge.decision_source,
                });
        }

        observation = std::move(next);
        m_last_result = std::move(result);
        return true;
    }
    catch (const std::exception& exception) {
        result.ok = false;
        result.error = "BlackFlow map recognition failed: " + std::string(exception.what());
    }
    catch (...) {
        result.ok = false;
        result.error = "BlackFlow map recognition failed: unknown exception";
    }
    if (result.captured_bgr.empty() && !image.empty()) {
        result.captured_bgr = image.clone();
    }
    record_timing();
    set_error(error, result.error);
    m_last_result = std::move(result);
    observation = std::move(next);
    return false;
}

void BlackFlowMapObservationSource::configure_diagnostics(const DiagnosticSettings& settings)
{
    m_diagnostics = settings;
}

bool BlackFlowMapObservationSource::persist_diagnostics(const DiagnosticArtifactRequest& request, std::string* error)
{
    if (!valid_artifact_set_id(request.artifact_set_id)) {
        set_error(error, "BlackFlow diagnostic artifact set id is invalid");
        return false;
    }
    try {
        const auto directory = UserDir.get() / "debug" / "BlackFlow" / request.artifact_set_id;
        std::filesystem::create_directories(directory);
        json::object snapshot = request.snapshot;
        snapshot["accepted_observation_id"] = request.observation_id;
        snapshot["recognition_attempt_id"] = m_last_attempt_id;
        snapshot["recognition_attempt_count"] = m_last_attempt_count;
        snapshot["recognition_retry_count"] = m_last_retry_count;
        snapshot["recognition_screenshot_us"] = m_accumulated_screenshot_us;
        snapshot["recognition_us"] = m_accumulated_recognition_us;
        snapshot["recognition_floor"] = m_last_result.floor;
        snapshot["recognition_floor_source"] = "next_level_ocr";
        if (!m_last_result.error.empty()) {
            snapshot["recognition_error"] = m_last_result.error;
        }
        std::ofstream snapshot_file(directory / "snapshot.json", std::ios::binary);
        if (!snapshot_file) {
            set_error(error, "failed to create BlackFlow diagnostic snapshot");
            return false;
        }
        snapshot_file << json::value(std::move(snapshot)).format();
        if (request.include_images) {
            cv::Mat overlay = m_last_result.overlay_bgr;
            if (overlay.empty()) {
                overlay = m_analyzer != nullptr ? m_analyzer->draw_overlay(m_last_result)
                                                : m_last_result.captured_bgr.clone();
            }
            if (!write_image_if_present(directory / "captured.png", m_last_result.captured_bgr, error) ||
                !write_image_if_present(directory / "overlay.png", overlay, error)) {
                return false;
            }
        }
        return true;
    }
    catch (const std::exception& exception) {
        set_error(error, "BlackFlow diagnostic persistence failed: " + std::string(exception.what()));
        return false;
    }
    catch (...) {
        set_error(error, "BlackFlow diagnostic persistence failed: unknown exception");
        return false;
    }
}
} // namespace asst::blackflow
