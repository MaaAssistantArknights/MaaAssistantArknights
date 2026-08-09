#include "Task/MiniGame/PixelPaintTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::PixelPaintTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& task = details.get("details", "task", "");
    return task == "MiniGame@PixelPaint@Begin";
}

void asst::PixelPaintTaskPlugin::set_groups(std::vector<Group> groups)
{
    m_groups = std::move(groups);
}

bool asst::PixelPaintTaskPlugin::_run()
{
    LogTraceFunction;

    if (m_groups.empty()) {
        Log.info("PixelPaint | no groups to paint");
        return true;
    }

    // 开画前识别
    if (!in_editor_page()) {
        Log.error("PixelPaint | not in pixel editor page, abort");
        return false;
    }

    // 统计是否用到底区颜色（24~39）
    bool need_bottom = false;
    int total = 0;
    for (const auto& g : m_groups) {
        total += static_cast<int>(g.points.size());
        if (g.color >= 24) {
            need_bottom = true;
        }
    }

    Log.info("PixelPaint | total cells:", total, "groups:", m_groups.size(), "need bottom:", need_bottom);

    int done = 0;

    // 第一阶段：滚到顶，画 0~23
    if (!scroll_palette(false)) {
        Log.error("PixelPaint | scroll to top failed");
        return false;
    }
    for (const auto& g : m_groups) {
        if (g.color >= 24) {
            continue;
        }
        if (!draw_group(g, done, total)) {
            return false;
        }
    }

    // 第二阶段：滚到底，画 24~39
    if (need_bottom) {
        if (!scroll_palette(true)) {
            Log.error("PixelPaint | scroll to bottom failed");
            return false;
        }
        for (const auto& g : m_groups) {
            if (g.color < 24) {
                continue;
            }
            if (!draw_group(g, done, total)) {
                return false;
            }
        }
    }

    Log.info("PixelPaint | all done");
    return true;
}

bool asst::PixelPaintTaskPlugin::in_editor_page() const
{
    // 开发期未配模板时跳过识别
    if (Task.get("MiniGame@PixelPaint@EditorCheck") == nullptr) {
        Log.info("PixelPaint | EditorCheck not configured, skip recognition");
        return true;
    }

    auto ret = ProcessTask(*this, { "MiniGame@PixelPaint@EditorCheck" })
                   .set_retry_times(0)
                   .run();
    if (!ret) {
        Log.error("PixelPaint | editor page check failed");
    }
    return ret;
}

bool asst::PixelPaintTaskPlugin::scroll_palette(bool to_bottom) const
{
    const std::string task_name =
        to_bottom ? "MiniGame@PixelPaint@PaletteScrollToBottom" : "MiniGame@PixelPaint@PaletteScrollToTop";
    if (Task.get(task_name) == nullptr) {
        Log.error("PixelPaint | scroll task not found:", task_name);
        return false;
    }

    // 一般一次到位，滑两次做保险；端点多滑会被列表自然吸收
    for (int i = 0; i < 2; ++i) {
        if (need_exit()) {
            return false;
        }
        if (!ProcessTask(*this, { task_name }).set_retry_times(0).run()) {
            Log.warn("PixelPaint | scroll attempt", i + 1, "failed");
            return false;
        }
        sleep(150);
    }
    sleep(300);
    return true;
}

asst::Point asst::PixelPaintTaskPlugin::palette_slot_pos(int color) const
{
    static const std::string top_task = "MiniGame@PixelPaint@PaletteTop";
    static const std::string bottom_task = "MiniGame@PixelPaint@PaletteBottom";

    const int row = color / 4;
    const int col = color % 4;

    if (color < 24) {
        auto params = Task.get(top_task)->special_params;
        // [x0,x1,x2,x3, y0..y5]
        return { params[col], params[4 + row] };
    }

    auto params = Task.get(bottom_task)->special_params;
    // [x0..x3, y0..y5, bottom_first_global_row]
    const int first_global_row = params[10];
    const int visible_row = row - first_global_row;
    return { params[col], params[4 + visible_row] };
}

asst::Point asst::PixelPaintTaskPlugin::grid_center(int x, int y) const
{
    auto params = Task.get("MiniGame@PixelPaint@Grid")->special_params;
    // [left, top, right, bottom]
    const int left = params[0];
    const int top = params[1];
    const int right = params[2];
    const int bottom = params[3];

    constexpr int GridSize = 24;
    const double cell_w = static_cast<double>(right - left) / GridSize;
    const double cell_h = static_cast<double>(bottom - top) / GridSize;
    return {
        static_cast<int>(std::llround(left + (x + 0.5) * cell_w)),
        static_cast<int>(std::llround(top + (y + 0.5) * cell_h)),
    };
}

void asst::PixelPaintTaskPlugin::click_grid(const Point& pos) const
{
    ctrler()->click(pos);
    sleep(GridClickDelay);
}

bool asst::PixelPaintTaskPlugin::draw_group(const Group& group, int& done_cells, int total_cells)
{
    Log.info("PixelPaint | select color", group.color, "cells:", group.points.size());

    // 点色板选色
    const auto slot = palette_slot_pos(group.color);
    ctrler()->click(slot);
    sleep(PaletteClickDelay);

    int clicked = 0;
    for (const auto& p : group.points) {
        if (need_exit()) {
            Log.info("PixelPaint | stopped by exit request");
            return false;
        }

        click_grid(grid_center(p.x, p.y));
        ++clicked;
        ++done_cells;

        // 每 CheckEveryGridClicks 次点格做一次识别，防跑飞后继续乱点
        if (clicked % CheckEveryGridClicks == 0) {
            report_progress(done_cells, total_cells, group.color);
            if (!in_editor_page()) {
                Log.error("PixelPaint | left editor page while painting color", group.color);
                return false;
            }
        }
    }

    // 该色画完再回报一次（不足 CheckEveryGridClicks 的尾巴也覆盖到）
    report_progress(done_cells, total_cells, group.color);
    return true;
}

void asst::PixelPaintTaskPlugin::report_progress(int done_cells, int total_cells, int color)
{
    auto info = basic_info_with_what("PixelPaintProgress");
    auto& details = info["details"];
    details["done"] = done_cells;
    details["total"] = total_cells;
    details["color"] = color;
    callback(AsstMsg::SubTaskExtraInfo, info);
}
