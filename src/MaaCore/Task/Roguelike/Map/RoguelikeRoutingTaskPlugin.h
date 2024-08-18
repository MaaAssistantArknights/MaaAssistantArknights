#pragma once

#include "Common/AsstTypes.h"
#include "Config/Roguelike/RoguelikeMapConfig.h"
#include "Task/Roguelike/AbstractRoguelikeTaskPlugin.h"

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

protected:
    virtual bool _run() override;

private:
    void generate_map();
    void generate_edges(const size_t& node, const cv::Mat& image, const int& node_x);
    void refresh_following_combat_nodes();
    void navigate_route();
    void update_selected_x();

    // ———————— constants and variables ———————————————————————————————————————————————
    RoguelikeMap m_map;
    bool m_need_generate_map = true;
    size_t m_selected_column = 0; // 当前选中节点所在列
    int m_selected_x = 0;         // 当前选中节点的横坐标 (Rect.x)

    Point m_swipe_source = { 0, 0 };                                     // 生成地图时向左滑动起点
    Point m_swipe_target = { 0, 0 };                                     // 生成地图时向左滑动终点
    int m_origin_x = 0;                                                  // 第一列节点的默认横坐标 (Rect.x)
    int m_middle_x = 0;                                                  // 中间列节点的默认横坐标 (Rect.x)
    int m_last_x = 0;                                                    // 最后列节点的默认横坐标 (Rect.x)
    int m_node_width = 0;                                                // 节点 Rect.width
    int m_node_height = 0;                                               // 节点 Rect.height
    int m_column_offset = 0;                                             // 两列节点之间的距离
    int m_nameplate_offset = 0;      // 节点 Rect 下边缘到节点铭牌下边缘的距离
    int m_roi_margin = 0;            // roi 时的 margin offset
    int m_direction_threshold = 0;   // 节点间连线方向判定的阈值
};
}
