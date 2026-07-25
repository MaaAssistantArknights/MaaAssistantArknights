#pragma once

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Common/AsstTypes.h"
#include "Config/Roguelike/RoguelikeMapConfig.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "Vision/VisionHelper.h"

namespace asst
{
// 黑流树海迷宫地图识别器。忠实复刻参考方案 extract_map.py 的确定性流水线，
// 用 ONNX 模型（OnnxSessions: Blackflow_node/edge/player/node_type，均为 sklearn ExtraTrees 转换）
// 替代 sklearn 推理。输入图假定为 MAA 归一化后的 1280x720（scale = w/1280 = 1.0）。
//
// 四个模型的特征维度必须与训练时严格一致：node 181 维、edge 378 维、player 192 维、node_type 2118 维。
// 二分类模型（edge/player）因 skl2onnx 的已知 bug，只能读 probabilities[:,1]。
class RoguelikeBlackflowMapAnalyzer : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~RoguelikeBlackflowMapAnalyzer() override = default;

    enum class CellKind
    {
        None = 0,
        Road,
        Object,
    };

    struct Cell
    {
        int col = 0;
        int row = 0;
        CellKind kind = CellKind::None;
        RoguelikeNodeType type = RoguelikeNodeType::Unknown; // 仅 Object 有意义（节点分类模型识别）
        Point center;           // 屏幕像素坐标（节点中心）
        bool visited = false;   // 铭牌前绿色 ✓
        std::string ocr_text;   // 兼容现有调试输出：Object 节点的分类标签（road 为空）
    };

    struct Result
    {
        int cols = 0;
        int rows = 0;
        std::vector<Cell> cells;                                   // 仅包含存在的节点
        std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> edges; // 无向边（两端 (col,row)）
        std::pair<int, int> player = { -1, -1 };
        bool valid = false;
    };

    // 分析当前 m_image，返回识别结果。失败时 Result.valid == false。
    Result analyze();

private:
    // ———————— 网格检测（_detect_lattice） ————————
    struct Lattice
    {
        int step = 0;
        std::vector<float> xs;
        std::vector<float> ys;
        std::vector<cv::Vec3f> circles; // (x, y, r)，已偏移回全图坐标
        bool valid = false;
    };

    Lattice detect_lattice(const cv::Mat& gray) const;

    // ———————— 特征上下文（预拆分通道，全图；scale 固定 1.0） ————————
    struct NodeCtx
    {
        cv::Mat gray, sat, val, canny; // 均 CV_8U
        cv::Mat grad;                  // CV_32F = magnitude(Sobel_x, Sobel_y)
    };

    struct EdgeCtx
    {
        cv::Mat gray, sat, val, canny;      // CV_8U
        cv::Mat gx, gy, grad;               // CV_32F；gx/gy 为绝对值
    };

    struct PlayerCtx
    {
        cv::Mat gray, hue, sat, val, canny; // CV_8U
    };

    static NodeCtx make_node_ctx(const cv::Mat& image);
    static EdgeCtx make_edge_ctx(const cv::Mat& image);
    static PlayerCtx make_player_ctx(const cv::Mat& image);

    // ———————— 特征提取（与 extract_map.py 逐函数同构，scale 固定 1.0） ————————
    static std::vector<float>
        node_features(const NodeCtx& ctx, float x, float y, const std::vector<cv::Vec3f>& circles);
    // 节点类型分类模型特征：必须与导出模型时使用的特征布局保持一致。
    static std::vector<float> node_type_features(const cv::Mat& image, float x, float y);
    static std::vector<float> edge_features(const EdgeCtx& ctx, float x1, float y1, float x2, float y2);
    static std::vector<float> player_features(const PlayerCtx& ctx, float x, float y);

    // ———————— ONNX 推理 ————————
    // 对 batch 特征跑 TreeEnsembleClassifier，返回每行的概率矩阵（rows x n_classes）。
    static std::vector<std::vector<float>>
        run_tree_ensemble(const std::string& model_name, const std::vector<std::vector<float>>& feats, int n_features);

    // ———————— 已访问标记 ————————
    // 铭牌左侧是否有绿色 ✓
    bool nameplate_has_check(float cx, float cy) const;

    // ———————— 连通分量（_largest_component） ————————
    static std::vector<std::pair<int, int>> largest_component(
        const std::vector<std::pair<int, int>>& vertices,
        const std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>>& edges);

    // 地图区 ROI（相对 1280x720）。参考 _detect_lattice 的裁剪窗口。
    static constexpr double BASE_WIDTH = 1280.0;
    static constexpr double BASE_GRID_STEP = 101.0;
};
}
