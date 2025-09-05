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

private:
    bool handle_choose_mode();
    bool handle_switch_mode();

    void swipe_copper_list_left(int times, bool slowly = false) const;
    void swipe_copper_list_right(int times, bool slowly = false) const;
    void click_copper_at_position(int col, int row) const;

    RoguelikeCopper create_copper_from_name(
        const std::string& name,
        int col = 0,
        int row = 0,
        bool is_cast = false,
        const Rect& pos = Rect()) const;

    mutable asst::CoppersTaskRunMode m_run_mode;
    // 记录当前通宝列表的状态
    std::vector<RoguelikeCopper> m_copper_list;
    // 新拾取的通宝
    RoguelikeCopper m_new_copper;
    // 待拾取的通宝及其坐标
    std::vector<std::pair<RoguelikeCopper, Point>> m_pending_copper;

    int m_col = 4;        // 列数 (col)
    int m_row = 3;        // 行数 (row)
    int m_origin_x = 0;   // 第一列节点的默认横坐标 (Rect.x)
    int m_last_x = 0;     // 最后一列节点的默认横坐标 (Rect.x)
    int m_origin_y = 0;   // 第一行节点的默认纵坐标 (Rect.y)
    int m_row_offset = 0; // 两行节点之间的距离
};

}
