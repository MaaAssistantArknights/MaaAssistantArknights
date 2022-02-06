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

void asst::RoguelikeBattleTaskPlugin::set_stage_name(std::string stage)
{
    m_stage_name = std::move(stage);
}

bool asst::RoguelikeBattleTaskPlugin::_run()
{
    bool getted_info = get_stage_info();

    speed_up();

    if (!getted_info) {
        return true;
    }

    while (!need_exit()) {
        // 不在战斗场景，且已使用过了干员，说明已经打完了，就结束循环
        if (!auto_battle() && m_used_opers) {
            break;
        }
    }

    clear();

    return true;
}

bool asst::RoguelikeBattleTaskPlugin::get_stage_info()
{
    LogTraceFunction;

    const auto& tile = Resrc.tile();
    bool calced = false;

    if (m_stage_name.empty()) {
        const auto stage_name_task_ptr = Task.get("BattleStageName");
        sleep(stage_name_task_ptr->pre_delay);

        constexpr int StageNameRetryTimes = 50;
        for (int i = 0; i != StageNameRetryTimes; ++i) {
            cv::Mat image = Ctrler.get_image();
            OcrImageAnalyzer name_analyzer(image);
            name_analyzer.set_task_info(stage_name_task_ptr);
            if (!name_analyzer.analyze()) {
                continue;
            }

            for (const auto& tr : name_analyzer.get_result()) {
                auto side_info = tile.calc(tr.text, true);
                if (side_info.empty()) {
                    continue;
                }
                m_side_tile_info = std::move(side_info);
                m_stage_name = tr.text;
                calced = true;
                break;
            }
            if (calced) {
                break;
            }
        }
    }
    else {
        m_side_tile_info = tile.calc(m_stage_name, true);
        calced = true;
    }

    if (calced) {
#ifdef ASST_DEBUG
        auto normal_tiles = tile.calc(m_stage_name, false);
        cv::Mat draw = Ctrler.get_image();
        for (const auto& [point, info] : normal_tiles) {
            using TileKey = TilePack::TileKey;
            static const std::unordered_map<TileKey, std::string> TileKeyMapping = {
                { TileKey::Invalid, "invalid" },
                { TileKey::Forbidden, "forbidden" },
                { TileKey::Wall, "wall" },
                { TileKey::Road, "road" },
                { TileKey::Home, "end" },
                { TileKey::EnemyHome, "start" },
                { TileKey::Floor, "floor" },
                { TileKey::Hole, "hole" },
                { TileKey::Telin, "telin" },
                { TileKey::Telout, "telout" }
            };

            cv::putText(draw, TileKeyMapping.at(info.key), cv::Point(info.pos.x, info.pos.y), 1, 1, cv::Scalar(0, 0, 255));
        }
#endif

        auto cb_info = basic_info_with_what("StageInfo");
        auto& details = cb_info["details"];
        details["name"] = m_stage_name;
        details["size"] = m_side_tile_info.size();
        callback(AsstMsg::SubTaskExtraInfo, cb_info);
    }
    else {
        callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("StageInfoError"));
    }

    return calced;
}

bool asst::RoguelikeBattleTaskPlugin::auto_battle()
{
    LogTraceFunction;

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
                used_skills = true;
                if (!use_skill(rect)) {
                    break;
                }
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

    // 计算往哪边拖动（干员朝向）
    Point direction = calc_direction(placed_loc, opt_oper.role);

    // 将方向转换为实际的 swipe end 坐标点
    Point end_point = placed_point;
    constexpr int coeff = 500;
    end_point.x += direction.x * coeff;
    end_point.y += direction.y * coeff;

    end_point.x = std::max(0, end_point.x);
    end_point.x = std::min(end_point.x, WindowWidthDefault);
    end_point.y = std::max(0, end_point.y);
    end_point.y = std::min(end_point.y, WindowHeightDefault);

    Ctrler.swipe(placed_point, end_point, swipe_oper_task_ptr->rear_delay);

    m_used_tiles.emplace(placed_loc);
    m_used_opers = true;
    ++m_cur_home_index;

    return true;
}

bool asst::RoguelikeBattleTaskPlugin::speed_up()
{
    LogTraceFunction;

    ProcessTask task(*this, { "BattleSpeedUp" });
    return task.run();
}

bool asst::RoguelikeBattleTaskPlugin::use_skill(const asst::Rect& rect)
{
    LogTraceFunction;

    Ctrler.click(rect);

    ProcessTask task(*this, { "BattleUseSkill" });
    task.set_retry_times(0);
    return task.run();
}

void asst::RoguelikeBattleTaskPlugin::clear()
{
    m_used_opers = false;
    m_pre_hp = 0;
    m_homes.clear();
    m_cur_home_index = 0;
    m_stage_name.clear();
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

asst::Point asst::RoguelikeBattleTaskPlugin::calc_direction(Point loc, Role role)
{
    // 根据家门的方向计算一下大概的朝向
    if (m_cur_home_index >= m_homes.size()) {
        m_cur_home_index = 0;
    }
    Point home_loc(5, 5);
    if (m_cur_home_index < m_homes.size()) {
        home_loc = m_homes.at(m_cur_home_index);
    }
    Point home_point = m_side_tile_info.at(home_loc).pos;
    Rect home_rect(home_point.x, home_point.y, 1, 1);

    int dx = loc.x - home_loc.x;
    int dy = loc.y - home_loc.y;

    Point direction(0, 0);
    switch (role) {
    case Role::Medic:
    {
        if (std::abs(dx) < std::abs(dy)) {
            direction.y = -dy;
        }
        else {
            direction.x = -dx;
        }
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
            direction.y = dy;
        }
        else {
            direction.x = dx;
        }
    }
    break;
    }

    return direction;
}
