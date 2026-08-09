#pragma once

#include "Task/AbstractTaskPlugin.h"

#include "Common/AsstTypes.h"

#include <optional>
#include <vector>

namespace asst
{
class PixelPaintTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~PixelPaintTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    struct Group
    {
        inline static constexpr int GridSize = 24;
        inline static constexpr int PaletteSize = 40;

        int color = 0; // 0~39，与游戏色板顺序一致
        std::vector<Point> points; // 格坐标，x/y ∈ 0..23
    };

    void set_groups(std::vector<Group> groups);

    const std::vector<Group>& get_groups() const { return m_groups; }

protected:
    virtual bool _run() override;

private:
    // 识别闸：确认仍在像素画编辑页。未配置 EditorCheck 任务（开发期）时跳过识别。
    bool in_editor_page() const;

    // 滚动色板到顶/底（两端锚定），返回是否成功
    bool scroll_palette(bool to_bottom) const;

    // 色号 → 色板槽位 720p 坐标；任务缺失返回空
    std::optional<Point> palette_slot_pos(int color) const;

    // 格坐标 → 画布格心 720p 坐标；任务缺失返回空
    std::optional<Point> grid_center(int x, int y) const;

    // 点一个格子并等待
    void click_grid(const Point& pos) const;

    // 画一种颜色的所有格子（已选好色），返回是否完成
    bool draw_group(const Group& group, int& done_cells, int total_cells);

    // 发送进度回调
    void report_progress(int done_cells, int total_cells, int color);

    std::vector<Group> m_groups;

    // 固定节奏（ms）
    inline static constexpr unsigned GridClickDelay = 0;
    inline static constexpr unsigned PaletteClickDelay = 50;

    // 同色同行连续格用拖动绘制，每格滑动时长（ms）
    inline static constexpr unsigned SwipeMsPerCell = 20;

    // 超过该长度的线段才用拖动（短线段逐格点更快更稳）
    inline static constexpr int MinSwipeSegmentLen = 3;

    // 每 N 次点格做一次识别
    inline static constexpr int CheckEveryGridClicks = 10;
};
} // namespace asst
