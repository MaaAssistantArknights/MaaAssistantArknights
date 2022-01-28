#include "RoguelikeBattleTaskPlugin.h"

#include "BattleImageAnalyzer.h"
#include "BattlePerspectiveImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"
#include "ProcessTask.h"
#include "OcrImageAnalyzer.h"
#include "Resource.h"
#include "Logger.hpp"

bool asst::RoguelikeBattleTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "Roguelike1StartAction") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeBattleTaskPlugin::_run()
{
    clear();

    bool getted_info = get_stage_info();

    speed_up();

    if (getted_info) {
        while (!need_exit()) {
            // 不在战斗场景，且已使用过了干员，说明已经打完了，就结束循环
            if (!auto_battle() && m_used_opers) {
                break;
            }
        }
    }

    return true;
}

bool asst::RoguelikeBattleTaskPlugin::get_stage_info()
{
    LogTraceFunction;

    const auto stage_name_task_ptr = Task.get("BattleStageName");
    sleep(stage_name_task_ptr->pre_delay);

    const auto& tile = Resrc.tile();
    bool calced = false;
    for (int i = 0; i != m_retry_times; ++i) {
        OcrImageAnalyzer name_analyzer(Ctrler.get_image());
        name_analyzer.set_task_info(stage_name_task_ptr);
        name_analyzer.analyze();

        for (const auto& tr : name_analyzer.get_result()) {
            auto side_info = tile.calc(tr.text, true);
            if (side_info.empty()) {
                continue;
            }
            m_side_tile_info = std::move(side_info);
            calced = true;
            Log.info("stage info getted, tiles_size", m_side_tile_info.size());
            break;
        }
        if (calced) {
            break;
        }
    }

    return calced;
}

bool asst::RoguelikeBattleTaskPlugin::auto_battle()
{
    LogTraceFunction;

    using Role = asst::BattleImageAnalyzer::Role;
    using Oper = asst::BattleImageAnalyzer::Oper;

    BattleImageAnalyzer battle_analyzer(Ctrler.get_image());
    if (!battle_analyzer.analyze()) {
        return false;
    }

    if (int hp = battle_analyzer.get_hp();
        hp != 0) {
        bool used_skills = false;
        if (hp < m_pre_hp) {    // 说明漏怪了，漏怪就开技能（
            for (const Rect& rect : battle_analyzer.get_ready_skills()) {
                use_skill(rect);
                used_skills = true;
            }
        }
        m_pre_hp = hp;
        if (used_skills) {
            return true;
        }
    }

    const auto& opers = battle_analyzer.get_opers();
    if (opers.empty()) {
        return true;
    }

    static const std::array<Role, 9> RoleOrder = {
        Role::Pioneer,
        Role::Sniper,
        Role::Warrior,
        Role::Support,
        Role::Medic,
        Role::Caster,
        Role::Special,
        Role::Tank,
        Role::Drone
    };
    const auto use_oper_task_ptr = Task.get("BattleUseOper");
    const auto swipe_oper_task_ptr = Task.get("BattleSwipeOper");

    // 点击当前最合适的干员
    Oper opt_oper;
    bool oper_found = false;
    for (auto role : RoleOrder) {
        for (const auto& oper : opers) {
            if (!oper.available) {
                continue;
            }
            if (oper.role == role) {
                opt_oper = oper;
                oper_found = true;
                break;
            }
        }
        if (oper_found) {
            break;
        }
    }
    if (!oper_found) {
        return true;
    }
    Ctrler.click(opt_oper.rect);
    sleep(use_oper_task_ptr->pre_delay);

    // 将干员拖动到场上
    Loc loc = Loc::All;
    switch (opt_oper.role) {
    case Role::Medic:
    case Role::Support:
    case Role::Sniper:
    case Role::Caster:
        loc = Loc::Ranged;
        break;
    case Role::Pioneer:
    case Role::Warrior:
    case Role::Tank:
        loc = Loc::Melee;
        break;
    case Role::Special:
    case Role::Drone:
    default:
        // 特种和无人机，有的只能放地面，有的又只能放高台，不好判断
        // 笨办法，都试试，总有一次能成的
    {
        static Loc static_loc = Loc::Melee;
        loc = static_loc;
        if (static_loc == Loc::Melee) {
            static_loc = Loc::Ranged;
        }
        else {
            static_loc = Loc::Melee;
        }
    }
    break;
    }

    Point placed_loc = get_placed(loc);
    Point placed_point = m_side_tile_info.at(placed_loc).pos;
#ifdef ASST_DEBUG
    auto image = Ctrler.get_image();
    cv::circle(image, cv::Point(placed_point.x, placed_point.y), 10, cv::Scalar(0, 0, 255), -1);
#endif
    Rect placed_rect(placed_point.x, placed_point.y, 1, 1);
    Ctrler.swipe(opt_oper.rect, placed_rect, swipe_oper_task_ptr->pre_delay);
    sleep(use_oper_task_ptr->rear_delay);

    // 拖动干员朝向
    if (m_cur_home_index >= m_homes.size()) {
        m_cur_home_index = 0;
    }
    Point home_loc(5, 5);
    if (m_cur_home_index < m_homes.size()) {
        home_loc = m_homes.at(m_cur_home_index);
    }
    Point home_point = m_side_tile_info.at(home_loc).pos;
    Rect home_rect(home_point.x, home_point.y, 1, 1);

    int dx = placed_loc.x - home_loc.x;
    int dy = placed_loc.y - home_loc.y;

    constexpr int coeff = 500;
    Point end_point;
    switch (opt_oper.role) {
    case Role::Medic:
    {
        if (std::abs(dx) <= std::abs(dy)) {
            dx = 0;
        }
        else {
            dy = 0;
        }
        end_point.x = placed_point.x - coeff * dx;
        end_point.y = placed_point.y - coeff * dy;
    }
    break;
    case Role::Support:
    case Role::Pioneer:
    case Role::Warrior:
    case Role::Sniper:
    case Role::Special:
    case Role::Tank:
    case Role::Caster:
    case Role::Drone:
    default:
    {
        if (std::abs(dx) < std::abs(dy)) {
            dx = 0;
        }
        else {
            dy = 0;
        }
        end_point.x = placed_point.x + coeff * dx;
        end_point.y = placed_point.y + coeff * dy;
    }
    break;
    }

    if (end_point.x < 0) {
        end_point.x = 0;
    }
    else if (end_point.x >= WindowWidthDefault) {
        end_point.x = WindowWidthDefault - 1;
    }
    if (end_point.y < 0) {
        end_point.y = 0;
    }
    else if (end_point.y >= WindowHeightDefault) {
        end_point.y = WindowHeightDefault - 1;
    }
    Ctrler.swipe(placed_point, end_point, swipe_oper_task_ptr->rear_delay);

    m_used_tiles.emplace(placed_loc);
    m_used_opers = true;
    ++m_cur_home_index;

    return true;
}

bool asst::RoguelikeBattleTaskPlugin::speed_up()
{
    ProcessTask task(*this, { "BattleSpeedUp" });
    return task.run();
}

bool asst::RoguelikeBattleTaskPlugin::use_skill(const asst::Rect& rect)
{
    LogTraceFunction;

    Ctrler.click(rect);

    ProcessTask task(*this, { "BattleUseSkill" });
    return task.run();
}

void asst::RoguelikeBattleTaskPlugin::clear()
{
    m_used_opers = false;
    m_pre_hp = 0;
    m_homes.clear();
    m_cur_home_index = 0;
    m_side_tile_info.clear();
    m_used_tiles.clear();
}

//asst::Rect asst::RoguelikeBattleTaskPlugin::get_placed_by_cv()
//{
//    BattlePerspectiveImageAnalyzer placed_analyzer(Ctrler.get_image());
//    placed_analyzer.set_src_homes(m_home_cache);
//    if (!placed_analyzer.analyze()) {
//        return Rect();
//    }
//    Point nearest_point = placed_analyzer.get_nearest_point();
//    Rect placed_rect(nearest_point.x, nearest_point.y, 1, 1);
//    return placed_rect;
//}

asst::Point asst::RoguelikeBattleTaskPlugin::get_placed(Loc buildable_type)
{
    LogTraceFunction;

    if (m_homes.empty()) {
        for (const auto& [loc, side] : m_side_tile_info) {
            if (side.key == TilePack::TileKey::Home) {
                m_homes.emplace_back(loc);
            }
        }
        if (m_homes.empty()) {
            Log.error("Unknown home pos");
        }
    }
    if (m_cur_home_index >= m_homes.size()) {
        m_cur_home_index = 0;
    }

    Point nearest;
    int min_dist = INT_MAX;

    Point home(5, 5);   // 默认值，一般是地图的中间
    if (m_cur_home_index < m_homes.size()) {
        home = m_homes.at(m_cur_home_index);
    }

    for (const auto& [loc, tile] : m_side_tile_info) {
        if (tile.buildable == buildable_type
            || tile.buildable == Loc::All) {
            if (m_used_tiles.find(loc) != m_used_tiles.cend()) {
                continue;
            }
            int dx = std::abs(home.x - loc.x);
            int dy = std::abs(home.y - loc.y);
            int dist = dx * dx + dy * dy;
            if (dist < min_dist) {
                min_dist = dist;
                nearest = loc;
            }
        }
    }
    Log.info(__FUNCTION__, nearest.to_string());

    return nearest;
}
