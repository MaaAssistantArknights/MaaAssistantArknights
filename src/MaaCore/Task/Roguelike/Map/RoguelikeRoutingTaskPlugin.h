#pragma once

#include "Common/AsstTypes.h"
#include "Config/Roguelike/RoguelikeMapConfig.h"
#include "Task/Roguelike/AbstractRoguelikeTaskPlugin.h"
#include "Utils/NoWarningCVMat.h"

namespace asst
{
class RoguelikeRoutingTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeRoutingTaskPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual bool load_params(const json::value& params) override;
    virtual void reset_in_run_variables() override;

    enum class RoutingStrategy
    {
        None,
        FastInvestment_Sarkaz,
        FastInvestment_JieGarden,
        FastPass
    };

protected:
    virtual bool _run() override;

private:
    /// <summary>
    /// 识别画面中节点并更新地图信息。
    /// </summary>
    /// <param name="image">截图。</param>
    /// <param name="leftmost_column">
    ///     画面最左侧节点 (忽视 init node) 所在列的 index。
    ///     按照定义，要求 leftmost_column >= 1。
    /// </param>
    /// <param name="image_draw_opt">ASST_DEBUG 模式下用于标注识别结果的截图。若为 std::nullopt
    /// 则不标注识别结果。</param> <returns> 若有新地图信息，则返回 true, 反之则返回 false。
    /// </returns>
    /// <remarks>
    /// 画面中最多同时存在三列节点。
    /// </remarks>
    bool update_map(
        const cv::Mat& image,
        size_t leftmost_column = RoguelikeMap::INIT_INDEX + 1,
        std::optional<std::reference_wrapper<cv::Mat>> image_draw_opt = std::nullopt);

    void generate_map();
    void generate_edges(
        const size_t& node,
        const cv::Mat& image,
        const int& node_x,
        std::optional<std::reference_wrapper<cv::Mat>> image_draw_opt = std::nullopt);
    void refresh_following_combat_nodes();
    void navigate_route();
    void update_selected_x();

    // ———————— constants and variables ———————————————————————————————————————————————
    RoutingStrategy m_routing_strategy = RoutingStrategy::None;
    RoguelikeMap m_map;
    bool m_need_generate_map = true;
    size_t m_selected_column = 0;  // 当前选中节点所在列
    int m_selected_x = 0;          // 当前选中节点的横坐标 (Rect.x)

    int m_origin_x = 0;            // 第一列节点的默认横坐标 (Rect.x)
    int m_middle_x = 0;            // 中间列节点的默认横坐标 (Rect.x)
    int m_last_x = 0;              // 最后列节点的默认横坐标 (Rect.x)
    int m_node_width = 0;          // 节点 Rect.width
    int m_node_height = 0;         // 节点 Rect.height
    int m_column_offset = 0;       // 两列节点之间的距离
    int m_nameplate_offset = 0;    // 节点 Rect 下边缘到节点铭牌下边缘的距离
    int m_roi_margin = 0;          // roi 的 margin offset
    int m_direction_threshold = 0; // 节点间连线方向判定的阈值
};
}
