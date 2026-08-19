#include "Vision/Roguelike/BlackFlow/EdgeDetector.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <numeric>
#include <stdexcept>

#include <nlohmann/json.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#if __has_include(<onnxruntime_cxx_api.h>)
#include <onnxruntime_cxx_api.h>
#elif __has_include(<onnxruntime/onnxruntime_cxx_api.h>)
#include <onnxruntime/onnxruntime_cxx_api.h>
#else
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#endif

#include "Config/OnnxSessions.h"

namespace asst::blackflow::perception
{
namespace
{

struct CandidateEdge
{
    int node_a = -1;
    int node_b = -1;
};

bool node_exists(const Node& node)
{
    return node.exists;
}

void promote_node_from_edge(Node& node, double probability, std::vector<int>& inferred_existing_node_ids)
{
    if (!node.exists) {
        node.exists = true;
        node.existence_confidence = probability;
        node.existence_source = "corridor_net_threshold_edge";
        node.kind = NodeKind::Unknown;
        node.type = "unclassified";
        node.display_name.clear();
        node.confidence = 0.0;
        node.presence_frame_hits = std::max(node.presence_frame_hits, 1);
        node.evidence.push_back("corridor_net_threshold_edge_endpoint");
        inferred_existing_node_ids.push_back(node.id);
    }
    else if (node.existence_source == "corridor_net_threshold_edge") {
        node.existence_confidence = std::max(node.existence_confidence, probability);
    }
}

class DisjointSet
{
public:
    explicit DisjointSet(std::size_t size) :
        m_parent(size),
        m_rank(size, 0)
    {
        std::iota(m_parent.begin(), m_parent.end(), 0);
    }

    int find(int value)
    {
        int& parent = m_parent[static_cast<std::size_t>(value)];
        if (parent != value) {
            parent = find(parent);
        }
        return parent;
    }

    bool unite(int left, int right)
    {
        int left_root = find(left);
        int right_root = find(right);
        if (left_root == right_root) {
            return false;
        }
        int& left_rank = m_rank[static_cast<std::size_t>(left_root)];
        int& right_rank = m_rank[static_cast<std::size_t>(right_root)];
        if (left_rank < right_rank) {
            std::swap(left_root, right_root);
        }
        m_parent[static_cast<std::size_t>(right_root)] = left_root;
        if (left_rank == right_rank) {
            ++left_rank;
        }
        return true;
    }

private:
    std::vector<int> m_parent;
    std::vector<int> m_rank;
};

int connected_component_count(DisjointSet& sets, const std::vector<Node>& nodes)
{
    std::vector<bool> roots(nodes.size(), false);
    int count = 0;
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        if (!node_exists(nodes[index])) {
            continue;
        }
        const int root = sets.find(static_cast<int>(index));
        if (!roots[static_cast<std::size_t>(root)]) {
            roots[static_cast<std::size_t>(root)] = true;
            ++count;
        }
    }
    return count;
}

std::vector<CandidateEdge> candidate_edges(int rows, int columns)
{
    std::vector<CandidateEdge> result;
    result.reserve(static_cast<std::size_t>(rows * (columns - 1) + (rows - 1) * columns));
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const int index = row * columns + column;
            if (column + 1 < columns) {
                result.push_back({ index, index + 1 });
            }
            if (row + 1 < rows) {
                result.push_back({ index, index + columns });
            }
        }
    }
    return result;
}

cv::Mat rectify_corridor(
    const cv::Mat& frame,
    cv::Point2f a,
    cv::Point2f b,
    double endpoint_margin_ratio,
    double width_ratio,
    double minimum_width,
    double maximum_width,
    int output_width,
    int output_height)
{
    const cv::Point2f vector = b - a;
    const double length = cv::norm(vector);
    if (length <= 1.0) {
        return {};
    }
    const cv::Point2f direction = vector * static_cast<float>(1.0 / length);
    const cv::Point2f normal(-direction.y, direction.x);
    const cv::Point2f inner_start = a + direction * static_cast<float>(length * endpoint_margin_ratio);
    const cv::Point2f inner_end = b - direction * static_cast<float>(length * endpoint_margin_ratio);
    const double full_width = std::clamp(length * width_ratio, minimum_width, maximum_width);
    const cv::Point2f half_normal = normal * static_cast<float>(full_width * 0.5);

    const std::array<cv::Point2f, 4> source { inner_start - half_normal,
                                              inner_start + half_normal,
                                              inner_end + half_normal,
                                              inner_end - half_normal };
    const std::array<cv::Point2f, 4> destination {
        cv::Point2f(0.0F, 0.0F),
        cv::Point2f(0.0F, static_cast<float>(output_height - 1)),
        cv::Point2f(static_cast<float>(output_width - 1), static_cast<float>(output_height - 1)),
        cv::Point2f(static_cast<float>(output_width - 1), 0.0F)
    };
    const cv::Mat transform = cv::getPerspectiveTransform(source.data(), destination.data());
    cv::Mat output;
    cv::warpPerspective(
        frame,
        output,
        transform,
        cv::Size(output_width, output_height),
        cv::INTER_CUBIC,
        cv::BORDER_CONSTANT,
        cv::Scalar());
    return output;
}

void draw_dashed_line(cv::Mat& image, cv::Point2f a, cv::Point2f b, const cv::Scalar& color, int thickness)
{
    const cv::Point2f delta = b - a;
    const double length = cv::norm(delta);
    if (length <= 0.0) {
        return;
    }
    const cv::Point2f direction = delta * static_cast<float>(1.0 / length);
    constexpr double Dash = 6.0;
    constexpr double Gap = 4.0;
    for (double offset = 0.0; offset < length; offset += Dash + Gap) {
        const double end = std::min(length, offset + Dash);
        cv::line(
            image,
            a + direction * static_cast<float>(offset),
            a + direction * static_cast<float>(end),
            color,
            thickness,
            cv::LINE_AA);
    }
}

}

struct EdgeDetector::Impl
{
    std::string session_name;
    Ort::Session* session = nullptr;
    bool session_acquired = false;
    std::filesystem::path runtime_manifest_path;
    std::string input_name = "input";
    std::string output_name = "logit";
    int input_channels = 3;
    int input_height = 40;
    int input_width = 160;
    float scale = 1.0F / 255.0F;
    std::array<float, 3> mean { 0.5F, 0.5F, 0.5F };
    std::array<float, 3> standard_deviation { 0.25F, 0.25F, 0.25F };
    double endpoint_margin_ratio = 0.16;
    double width_ratio = 0.30;
    double minimum_width = 16.0;
    double maximum_width = 64.0;
    double temperature = 1.0;
    double probability_threshold = 0.5;
    double raw_logit_threshold = 0.0;

    ~Impl()
    {
        if (session_acquired) {
            OnnxSessions::get_instance().release(session_name);
        }
    }

    std::vector<float> run(const std::vector<cv::Mat>& crops) const
    {
        if (!session) {
            throw std::runtime_error("CorridorNet session is not loaded");
        }
        if (crops.empty()) {
            return {};
        }
        const std::size_t plane = static_cast<std::size_t>(input_height * input_width);
        std::vector<float> input(crops.size() * static_cast<std::size_t>(input_channels) * plane);
        for (std::size_t batch = 0; batch < crops.size(); ++batch) {
            if (crops[batch].size() != cv::Size(input_width, input_height) || crops[batch].type() != CV_8UC3) {
                throw std::runtime_error("CorridorNet crop has an unexpected shape or type");
            }
            for (int y = 0; y < input_height; ++y) {
                const auto* row = crops[batch].ptr<cv::Vec3b>(y);
                for (int x = 0; x < input_width; ++x) {
                    const cv::Vec3b pixel = row[x];
                    const std::array<float, 3> rgb { static_cast<float>(pixel[2]),
                                                     static_cast<float>(pixel[1]),
                                                     static_cast<float>(pixel[0]) };
                    for (int channel = 0; channel < 3; ++channel) {
                        const std::size_t index = batch * 3 * plane + static_cast<std::size_t>(channel) * plane +
                                                  static_cast<std::size_t>(y * input_width + x);
                        input[index] =
                            (rgb[static_cast<std::size_t>(channel)] * scale - mean[static_cast<std::size_t>(channel)]) /
                            standard_deviation[static_cast<std::size_t>(channel)];
                    }
                }
            }
        }

        const std::array<std::int64_t, 4> shape { static_cast<std::int64_t>(crops.size()),
                                                  input_channels,
                                                  input_height,
                                                  input_width };
        Ort::MemoryInfo memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value tensor =
            Ort::Value::CreateTensor<float>(memory, input.data(), input.size(), shape.data(), shape.size());
        const std::array<const char*, 1> input_names { input_name.c_str() };
        const std::array<const char*, 1> output_names { output_name.c_str() };
        auto outputs =
            session->Run(Ort::RunOptions { nullptr }, input_names.data(), &tensor, 1, output_names.data(), 1);
        if (outputs.size() != 1 || !outputs.front().IsTensor()) {
            throw std::runtime_error("CorridorNet returned an invalid output");
        }
        const auto info = outputs.front().GetTensorTypeAndShapeInfo();
        if (info.GetElementCount() != crops.size()) {
            throw std::runtime_error("CorridorNet output count does not match candidate edges");
        }
        const float* values = outputs.front().GetTensorData<float>();
        return std::vector<float>(values, values + crops.size());
    }
};

EdgeDetector::EdgeDetector() :
    m_impl(std::make_unique<Impl>())
{
}

EdgeDetector::~EdgeDetector() = default;
EdgeDetector::EdgeDetector(EdgeDetector&&) noexcept = default;
EdgeDetector& EdgeDetector::operator=(EdgeDetector&&) noexcept = default;

bool EdgeDetector::load(const std::filesystem::path& runtime_manifest_path, std::string& error)
{
    try {
        std::ifstream input(runtime_manifest_path, std::ios::binary);
        if (!input) {
            error = "Cannot open CorridorNet runtime manifest: " + runtime_manifest_path.string();
            return false;
        }
        nlohmann::json manifest;
        input >> manifest;
        if (manifest.value("schema_version", 0) != 1 ||
            manifest.value("model_name", std::string()) != "BlackFlow_corridor_net") {
            error = "Unsupported CorridorNet runtime manifest";
            return false;
        }
        auto next = std::make_unique<Impl>();
        next->runtime_manifest_path = std::filesystem::absolute(runtime_manifest_path);
        next->session_name = manifest.at("model_name").get<std::string>();

        const auto input_spec = manifest.at("input");
        const auto output_spec = manifest.at("output");
        next->input_name = input_spec.at("name").get<std::string>();
        next->output_name = output_spec.at("name").get<std::string>();
        const auto shape = input_spec.at("shape");
        next->input_channels = shape.at(1).get<int>();
        next->input_height = shape.at(2).get<int>();
        next->input_width = shape.at(3).get<int>();
        if (next->input_channels != 3 || next->input_height <= 0 || next->input_width <= 0) {
            error = "CorridorNet input shape is unsupported";
            return false;
        }

        const auto preprocessing = manifest.at("preprocessing");
        next->scale = preprocessing.at("scale").get<float>();
        for (int index = 0; index < 3; ++index) {
            next->mean[static_cast<std::size_t>(index)] = preprocessing.at("mean").at(index).get<float>();
            next->standard_deviation[static_cast<std::size_t>(index)] = preprocessing.at("std").at(index).get<float>();
            if (next->standard_deviation[static_cast<std::size_t>(index)] <= 0.0F) {
                error = "CorridorNet preprocessing standard deviations must be positive";
                return false;
            }
        }
        const auto corridor = manifest.at("corridor");
        next->endpoint_margin_ratio = corridor.at("endpoint_margin_ratio").get<double>();
        next->width_ratio = corridor.at("width_ratio_of_edge_length").get<double>();
        next->minimum_width = corridor.at("minimum_width_pixels").get<double>();
        next->maximum_width = corridor.at("maximum_width_pixels").get<double>();
        const auto decision = manifest.at("decision");
        next->temperature = decision.at("temperature").get<double>();
        next->probability_threshold = decision.at("probability_threshold").get<double>();
        next->raw_logit_threshold = decision.at("raw_logit_threshold").get<double>();
        if (next->temperature <= 0.0) {
            error = "CorridorNet calibration temperature must be positive";
            return false;
        }
        if (next->probability_threshold <= 0.0 || next->probability_threshold >= 1.0) {
            error = "CorridorNet probability threshold must be between zero and one";
            return false;
        }
        const double threshold_from_probability =
            next->temperature * std::log(next->probability_threshold / (1.0 - next->probability_threshold));
        if (std::abs(threshold_from_probability - next->raw_logit_threshold) > 1e-5) {
            error = "CorridorNet probability and raw-logit thresholds are inconsistent";
            return false;
        }

        next->session = &OnnxSessions::get_instance().acquire(next->session_name);
        next->session_acquired = true;
        const cv::Mat warmup(next->input_height, next->input_width, CV_8UC3, cv::Scalar());
        next->run({ warmup });
        m_impl = std::move(next);
        return true;
    }
    catch (const Ort::Exception& exception) {
        error = "CorridorNet ONNX Runtime error: " + std::string(exception.what());
        return false;
    }
    catch (const std::exception& exception) {
        error = "CorridorNet cannot be loaded: " + std::string(exception.what());
        return false;
    }
    catch (...) {
        error = "CorridorNet cannot be loaded: unknown exception";
        return false;
    }
}

bool EdgeDetector::ready() const noexcept
{
    return m_impl && m_impl->session != nullptr;
}

EdgeDetectionResult
    EdgeDetector::detect(const std::vector<cv::Mat>& frames, std::vector<Node>& nodes, int rows, int columns) const
{
    EdgeDetectionResult output;
    output.model_ready = ready();
    if (!ready()) {
        output.error = "CorridorNet is not loaded";
        return output;
    }
    if (frames.empty() || frames.front().empty()) {
        output.error = "Edge detection requires at least one normalized frame";
        return output;
    }
    if (rows <= 0 || columns <= 0 || nodes.size() < static_cast<std::size_t>(rows * columns)) {
        output.error = "Edge detection received an invalid grid or node array";
        return output;
    }

    try {
        const std::vector<CandidateEdge> candidates = candidate_edges(rows, columns);
        std::vector<cv::Mat> crops;
        crops.reserve(candidates.size());
        for (const CandidateEdge& candidate : candidates) {
            crops.push_back(rectify_corridor(
                frames.front(),
                nodes[static_cast<std::size_t>(candidate.node_a)].center,
                nodes[static_cast<std::size_t>(candidate.node_b)].center,
                m_impl->endpoint_margin_ratio,
                m_impl->width_ratio,
                m_impl->minimum_width,
                m_impl->maximum_width,
                m_impl->input_width,
                m_impl->input_height));
        }
        const std::vector<float> logits = m_impl->run(crops);
        output.edges.reserve(candidates.size());
        for (std::size_t index = 0; index < candidates.size(); ++index) {
            const CandidateEdge candidate = candidates[index];
            const double logit = logits[index];
            const double scaled = std::clamp(logit / m_impl->temperature, -80.0, 80.0);
            const double probability = 1.0 / (1.0 + std::exp(-scaled));
            const bool cnn_connected = probability >= m_impl->probability_threshold;
            Edge edge;
            edge.id = static_cast<int>(index);
            edge.node_a = candidate.node_a;
            edge.node_b = candidate.node_b;
            edge.cnn_connected = cnn_connected;
            edge.connected = false;
            edge.raw_logit = logit;
            edge.calibrated_probability = probability;
            edge.confidence = probability;
            edge.decision_source = cnn_connected ? "cnn_threshold" : "cnn_below_threshold";
            edge.path = { nodes[static_cast<std::size_t>(candidate.node_a)].center,
                          nodes[static_cast<std::size_t>(candidate.node_b)].center };
            output.edges.push_back(std::move(edge));
        }

        for (Edge& edge : output.edges) {
            if (!edge.cnn_connected) {
                continue;
            }
            promote_node_from_edge(
                nodes[static_cast<std::size_t>(edge.node_a)],
                edge.calibrated_probability,
                output.inferred_existing_node_ids);
            promote_node_from_edge(
                nodes[static_cast<std::size_t>(edge.node_b)],
                edge.calibrated_probability,
                output.inferred_existing_node_ids);
            edge.connected = true;
        }

        for (const Node& node : nodes) {
            if (node_exists(node)) {
                ++output.existing_node_count;
            }
        }
        if (output.existing_node_count == 0) {
            output.error = "Edge recovery found no existing nodes";
            return output;
        }

        DisjointSet components(nodes.size());
        for (const Edge& edge : output.edges) {
            if (edge.connected) {
                components.unite(edge.node_a, edge.node_b);
            }
        }
        output.connected_components_before_constraint = connected_component_count(components, nodes);
        int component_count = output.connected_components_before_constraint;

        std::vector<std::size_t> order(output.edges.size());
        std::iota(order.begin(), order.end(), 0);
        std::ranges::sort(order, [&](std::size_t left, std::size_t right) {
            if (output.edges[left].calibrated_probability != output.edges[right].calibrated_probability) {
                return output.edges[left].calibrated_probability > output.edges[right].calibrated_probability;
            }
            return output.edges[left].id < output.edges[right].id;
        });
        for (const std::size_t index : order) {
            if (component_count <= 1) {
                break;
            }
            Edge& edge = output.edges[index];
            if (edge.connected) {
                continue;
            }
            const Node& a = nodes[static_cast<std::size_t>(edge.node_a)];
            const Node& b = nodes[static_cast<std::size_t>(edge.node_b)];
            if (!node_exists(a) || !node_exists(b)) {
                continue;
            }
            if (!components.unite(edge.node_a, edge.node_b)) {
                continue;
            }
            edge.connected = true;
            edge.forced_by_connectivity_constraint = true;
            edge.decision_source = "connectivity_constraint_cross_component_highest_cnn_score";
            --component_count;
            output.connectivity_constraint_applied = true;
        }

        output.connected_components_after_constraint = component_count;
        output.graph_connected = component_count == 1;
        if (!output.graph_connected) {
            output.error = "Edge recovery could not connect all existing nodes: " + std::to_string(component_count) +
                           " connected components remain";
        }
        return output;
    }
    catch (const Ort::Exception& exception) {
        output.error = "CorridorNet inference failed: " + std::string(exception.what());
        return output;
    }
    catch (const std::exception& exception) {
        output.error = "Edge detection failed: " + std::string(exception.what());
        return output;
    }
    catch (...) {
        output.error = "Edge detection failed: unknown exception";
        return output;
    }
}

cv::Mat EdgeDetector::draw_overlay(
    const cv::Mat& image,
    const std::vector<Node>& nodes,
    const EdgeDetectionResult& result) const
{
    cv::Mat overlay = image.clone();
    for (const Edge& edge : result.edges) {
        if (!edge.connected || edge.node_a < 0 || edge.node_b < 0 || edge.node_a >= static_cast<int>(nodes.size()) ||
            edge.node_b >= static_cast<int>(nodes.size())) {
            continue;
        }
        draw_dashed_line(
            overlay,
            nodes[static_cast<std::size_t>(edge.node_a)].center,
            nodes[static_cast<std::size_t>(edge.node_b)].center,
            cv::Scalar(0, 255, 80),
            1);
    }
    for (const Node& node : nodes) {
        if (node.exists) {
            cv::circle(overlay, node.center, 3, cv::Scalar(0, 255, 80), cv::FILLED, cv::LINE_AA);
        }
    }
    return overlay;
}

void EdgeDetector::write_diagnostics(
    const std::filesystem::path& directory,
    const cv::Mat& frame,
    const std::vector<Node>& nodes,
    int rows,
    int columns) const
{
    if (!ready() || frame.empty() || nodes.size() < static_cast<std::size_t>(rows * columns)) {
        return;
    }
    std::filesystem::create_directories(directory);
    const auto candidates = candidate_edges(rows, columns);
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        const auto candidate = candidates[index];
        const cv::Mat crop = rectify_corridor(
            frame,
            nodes[static_cast<std::size_t>(candidate.node_a)].center,
            nodes[static_cast<std::size_t>(candidate.node_b)].center,
            m_impl->endpoint_margin_ratio,
            m_impl->width_ratio,
            m_impl->minimum_width,
            m_impl->maximum_width,
            m_impl->input_width,
            m_impl->input_height);
        const std::string name = "edge_" + std::to_string(index) + "_n" + std::to_string(candidate.node_a) + "_n" +
                                 std::to_string(candidate.node_b) + ".png";
        cv::imwrite((directory / name).string(), crop);
    }
}

}
