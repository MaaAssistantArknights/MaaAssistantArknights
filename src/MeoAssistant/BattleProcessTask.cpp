#include "BattleProcessTask.h"

#include "Controller.h"
#include "Resource.h"
#include "TaskData.h"
#include "Logger.hpp"

#include "ProcessTask.h"

#include "OcrImageAnalyzer.h"
#include "BattleImageAnalyzer.h"

void asst::BattleProcessTask::set_stage_name(std::string name)
{
    m_stage_name = std::move(name);
}

bool asst::BattleProcessTask::_run()
{
    bool ret = get_stage_info();

    if (!ret) {
        return false;
    }

    while (!analyze_opers_preview()) {
        ;
    }

    for (const BattleAction& action : m_actions.actions) {
        do_action(action);
    }

    return true;
}

bool asst::BattleProcessTask::get_stage_info()
{
    const auto& tile = Resrc.tile();

    m_side_tile_info = tile.calc(m_stage_name, true);
    m_normal_tile_info = tile.calc(m_stage_name, false);

    if (m_side_tile_info.empty() || m_normal_tile_info.empty()) {
        return false;
    }

    const auto& battle = Resrc.battle();
    bool contains = battle.contains_actions(m_stage_name);
    if (!contains) {
        return false;
    }

    m_actions = battle.get_actions(m_stage_name);

    return true;
}

bool asst::BattleProcessTask::analyze_opers_preview()
{
    BattleImageAnalyzer analyzer(m_ctrler->get_image());
    analyzer.set_target(BattleImageAnalyzer::Target::Oper);
    if (!analyzer.analyze()) {
        return false;
    }
    // TODO: 干员头像出来之后，还要过 2 秒左右才可以点击，这里可能还要加个延时

    battle_pause();
    auto opers = analyzer.get_opers();

    for (size_t i = 0; i != opers.size(); ++i) {
        const auto& cur_oper = analyzer.get_opers();
        size_t offset = opers.size() > cur_oper.size() ? opers.size() - cur_oper.size() : 0;
        m_ctrler->click(cur_oper.at(i - offset).rect);

        sleep(Task.get("BattleUseOper")->pre_delay);

        auto image = m_ctrler->get_image();

        OcrImageAnalyzer name_analyzer(image);
        name_analyzer.set_task_info("BattleOperName");
        name_analyzer.set_replace(
            std::dynamic_pointer_cast<OcrTaskInfo>(
                Task.get("Roguelike1RecruitData"))
            ->replace_map);

        std::string oper_name = "Unknown";
        if (name_analyzer.analyze()) {
            name_analyzer.sort_result_by_score();
            oper_name = name_analyzer.get_result().front().text;
        }
        opers.at(i).name = oper_name;

        m_cur_opers_info.emplace(std::move(oper_name), std::move(opers.at(i)));

        // 干员特别多的时候，任意干员被点开，都会导致下方的干员图标被裁剪和移动。所以这里需要重新识别一下
        analyzer.set_image(image);
        analyzer.analyze();
    }
    battle_pause();
    cancel_selection();

    return true;
}

bool asst::BattleProcessTask::do_action(const BattleAction& action)
{
    if (!wait_condition(action)) {
        return false;
    }
    const auto use_oper_task_ptr = Task.get("BattleUseOper");
    const auto swipe_oper_task_ptr = Task.get("BattleSwipeOper");

    // TODO，临时调试方案。正式版本这里需要对 group_name -> oper_name 做一个转换
    auto iter = m_cur_opers_info.find(action.group_name);
    if (iter == m_cur_opers_info.cend()) {
        Log.error("battle opers group", action.group_name, "not found");
        return false;
    }

    // 点击干员
    Rect oper_rect = iter->second.rect;
    m_ctrler->click(oper_rect);
    sleep(use_oper_task_ptr->pre_delay);

    // 拖动到场上
    Point placed_point = m_side_tile_info[action.location].pos;
    Rect placed_rect{ placed_point.x ,placed_point.y, 1, 1 };
    m_ctrler->swipe(oper_rect, placed_rect);

    sleep(use_oper_task_ptr->rear_delay);

    if (action.direction != BattleDeployDirection::No) {
        static const std::unordered_map<BattleDeployDirection, Point> DirectionMapping = {
            { BattleDeployDirection::Right, Point(1, 0)},
            { BattleDeployDirection::Down, Point(0, 1)},
            { BattleDeployDirection::Left, Point(-1, 0)},
            { BattleDeployDirection::Up, Point(0, -1)},
            { BattleDeployDirection::No, Point(0, 0)},
        };

        // 计算往哪边拖动（干员朝向）
        Point direction = DirectionMapping.at(action.direction);

        // 将方向转换为实际的 swipe end 坐标点
        Point end_point = placed_point;
        constexpr int coeff = 500;
        end_point.x += direction.x * coeff;
        end_point.y += direction.y * coeff;

        end_point.x = std::max(0, end_point.x);
        end_point.x = std::min(end_point.x, WindowWidthDefault);
        end_point.y = std::max(0, end_point.y);
        end_point.y = std::min(end_point.y, WindowHeightDefault);

        m_ctrler->swipe(placed_point, end_point, swipe_oper_task_ptr->rear_delay);
    }

    return true;
}

bool asst::BattleProcessTask::wait_condition(const BattleAction& action)
{
    constexpr int analyze_target = BattleImageAnalyzer::Target::Kills | BattleImageAnalyzer::Target::Vacancies | BattleImageAnalyzer::Target::Cost;

    while (true) {
        auto image = m_ctrler->get_image();
        BattleImageAnalyzer analyzer(image);
        analyzer.set_target(analyze_target);

        if (!analyzer.analyze()) {
            return true;
        }
        if (action.kills_condition <= analyzer.get_kills()) {
            return true;
        }
    }

    return false;
}

bool asst::BattleProcessTask::battle_pause()
{
    return ProcessTask(*this, { "BattlePause" }).run();
}

bool asst::BattleProcessTask::cancel_selection()
{
    return ProcessTask(*this, { "BattleCancelSelection" }).run();
}
