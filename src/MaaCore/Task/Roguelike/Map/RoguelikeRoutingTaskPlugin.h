#pragma once

#include "Common/AsstTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "RoguelikeMap.h"
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

    enum class RoutingStrategy
    {
        None,
        Sarkaz_FastPass,                 // 实验模式，暂未开放给用户
        Sarkaz_FastInvestment,           // 点刺成锭分队快速投资
        JieGarden_FastPassWithBattle,    // 指挥分队一战快速投资/烧水
        JieGarden_FastPassWithoutBattle, // 指挥分队无战快速投资/烧水
        JieGarden_MerchantRecast,        // 商贾分队重投通宝刷诡意行商存款
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
    // only_first=true 时仅刷新第一个战斗后继节点即停（商贾分队触锁代币只够刷 1 次）
    void refresh_following_combat_nodes(bool only_first = false);
    // 商贾分队：循环重投通宝，直到投钱结果页 OCR 到「触锁代币」；票券耗尽则返回 false
    bool recast_until_token_cast();
    // 商贾分队：进图后游戏自动投钱，其结果弹窗被 RandomPickAfterNextLevel 点确定之前先 OCR 一次，
    // 判断开局是否已投出「触锁代币」，是则置 m_token_cast_at_start，重投阶段直接跳过
    void detect_start_auto_cast();
    void navigate_route();
    void update_selected_x();

    inline static std::function<std::string(RoguelikeNodeType)> type2name = &RoguelikeMapConfig::type2name;

    // ———————— constants and variables ———————————————————————————————————————————————
    RoutingStrategy m_routing_strategy = RoutingStrategy::None;
    RoguelikeMap m_map;
    bool m_need_generate_map = true;
    size_t m_selected_column = 0; // 当前选中节点所在列
    int m_selected_x = 0;         // 当前选中节点的横坐标 (Rect.x)

    // 商贾分队开局自动投：
    bool m_token_cast_at_start = false;            // 开局自动投是否已投出「触锁代币」（true 则免开盒重投）
    bool m_start_auto_cast_checked = false;        // 本局是否已检测过开局自投（只检测一次）
    mutable bool m_verify_start_auto_cast = false; // 本次 verify 命中的是开局自投拦截而非地图寻路

    int m_origin_x = 0;                            // 第一列节点的默认横坐标 (Rect.x)
    int m_middle_x = 0;                            // 中间列节点的默认横坐标 (Rect.x)
    int m_last_x = 0;                              // 最后列节点的默认横坐标 (Rect.x)
    int m_node_width = 0;                          // 节点 Rect.width
    int m_node_height = 0;                         // 节点 Rect.height
    int m_column_offset = 0;                       // 两列节点之间的距离
    int m_nameplate_offset = 0;                    // 节点 Rect 下边缘到节点铭牌下边缘的距离
    int m_roi_margin = 0;                          // roi 的 margin offset
    int m_direction_threshold = 0;                 // 节点间连线方向判定的阈值

    // view-related
    int m_left_most_column_x_in_view = 0;
};
}
