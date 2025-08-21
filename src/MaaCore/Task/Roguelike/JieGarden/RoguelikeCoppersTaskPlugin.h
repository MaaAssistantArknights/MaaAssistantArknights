#pragma once

#include "Common/AsstTypes.h"
#include "Config/Roguelike/JieGarden/RoguelikeCoppersConfig.h"
#include "Task/Roguelike/AbstractRoguelikeTaskPlugin.h"

namespace asst
{

enum class CoppersTaskRunMode
{
    CHOOSE, // 拾取掉落
    SWITCH  // 交换通宝
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
    // 向左缓慢滑动
    void slowly_swipe_to_the_left_of_copperlist(int loop_times = 1) const;
    // 向右缓慢滑动
    void slowly_swipe_to_the_right_of_copperlist(int loop_times = 1) const;
    void swipe_to_the_left_of_copperlist(int loop_times = 1) const;
    void swipe_to_the_right_of_copperlist(int loop_times = 1) const;

private:
    void clear();

    // 记录当前插件运行模式
    mutable asst::CoppersTaskRunMode m_run_mode;
    // 记录当前通宝列表的状态
    std::vector<RoguelikeCopper> m_copper_list;
    // 新拾取的通宝
    RoguelikeCopper m_new_copper;
    // 待拾取的通宝及其坐标
    std::vector<std::pair<RoguelikeCopper, Point>> m_pending_copper;

    int m_col = 0;              // 列数 (col)
    int m_row = 0;              // 行数 (row)
    int m_origin_x = 0;         // 第一列节点的默认横坐标 (Rect.x)
    int m_origin_y = 0;         // 第一行节点的默认纵坐标 (Rect.y)
    int m_last_y = 0;           // 最后行节点的默认纵坐标 (Rect.y)
    int m_column_offset = 0;    // 两列节点之间的距离
    int m_row_offset = 0;       // 两行节点之间的距离
    int m_ocr_roi_offset_x = 0; // OCR区域相对于通宝图标的横坐标偏移
    int m_ocr_roi_offset_y = 0; // OCR区域相对于通宝图标的纵坐标偏移
    int m_ocr_roi_width = 0;    // OCR区域宽度
    int m_ocr_roi_height = 0;   // OCR区域高度
};
}
