#pragma once

#include "Common/AsstTypes.h"
#include "Config/Roguelike/JieGarden/RoguelikeCoppersConfig.h"
#include "Task/Roguelike/AbstractRoguelikeTaskPlugin.h"
#include "Vision/Roguelike/JieGarden/RoguelikeCoppersAnalyzer.h"

#include <opencv2/core.hpp>

namespace asst
{

enum class CoppersTaskRunMode
{
    PICKUP,  // 拾取掉落通宝
    EXCHANGE // 交换已有通宝
};

class RoguelikeCoppersTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeCoppersTaskPlugin() override = default;

    virtual bool load_params(const json::value& params) override;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual void reset_in_run_variables() override;

protected:
    virtual bool _run() override;

private:
    bool handle_pickup_mode();
    bool handle_exchange_mode();

    bool swipe_copper_list_left(int times, bool slowly = false) const;
    bool swipe_copper_list_right(int times, bool slowly = false) const;
    void click_copper_at_position(int col, int row) const;

    std::optional<RoguelikeCopper> create_copper_from_name(
        const std::string& name,
        int col = 0,
        int row = 0,
        bool is_cast = false,
        const Rect& pos = Rect()) const;

#ifdef ASST_DEBUG
    // 调试绘制辅助函数
    void draw_detection_debug(
        cv::Mat& image,
        const RoguelikeCoppersAnalyzer::CopperDetection& detection,
        const cv::Scalar& color) const;
    void save_debug_image(const cv::Mat& image, const std::string& suffix) const;
#endif

    mutable asst::CoppersTaskRunMode m_run_mode;
    // 记录当前通宝列表的状态
    std::vector<RoguelikeCopper> m_copper_list;
    // 新拾取的通宝
    RoguelikeCopper m_new_copper;
    // 待拾取的通宝及其坐标
    std::vector<std::pair<RoguelikeCopper, Point>> m_pending_copper;

    int m_col = 4;        // 列数 (col)
    int m_origin_x = 0;   // 第一列节点的默认横坐标 (Rect.x)
    int m_last_x = 0;     // 最后一列节点的默认横坐标 (Rect.x)
    int m_origin_y = 0;   // 第一行节点的默认纵坐标 (Rect.y)
    int m_row_offset = 0; // 两行节点之间的距离
};

}
