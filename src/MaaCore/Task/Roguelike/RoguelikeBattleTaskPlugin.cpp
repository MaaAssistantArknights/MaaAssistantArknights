#include "RoguelikeBattleTaskPlugin.h"

#include "Utils/Ranges.hpp"
#include <chrono>
#include <future>

#include "Utils/NoWarningCV.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/TilePack.h"
#include "Config/Roguelike/RoguelikeCopilotConfig.h"
#include "Config/TaskData.h"
#include "Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/Miscellaneous/BattleImageAnalyzer.h"
#include "Vision/Miscellaneous/BattleSkillReadyImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

using namespace asst::battle;
using namespace asst::battle::roguelike;

asst::RoguelikeBattleTaskPlugin::RoguelikeBattleTaskPlugin(const AsstCallback& callback, Assistant* inst,
                                                           std::string_view task_chain)
    : AbstractTaskPlugin(callback, inst, task_chain), BattleHelper(inst)
{}

bool asst::RoguelikeBattleTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    auto roguelike_name_opt = status()->get_properties(Status::RoguelikeTheme);
    if (!roguelike_name_opt) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = std::move(roguelike_name_opt.value()) + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@StartAction") {
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
    using namespace std::chrono_literals;

    bool gotten_info = get_stage_info();
    if (!gotten_info) {
        return true;
    }
    if (!wait_start()) {
        return false;
    }

    speed_up();

    bool timeout = false;
    int not_in_battle_count = 0;
    auto start_time = std::chrono::steady_clock::now();
    while (!need_exit()) {
        // 不在战斗场景，且已使用过了干员，说明已经打完了，就结束循环
        if (!auto_battle() && m_opers_used) {
            if (++not_in_battle_count > 5) {
                break;
            }
        }
        else {
            not_in_battle_count = 0;
        }
        if (std::chrono::steady_clock::now() - start_time > 8min) {
            timeout = true;
            break;
        }
    }
    // 超过时间限制了，一般是某个怪被干员卡住了，一直不结束。
    if (timeout) {
        Log.info("Timeout, retreat!");
        all_melee_retreat();

        not_in_battle_count = 0;
        start_time = std::chrono::steady_clock::now();
        while (!need_exit()) {
            if (!auto_battle()) {
                if (++not_in_battle_count > 5) {
                    break;
                }
            }
            else {
                not_in_battle_count = 0;
            }
            if (std::chrono::steady_clock::now() - start_time > 2min) {
                Log.info("Timeout again, abandon!");
                abandon();
                break;
            }
        }
    }

    clear();

    return true;
}

void asst::RoguelikeBattleTaskPlugin::wait_for_start()
{
    ProcessTask(*this, { "RoguelikeWaitBattleStart" }).set_task_delay(0).set_retry_times(0).run();
}

bool asst::RoguelikeBattleTaskPlugin::get_stage_info()
{
    LogTraceFunction;

    wait_for_start();

    bool calced = false;

    const auto stage_name_task_ptr = Task.get("BattleStageName");
    sleep(stage_name_task_ptr->pre_delay);

    constexpr int StageNameRetryTimes = 50;
    for (int i = 0; i != StageNameRetryTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        std::this_thread::yield();

        OcrWithPreprocessImageAnalyzer name_analyzer(ctrler()->get_image());
        name_analyzer.set_task_info(stage_name_task_ptr);
        if (!name_analyzer.analyze()) {
            continue;
        }
        name_analyzer.sort_result_by_score();
        const std::string& text = name_analyzer.get_result().front().text;
        if (text.empty()) {
            continue;
        }

        static const std::vector<std::string> RoguelikeStageCode = { "ISW-NO", "ISW-DF", "ISW-DU", "ISW-SP",
                                                                     std::string() };
        TilePack::LevelKey stage_key;
        stage_key.name = text;

        for (const std::string& code : RoguelikeStageCode) {
            stage_key.code = code;
            auto side_info = Tile.calc(stage_key, true);
            if (side_info.empty()) {
                continue;
            }
            m_stage_name = text;

            m_side_tile_info = std::move(side_info);
            m_normal_tile_info = Tile.calc(stage_key, false);
            calced = true;
            break;
        }

        if (calced) {
            break;
        }
    }

    if (!calced) {
        callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("StageInfoError"));
        return false;
    }

    auto opt = RoguelikeCopilot.get_stage_data(m_stage_name);
    if (opt && !opt->replacement_home.empty()) {
        m_homes = opt->replacement_home;
        auto homes_pos = m_homes | views::transform(&ReplacementHome::location);
        auto invalid_homes_pos =
            homes_pos | views::filter([&](const auto& home_pos) { return !m_normal_tile_info.contains(home_pos); }) |
            views::transform(&Point::to_string);
        if (!invalid_homes_pos.empty()) {
            Log.error("No replacement homes point:", invalid_homes_pos);
        }
        Log.info("replacement home:", homes_pos | views::transform(&Point::to_string));
        m_blacklist_location = opt->blacklist_location;
        m_stage_use_dice = opt->use_dice_stage;
        m_role_order = opt->role_order;
        m_force_air_defense.stop_blocking_deploy_num = opt->stop_deploy_blocking_num;
        m_force_air_defense.deploy_air_defense_num = opt->force_deploy_air_defense_num;
        m_force_air_defense.ban_medic = opt->force_ban_medic;
        m_force_deploy_direction = opt->force_deploy_direction;
    }
    else {
        for (const auto& [loc, side] : m_normal_tile_info) {
            if (side.key == TilePack::TileKey::Home) {
                m_homes.emplace_back(ReplacementHome { loc, battle::DeployDirection::None });
            }
        }
        m_stage_use_dice = true;
        m_role_order = {
            battle::Role::Warrior, battle::Role::Pioneer, battle::Role::Medic,
            battle::Role::Tank,    battle::Role::Sniper,  battle::Role::Caster,
            battle::Role::Support, battle::Role::Special, battle::Role::Drone,
        };
    }
    m_wait_blocking.assign(m_homes.size(), true);
    m_wait_medic.assign(m_homes.size(), true);
    m_indeed_no_medic.assign(m_homes.size(), false);
    // 认为开局时未在所有路线放干员时为紧急状态，并把路线压入栈
    // 由于开局路线设为0，故0不需要被压入栈
    m_cur_home_index = 0;
    m_is_cur_urgent = true;
    for (size_t index = m_homes.size() - 1; index > 0; index--) {
        m_next_urgent_index.push(index);
    }
    if (opt && !opt->key_kills.empty()) {
        std::string log_str = "[ ";
        for (const auto& kills : opt->key_kills) {
            m_key_kills.emplace(kills);
            log_str += std::to_string(kills) + ", ";
        }
        log_str += "]";
        Log.info("key kills:", log_str);
    }

    if (m_homes.empty()) {
        Log.error("Unknown home pos");
    }

    auto cb_info = basic_info_with_what("StageInfo");
    auto& details = cb_info["details"];
    details["name"] = m_stage_name;
    details["size"] = m_side_tile_info.size();
    callback(AsstMsg::SubTaskExtraInfo, cb_info);

    return true;
}

bool asst::RoguelikeBattleTaskPlugin::battle_pause()
{
    return ProcessTask(*this, { "BattlePause" }).run();
}

asst::battle::LocationType asst::RoguelikeBattleTaskPlugin::get_role_location_type(const battle::Role& role)
{
    switch (role) {
    case battle::Role::Medic:
    case battle::Role::Support:
    case battle::Role::Sniper:
    case battle::Role::Caster:
        return battle::LocationType::Ranged;
        break;
    case battle::Role::Pioneer:
    case battle::Role::Warrior:
    case battle::Role::Tank:
    case battle::Role::Special:
    case battle::Role::Drone:
        return battle::LocationType::Melee;
        break;
    default:
        return battle::LocationType::None;
        break;
    }
}

asst::battle::LocationType asst::RoguelikeBattleTaskPlugin::get_oper_location_type(const std::string& name)
{
    return BattleData.get_location_type(name);
}

asst::battle::OperPosition asst::RoguelikeBattleTaskPlugin::get_role_position(const battle::Role& role)
{
    switch (role) {
    case battle::Role::Support:
    case battle::Role::Sniper:
    case battle::Role::Caster:
        return battle::OperPosition::AirDefense;
        break;
    case battle::Role::Pioneer:
    case battle::Role::Warrior:
    case battle::Role::Tank:
        return battle::OperPosition::Blocking;
        break;
    case battle::Role::Medic:
    case battle::Role::Special:
    case battle::Role::Drone:
    default:
        return battle::OperPosition::None;
        break;
    }
}

void asst::RoguelikeBattleTaskPlugin::set_position_full(const battle::LocationType& loc_type, bool full)
{
    switch (loc_type) {
    case battle::LocationType::Melee:
        m_melee_full = full;
        break;
    case battle::LocationType::Ranged:
        m_ranged_full = full;
        break;
    case battle::LocationType::All:
        m_melee_full = full;
        m_ranged_full = full;
        break;
    case battle::LocationType::Invalid:
    case battle::LocationType::None:
    default:
        break;
    }
}

void asst::RoguelikeBattleTaskPlugin::set_position_full(const Point& point, bool full)
{
    if (auto tile_iter = m_normal_tile_info.find(point); tile_iter != m_normal_tile_info.end()) {
        set_position_full(tile_iter->second.buildable, full);
    }
}

void asst::RoguelikeBattleTaskPlugin::set_position_full(const battle::Role& role, bool full)
{
    set_position_full(get_role_location_type(role), full);
}

void asst::RoguelikeBattleTaskPlugin::set_position_full(const std::string& name, bool full)
{
    set_position_full(get_oper_location_type(name), full);
}

bool asst::RoguelikeBattleTaskPlugin::get_position_full(const battle::Role& role)
{
    const auto& loc_type = get_role_location_type(role);
    switch (loc_type) {
    case battle::LocationType::Melee:
        return m_melee_full;
        break;
    case battle::LocationType::Ranged:
        return m_ranged_full;
        break;
    case battle::LocationType::All:
        return m_melee_full && m_ranged_full;
        break;
    case battle::LocationType::Invalid:
    case battle::LocationType::None:
    default:
        break;
    }
    return false;
}

bool asst::RoguelikeBattleTaskPlugin::auto_battle()
{
    LogTraceFunction;

    // 将存在于场上超过限时的召唤物所在的地块重新设为可用
    Time_Point now_time = std::chrono::system_clock::now();
    while ((!m_need_clear_tiles.empty()) && m_need_clear_tiles.top().placed_time < now_time) {
        const auto& placed_loc = m_need_clear_tiles.top().placed_loc;
        if (auto iter = m_used_tiles.find(placed_loc); iter != m_used_tiles.end()) {
            Log.info("Drone at location (", placed_loc.x, ",", placed_loc.y, ") is recognized as retreated");
            set_position_full(placed_loc, false);
            m_opers_in_field.erase(iter->second);
            m_used_tiles.erase(iter);
        }
        m_need_clear_tiles.pop();
    }

    const cv::Mat& image = ctrler()->get_image();
    if (!m_first_deploy && try_possible_skill(image)) {
        return true;
    }

    BattleImageAnalyzer battle_analyzer(image);
    battle_analyzer.set_target(BattleImageAnalyzer::Target::Roguelike);

    if (!battle_analyzer.analyze()) {
        return false;
    }

    battle_analyzer.sort_opers_by_cost();
    auto opers = battle_analyzer.get_opers();
    if (opers.empty()) {
        return true;
    }
    battle_analyzer.set_target(BattleImageAnalyzer::Target::Oper);

    if (m_cur_home_index >= m_homes.size()) {
        m_cur_home_index = 0;
    }

    int available_count = 0;
    int cooling_count = 0;
    std::vector<size_t> new_urgent;
    std::vector<std::string> cooling_opers;
    const auto use_oper_task_ptr = Task.get("BattleUseOper");
    bool has_dice = false;
    battle::DeploymentOper dice;
    for (const auto& oper : opers) {
        if (oper.cooling) cooling_count++;
        if (oper.available) available_count++;
    }
    for (auto& oper : opers) {
        if (oper.role != battle::Role::Drone) {
            continue;
        }
        if (m_dice_image.empty()) {
            continue;
        }
        MatchImageAnalyzer dice_analyzer(oper.avatar);
        dice_analyzer.set_task_info("DiceAvatarMatch");
        dice_analyzer.set_templ(m_dice_image);
        if (!dice_analyzer.analyze()) {
            continue;
        }
        has_dice = true;
        oper.name = Dice;
        dice = oper;
        break;
    }

    bool can_use_dice = m_stage_use_dice && has_dice;
    if (!can_use_dice) {
        m_use_dice = false;
    }
    // 如果发现有新撤退，就更新m_retreated_opers
    // 如果发现有新转好的，只更新m_last_cooling_count，在部署时从set中删去
    if (cooling_count > m_last_cooling_count && !m_first_deploy) {
        battle_pause();
        int remain_add = cooling_count - m_last_cooling_count;
        for (size_t i = 0; i < opers.size(); ++i) {
            const auto& cur_opers = battle_analyzer.get_opers();
            if (cur_opers.empty()) {
                // 只会在战斗结束时发生，战斗结束后cur_opers为空，导致访问越界
                return true;
            }
            size_t offset = opers.size() > cur_opers.size() ? opers.size() - cur_opers.size() : 0;
            const auto& oper = cur_opers.at(i - offset);
            if ((!oper.cooling) || oper.role == battle::Role::Drone) {
                continue;
            }
            ctrler()->click(oper.rect);
            sleep(use_oper_task_ptr->pre_delay);
            const cv::Mat& new_image = ctrler()->get_image();

            OcrWithPreprocessImageAnalyzer oper_name_analyzer(new_image);
            oper_name_analyzer.set_task_info("BattleOperName");
            oper_name_analyzer.set_replace(Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map);

            std::string oper_name = UnknownName;
            if (oper_name_analyzer.analyze()) {
                oper_name_analyzer.sort_result_by_score();
                oper_name = oper_name_analyzer.get_result().front().text;
            }

            battle_analyzer.set_image(new_image);
            battle_analyzer.analyze();
            battle_analyzer.sort_opers_by_cost();

            if (is_oper_name_error(oper_name)) {
                continue;
            }

            if (m_retreated_opers.contains(oper_name)) {
                continue;
            }
            auto iter = m_opers_in_field.find(oper_name);
            if (iter == m_opers_in_field.end()) {
                continue;
            }

            Log.info(oper_name, "retreated");
            if (auto del_pos_blocking = m_blocking_for_home_index.find(iter->second);
                del_pos_blocking != m_blocking_for_home_index.end()) {
                m_wait_blocking[del_pos_blocking->second] = true;
                new_urgent.emplace_back(del_pos_blocking->second);
                m_blocking_for_home_index.erase(del_pos_blocking);
            }
            else if (auto del_pos_medic = m_medic_for_home_index.find(iter->second);
                     del_pos_medic != m_medic_for_home_index.end()) {
                for (const size_t& home_index : del_pos_medic->second) {
                    m_wait_medic[home_index] = true;
                }
                m_medic_for_home_index.erase(del_pos_medic);
            }
            if (auto del_pos_tiles = m_used_tiles.find(iter->second); del_pos_tiles != m_used_tiles.end()) {
                if (m_normal_tile_info[del_pos_tiles->first].buildable == battle::LocationType::Melee) {
                    m_force_air_defense.has_deployed_blocking_num--;
                }
                m_used_tiles.erase(del_pos_tiles);
            }
            set_position_full(iter->second, false);
            m_opers_in_field.erase(iter);
            m_retreated_opers.emplace(oper_name);
            remain_add--;
            if (!remain_add) break;
        }
        battle_pause();
        cancel_oper_selection();
    }
    m_last_cooling_count = cooling_count;
    if (!new_urgent.empty()) {
        // 出现新的紧急情况，立即切到这条线路，并把其他紧急情况压入栈
        Log.info("New urgent situation detected");
        if (m_is_cur_urgent) {
            m_next_urgent_index.push(m_cur_home_index);
        }
        else {
            m_last_not_urgent = static_cast<int>(m_cur_home_index);
        }
        m_cur_home_index = new_urgent.at(0);
        for (size_t i = 1; i < new_urgent.size(); ++i) {
            m_next_urgent_index.push(new_urgent.at(i));
        }
        if (can_use_dice) {
            // 先放骰子，再放近战
            m_use_dice = true;
            m_next_urgent_index.push(new_urgent.at(0));
        }
    }
    else if ((!m_is_cur_urgent) && m_used_tiles.size() >= 2) {
        // 超过一半的人费用都没好，且没有紧急情况，那就不下人
        size_t not_cooling_count = opers.size() - cooling_count;
        if (available_count <= not_cooling_count / 2) {
            Log.trace("already used", m_used_tiles.size(), ", now_total", opers.size(), ", available", available_count,
                      ", not_cooling", not_cooling_count);
            return true;
        }
    }

    const auto swipe_oper_task_ptr = Task.get("BattleSwipeOper");

    // 点击当前最合适的干员
    battle::DeploymentOper opt_oper;
    bool oper_found = false;

    bool has_blocking = false;
    bool has_medic = false;
    bool wait_blocking = m_wait_blocking[m_cur_home_index];
    bool wait_medic = m_wait_medic[m_cur_home_index];
    bool force_need_air_defense =
        (!m_force_air_defense.has_finished_deploy_air_defense) &&
        (m_force_air_defense.has_deployed_blocking_num >= m_force_air_defense.stop_blocking_deploy_num) &&
        (!m_use_dice);
    if (m_use_dice) {
        opt_oper = std::move(dice);
        oper_found = true;
        if (available_locations(battle::LocationType::Melee).empty()) {
            m_melee_full = true;
            Log.info("Tiles full");
            return true;
        }
    }
    else {
        // 对于每个蓝门，先下个地面单位（如果有的话）
        // 第二个人下奶（如果有的话）
        bool has_air_defense = false;
        for (const auto& op : opers) {
            if (op.cooling) {
                continue;
            }
            if (op.role == battle::Role::Medic) {
                has_medic = true;
            }
            battle::OperPosition position = get_role_position(op.role);
            if (position == battle::OperPosition::Blocking) {
                has_blocking = true;
            }
            else if (position == battle::OperPosition::AirDefense) {
                if (m_force_air_defense.ban_medic && op.role == battle::Role::Medic) {
                    continue;
                }
                has_air_defense = true;
            }
        }

        wait_blocking &= has_blocking;
        wait_medic &= has_medic;
        if (force_need_air_defense) {
            Log.info("RANGED ROLE IS NEEDED UNDER FORCE");
            if (!has_air_defense) {
                m_force_air_defense.has_finished_deploy_air_defense = true;
                Log.info("FORCE RANGED OPER DEPLOY END");
                return true;
            }
        }
        for (auto role : m_role_order) {
            battle::OperPosition position = get_role_position(role);
            if (force_need_air_defense) {
                if (position != battle::OperPosition::AirDefense) continue;
                if (m_force_air_defense.ban_medic && role == battle::Role::Medic) continue;
            }
            else {
                if (wait_blocking) {
                    if (position != battle::OperPosition::Blocking) continue;
                }
                else if (wait_medic) {
                    if (role != battle::Role::Medic) {
                        continue;
                    }
                }
            }

            if (get_position_full(role)) {
                continue;
            }

            for (const auto& oper : opers) {
                if (!oper.available) {
                    continue;
                }
                if (oper.name == Dice) {
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

        // 预计算干员是否有点地方放
        if (available_locations(opt_oper.role).empty()) {
            set_position_full(opt_oper.role, true);
            Log.info("Tiles full");
            if (force_need_air_defense) {
                m_force_air_defense.has_finished_deploy_air_defense = true;
                Log.info("FORCE RANGED OPER DEPLOY END");
            }
            return true;
        }

        if (m_first_deploy) {
            m_first_deploy = false;
            battle_pause();
            bool clicked_drone = false;
            for (size_t i = 0; i < opers.size(); ++i) {
                const auto& cur_opers = battle_analyzer.get_opers();
                if (cur_opers.empty()) {
                    // 只会在战斗结束时发生，战斗结束后cur_opers为空，导致访问越界
                    return true;
                }
                size_t offset = opers.size() > cur_opers.size() ? opers.size() - cur_opers.size() : 0;
                const auto& oper = cur_opers.at(i - offset);
                if (oper.role == battle::Role::Drone) {
                    clicked_drone = true;
                    ctrler()->click(oper.rect);
                    sleep(use_oper_task_ptr->pre_delay);
                    const cv::Mat& new_image = ctrler()->get_image();

                    OcrWithPreprocessImageAnalyzer oper_name_analyzer(new_image);
                    oper_name_analyzer.set_task_info("BattleOperName");
                    oper_name_analyzer.set_replace(Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map);

                    battle_analyzer.set_image(new_image);
                    battle_analyzer.analyze();
                    battle_analyzer.sort_opers_by_cost();

                    if (!oper_name_analyzer.analyze()) continue;
                    oper_name_analyzer.sort_result_by_score();
                    if (oper_name_analyzer.get_result().front().text.find(Dice) != std::string::npos) {
                        m_dice_image = opers.at(i).avatar;
                        Log.info("Dice detected");
                        break;
                    }
                }
            }
            battle_pause();
            if (clicked_drone) {
                cancel_oper_selection();
            }
        }

        ctrler()->click(opt_oper.rect);
        sleep(use_oper_task_ptr->pre_delay);

        OcrWithPreprocessImageAnalyzer oper_name_analyzer(ctrler()->get_image());
        oper_name_analyzer.set_task_info("BattleOperName");
        oper_name_analyzer.set_replace(Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map);

        opt_oper.name = UnknownName;
        if (oper_name_analyzer.analyze()) {
            oper_name_analyzer.sort_result_by_score();
            opt_oper.name = oper_name_analyzer.get_result().front().text;
        }
        if (opt_oper.name == "阿米娅" && opt_oper.role == battle::Role::Warrior) {
            opt_oper.name = "阿米娅-WARRIOR";
        }

        if (!is_oper_name_error(opt_oper.name)) {
            auto real_loc_type = get_oper_location_type(opt_oper.name);
            if (real_loc_type != battle::LocationType::Invalid && // 说明名字识别错了
                real_loc_type != battle::LocationType::All && real_loc_type != get_role_location_type(opt_oper.role)) {
                // 重新计算干员是否有地方放
                if (available_locations(opt_oper.name).empty()) {
                    set_position_full(opt_oper.name, true);
                    Log.info("re-calc available loc, Tiles full");
                    // TODO: 这里可能存在一个问题：
                    // 如果有地面位置，但没高台位置了，尝试“高台先锋”这种职业时
                    // 前面的逻辑会因为他是“先锋”点开他，但是识别到名字确定是高台单位后，又会取消
                    // 会一直循环上面的这个操作
                    cancel_oper_selection();
                    return true;
                }
            }
        }

        if (opt_oper.name == Dice) {
            // 在drone被认为是近战时必须加上
            cancel_oper_selection();
            return true;
        }
    }

    // 计算最优部署位置及方向
    const auto& [placed_loc, direction] = calc_best_plan(opt_oper);
    if (placed_loc == Point::zero() && direction == Point::zero()) {
        Log.info("Tiles full while calc best plan.");
        cancel_oper_selection();
        return true;
    }

    // 将干员拖动到场上
    Point placed_point = m_side_tile_info.at(placed_loc).pos;
    Rect placed_rect(placed_point.x, placed_point.y, 1, 1);
    int dist = static_cast<int>(Point::distance(
        placed_point, { opt_oper.rect.x + opt_oper.rect.width / 2, opt_oper.rect.y + opt_oper.rect.height / 2 }));
    // 1000 是随便取的一个系数，把整数的 pre_delay 转成小数用的
    int duration = static_cast<int>(dist / 1000.0 * swipe_oper_task_ptr->pre_delay);
    bool deploy_with_pause = ctrler()->support_swipe_with_pause();
    ctrler()->swipe(opt_oper.rect, placed_rect, duration, false, swipe_oper_task_ptr->special_params.at(1),
                    swipe_oper_task_ptr->special_params.at(2), deploy_with_pause);
    sleep(use_oper_task_ptr->post_delay);

    // 将方向转换为实际的 swipe end 坐标点
    if (direction != Point::zero()) {
        static const int coeff = swipe_oper_task_ptr->special_params.at(0);
        Point end_point = placed_point + (direction * coeff);
        ctrler()->swipe(placed_point, end_point, swipe_oper_task_ptr->post_delay);
        sleep(use_oper_task_ptr->post_delay);
    }

    if (opt_oper.role == battle::Role::Drone) {
        cancel_oper_selection();
        now_time = std::chrono::system_clock::now();
        if (opt_oper.name == Dice) {
            m_need_clear_tiles.emplace(now_time + std::chrono::seconds(20), placed_loc);
        }
        else {
            m_need_clear_tiles.emplace(now_time + std::chrono::seconds(35), placed_loc);
        }
    }
    if (deploy_with_pause) {
        ctrler()->press_esc();
    }

    Log.info("Try to deploy oper", opt_oper.name);
    m_opers_used = true;
    m_used_tiles.emplace(placed_loc, opt_oper.name);
    m_opers_in_field.emplace(opt_oper.name, placed_loc);
    m_retreated_opers.erase(opt_oper.name);
    if (get_role_position(opt_oper.role) == battle::OperPosition::Blocking) {
        m_force_air_defense.has_deployed_blocking_num++;
    }
    if (force_need_air_defense) {
        m_force_air_defense.has_deployed_air_defense_num++;
        if (m_force_air_defense.has_deployed_air_defense_num >= m_force_air_defense.deploy_air_defense_num) {
            m_force_air_defense.has_finished_deploy_air_defense = true;
            Log.info("FORCE RANGED OPER DEPLOY END");
        }
        // 不改变当前index，直接进入下一步
        return true;
    }

    if (wait_blocking) {
        m_wait_blocking[m_cur_home_index] = false;
        m_blocking_for_home_index.emplace(placed_loc, m_cur_home_index);
    }
    else if (wait_medic) {
        // 两次轮到wait_medic的轮次时仍没有医疗干员奶当前位置，即认为奶不到
        if (m_wait_medic[m_cur_home_index]) {
            m_indeed_no_medic[m_cur_home_index] = false;
        }
        else {
            if (m_indeed_no_medic[m_cur_home_index]) {
                m_wait_medic[m_cur_home_index] = false;
            }
            else {
                m_indeed_no_medic[m_cur_home_index] = true;
            }
        }
    }

    // 如果有紧急需要部署的路线，那么下一条路线设为它
    if (m_next_urgent_index.empty()) {
        if (m_last_not_urgent != -1) {
            // 从上一次不是紧急的下一条路线开始
            m_cur_home_index = static_cast<size_t>(m_last_not_urgent) + 1;
            m_last_not_urgent = -1;
        }
        else {
            ++m_cur_home_index;
        }
        m_is_cur_urgent = false;
    }
    else {
        if (m_last_not_urgent == -1) {
            m_last_not_urgent = static_cast<int>(m_cur_home_index);
        }
        m_is_cur_urgent = true;
        m_cur_home_index = m_next_urgent_index.top();
        Log.info("Enter urgent situation");
        m_next_urgent_index.pop();
    }
    Log.info("To path", m_cur_home_index);

    return true;
}

bool asst::RoguelikeBattleTaskPlugin::speed_up()
{
    return ProcessTask(*this, { "RoguelikeBattleSpeedUp" }).run();
}

bool asst::RoguelikeBattleTaskPlugin::use_skill(const Rect& rect)
{
    ctrler()->click(rect);
    sleep(Task.get("BattleUseOper")->pre_delay);

    ProcessTask task(*this, { "BattleUseSkillJustClick" });
    task.set_retry_times(0);
    return task.run();
}

bool asst::RoguelikeBattleTaskPlugin::retreat(const Point& point)
{
    ctrler()->click(point);
    sleep(Task.get("BattleUseOper")->pre_delay);

    return ProcessTask(*this, { "BattleOperRetreatJustClick" }).run();
}

bool asst::RoguelikeBattleTaskPlugin::abandon()
{
    return ProcessTask(*this, { "RoguelikeBattleExitBegin" }).run();
}

void asst::RoguelikeBattleTaskPlugin::all_melee_retreat()
{
    for (const auto& loc : m_used_tiles | views::keys) {
        auto& tile_info = m_normal_tile_info[loc];
        auto& type = tile_info.buildable;
        if (type == battle::LocationType::Melee || type == battle::LocationType::All) {
            retreat(tile_info.pos);
        }
    }
}

void asst::RoguelikeBattleTaskPlugin::clear()
{
    m_opers_used = false;
    m_pre_hp = 0;
    m_homes.clear();
    m_wait_blocking.clear();
    m_wait_medic.clear();
    m_indeed_no_medic.clear();
    m_blocking_for_home_index.clear();
    m_medic_for_home_index.clear();
    decltype(m_next_urgent_index) empty_stack;
    m_next_urgent_index.swap(empty_stack);
    m_is_cur_urgent = false;
    m_last_not_urgent = -1;
    m_blacklist_location.clear();
    m_retreated_opers.clear();
    decltype(m_need_clear_tiles) empty_heap;
    m_need_clear_tiles.swap(empty_heap);
    decltype(m_key_kills) empty_queue;
    m_key_kills.swap(empty_queue);
    m_cur_home_index = 0;
    m_stage_name.clear();
    m_side_tile_info.clear();
    m_used_tiles.clear();
    m_opers_in_field.clear();
    m_kills = 0;
    m_total_kills = 0;
    m_use_dice = false;
    m_stage_use_dice = true;
    m_dice_image = cv::Mat();
    m_first_deploy = true;
    m_force_air_defense.clear();
    m_last_cooling_count = 0;
    m_force_deploy_direction.clear();
    m_melee_full = false;
    m_ranged_full = false;

    for (auto& [key, status_value] : m_restore_status) {
        status()->set_number(key, status_value);
    }
    m_restore_status.clear();
}

bool asst::RoguelikeBattleTaskPlugin::try_possible_skill(const cv::Mat& image)
{
    if (!check_key_kills(image)) {
        return false;
    }

    BattleSkillReadyImageAnalyzer skill_analyzer(image);
    bool used = false;
    for (auto& [loc, oper_name] : m_used_tiles) {
        std::string status_key = Status::RoguelikeSkillUsagePrefix + oper_name;
        auto usage = battle::SkillUsage::Possibly;
        auto usage_opt = status()->get_number(status_key);
        if (usage_opt) {
            usage = static_cast<battle::SkillUsage>(usage_opt.value());
        }

        if (usage != battle::SkillUsage::Possibly && usage != battle::SkillUsage::Once) {
            continue;
        }
        const Point pos = m_normal_tile_info.at(loc).pos;
        skill_analyzer.set_base_point(pos);
        if (!skill_analyzer.analyze()) {
            continue;
        }
        ctrler()->click(pos);
        sleep(Task.get("BattleUseOper")->pre_delay);
        bool ret = ProcessTask(*this, { "BattleSkillReadyOnClick" }).set_retry_times(2).run();
        if (!ret) {
            cancel_oper_selection();
        }
        used |= ret;
        if (usage == battle::SkillUsage::Once) {
            status()->set_number(status_key, static_cast<int64_t>(battle::SkillUsage::OnceUsed));
            m_restore_status[status_key] = static_cast<int64_t>(battle::SkillUsage::Once);
        }
    }
    return used;
}

bool asst::RoguelikeBattleTaskPlugin::check_key_kills(const cv::Mat& image)
{
    if (m_key_kills.empty()) {
        return true;
    }
    int need_kills = m_key_kills.front();

    BattleImageAnalyzer analyzer(image);
    if (m_total_kills) {
        analyzer.set_pre_total_kills(m_total_kills);
    }
    analyzer.set_target(BattleImageAnalyzer::Target::Kills);
    if (analyzer.analyze()) {
        m_kills = analyzer.get_kills();
        m_total_kills = analyzer.get_total_kills();
        if (m_kills >= need_kills) {
            m_key_kills.pop();
            return true;
        }
    }
    return false;
}

bool asst::RoguelikeBattleTaskPlugin::wait_start()
{
    auto start_time = std::chrono::system_clock::now();
    auto check_time = [&]() -> bool {
        using namespace std::chrono_literals;
        return std::chrono::system_clock::now() - start_time > 1min;
    };

    MatchImageAnalyzer officially_begin_analyzer;
    officially_begin_analyzer.set_task_info("BattleOfficiallyBegin");
    cv::Mat image;
    while (!need_exit() && !check_time()) {
        image = ctrler()->get_image();
        officially_begin_analyzer.set_image(image);
        if (officially_begin_analyzer.analyze()) {
            break;
        }
        std::this_thread::yield();
    }

    BattleImageAnalyzer oper_analyzer;
    oper_analyzer.set_target(BattleImageAnalyzer::Target::Oper);
    while (!need_exit() && !check_time()) {
        image = ctrler()->get_image();
        oper_analyzer.set_image(image);
        if (oper_analyzer.analyze()) {
            break;
        }
        std::this_thread::yield();
    }

    // 识别一帧总击杀数
    BattleImageAnalyzer kills_analyzer(image);
    kills_analyzer.set_target(BattleImageAnalyzer::Target::Kills);
    if (kills_analyzer.analyze()) {
        m_kills = kills_analyzer.get_kills();
        m_total_kills = kills_analyzer.get_total_kills();
    }

    if (!m_stage_name.empty()) {
        for (const auto& [loc, info] : m_normal_tile_info) {
            std::string text = "( " + std::to_string(loc.x) + ", " + std::to_string(loc.y) + " )";
            cv::putText(image, text, cv::Point(info.pos.x - 30, info.pos.y), 1, 1.2, cv::Scalar(0, 0, 255), 2);
        }
        asst::imwrite("map/" + m_stage_name + ".png", image);
    }
    else {
        // 存出来的是带时间戳的文件名
        kills_analyzer.save_img("map/");
    }
    return true;
}

bool asst::RoguelikeBattleTaskPlugin::cancel_oper_selection()
{
    return ProcessTask(*this, { "BattleCancelSelection" }).run();
}

bool asst::RoguelikeBattleTaskPlugin::is_oper_name_error(const std::string& name)
{
    return name == UnknownName || get_oper_location_type(name) == battle::LocationType::Invalid;
}

std::vector<asst::Point> asst::RoguelikeBattleTaskPlugin::available_locations(battle::Role role)
{
    return available_locations(get_role_location_type(role));
}

std::vector<asst::Point> asst::RoguelikeBattleTaskPlugin::available_locations(const std::string& name)
{
    return available_locations(get_oper_location_type(name));
}

std::vector<asst::Point> asst::RoguelikeBattleTaskPlugin::available_locations(battle::LocationType type)
{
    std::vector<Point> result;
    for (const auto& [loc, tile] : m_normal_tile_info) {
        bool position_mathced = tile.buildable == type || tile.buildable == battle::LocationType::All;
        position_mathced |= (type == battle::LocationType::All) && (tile.buildable == battle::LocationType::Melee ||
                                                                    tile.buildable == battle::LocationType::Ranged);
        if (position_mathced &&
            tile.key != TilePack::TileKey::DeepSea && // 水上要先放板子才能放人，肉鸽里也没板子，那就当作不可放置
            !m_used_tiles.contains(loc) && !m_blacklist_location.contains(loc)) {
            result.emplace_back(loc);
        }
    }
    return result;
}

asst::battle::AttackRange asst::RoguelikeBattleTaskPlugin::get_attack_range(const battle::DeploymentOper& oper)
{
    int64_t elite = status()->get_number(Status::RoguelikeCharElitePrefix + oper.name).value_or(0);
    battle::AttackRange right_attack_range = BattleData.get_range(oper.name, elite);

    if (right_attack_range == BattleDataConfig::EmptyRange) {
        switch (oper.role) {
        case battle::Role::Support:
            right_attack_range = {
                Point(-1, -1), Point(0, -1), Point(1, -1), Point(2, -1), //
                Point(-1, 0),  Point(0, 0),  Point(1, 0),  Point(2, 0),  //
                Point(-1, 1),  Point(0, 1),  Point(1, 1),  Point(2, 1),  //
            };
            break;
        case battle::Role::Caster:
            right_attack_range = {
                Point(0, -1), Point(1, -1), Point(2, -1),              //
                Point(0, 0),  Point(1, 0),  Point(2, 0),  Point(3, 0), //
                Point(0, 1),  Point(1, 1),  Point(2, 1),               //
            };
            break;
        case battle::Role::Medic:
        case battle::Role::Sniper:
            right_attack_range = {
                Point(0, -1), Point(1, -1), Point(2, -1), Point(3, -1), //
                Point(0, 0),  Point(1, 0),  Point(2, 0),  Point(3, 0),  //
                Point(0, 1),  Point(1, 1),  Point(2, 1),  Point(3, 1),  //
            };
            break;
        case battle::Role::Warrior:
            right_attack_range = { Point(0, 0), Point(1, 0), Point(2, 0) };
            break;
        case battle::Role::Special:
        case battle::Role::Tank:
        case battle::Role::Pioneer:
        case battle::Role::Drone:
            right_attack_range = { Point(0, 0), Point(1, 0) };
            break;
        default:
            break;
        }
    }

    return right_attack_range;
}

asst::RoguelikeBattleTaskPlugin::DeployInfo asst::RoguelikeBattleTaskPlugin::calc_best_plan(
    const battle::DeploymentOper& oper)
{
    if (m_cur_home_index >= m_homes.size()) {
        m_cur_home_index = 0;
    }

    Point home(5, 5); // 实在找不到家门了，随便取个点当家门用算了，一般是地图的中间
    Point recommended_direction;
    static const std::unordered_map<battle::DeployDirection, Point> direction_map = {
        { battle::DeployDirection::Up, Point::up() },     { battle::DeployDirection::Down, Point::down() },
        { battle::DeployDirection::Left, Point::left() }, { battle::DeployDirection::Right, Point::right() },
        { battle::DeployDirection::None, Point() },
    };
    if (m_cur_home_index < m_homes.size()) {
        const auto& rp_home = m_homes.at(m_cur_home_index);
        home = rp_home.location;
        recommended_direction = direction_map.at(rp_home.direction);
    }

    auto comp_dist = [&](const Point& lhs, const Point& rhs) -> bool {
        int lhs_y_dist = std::abs(lhs.y - home.y);
        int lhs_dist = std::abs(lhs.x - home.x) + lhs_y_dist;
        int rhs_y_dist = std::abs(rhs.y - home.y);
        int rhs_dist = std::abs(rhs.x - home.x) + rhs_y_dist;
        // 距离一样选择 x 轴上的，因为一般的地图都是横向的长方向
        return lhs_dist == rhs_dist ? lhs_y_dist < rhs_y_dist : lhs_dist < rhs_dist;
    };

    std::vector<Point> available_loc =
        is_oper_name_error(oper.name) ? available_locations(oper.role) : available_locations(oper.name);

    if (available_loc.empty()) {
        Log.error("No available locations");
        return {};
    }

    // 把所有可用的点按距离排个序
    ranges::sort(available_loc, comp_dist);

    if (oper.name == Dice) {
        return { available_loc.back(), Point::zero() };
    }

    Point best_location, best_direction;
    int max_score = INT_MIN;

    const auto& near_loc = available_loc.front();
    int min_dist = std::abs(near_loc.x - home.x) + std::abs(near_loc.y - home.y);

    // 取距离最近的N个点，计算分数。然后使用得分最高的点
    constexpr int CalcPointCount = 4;
    for (const auto& loc : available_loc | views::take(CalcPointCount)) {
        auto cur_result = calc_best_direction_and_score(loc, oper, recommended_direction);
        // 离得远的要扣分
        constexpr int DistWeights = -1050;
        int extra_dist = std::abs(loc.x - home.x) + std::abs(loc.y - home.y) - min_dist;
        int extra_dist_score = DistWeights * extra_dist;
        if (oper.role == battle::Role::Medic) { // 医疗干员离得远无所谓
            extra_dist_score = 0;
        }

        if (cur_result.second + extra_dist_score > max_score) {
            max_score = cur_result.second + extra_dist_score;
            best_location = loc;
            best_direction = cur_result.first;
        }
    }

    // 强制变化为确定的攻击方向
    if (auto iter = m_force_deploy_direction.find(best_location); iter != m_force_deploy_direction.end()) {
        if (iter->second.role.contains(oper.role)) {
            best_direction = direction_map.at(iter->second.direction);
        }
    }

    // 如果是医疗干员，判断覆盖范围内有无第一次放置的干员
    if (oper.role == battle::Role::Medic) {
        battle::AttackRange right_attack_range = get_attack_range(oper);
        for (const Point& direction : { Point::right(), Point::up(), Point::left(), Point::down() }) {
            if (direction == best_direction) break;
            for (Point& point : right_attack_range)
                point = { point.y, -point.x };
        }
        std::vector<size_t> contain_index;
        for (const Point& relative_pos : right_attack_range) {
            Point absolute_pos = best_location + relative_pos;
            if (auto iter = m_blocking_for_home_index.find(absolute_pos); iter != m_blocking_for_home_index.end()) {
                m_wait_medic[iter->second] = false;
                contain_index.emplace_back(iter->second);
            }
        }
        if (!contain_index.empty()) {
            m_medic_for_home_index.emplace(best_location, std::move(contain_index));
        }
    }

    return { best_location, best_direction };
}

std::pair<asst::Point, int> asst::RoguelikeBattleTaskPlugin::calc_best_direction_and_score(
    Point loc, const battle::DeploymentOper& oper, Point recommended_direction)
{
    LogTraceFunction;

    // 根据家门的方向计算一下大概的朝向
    if (m_cur_home_index >= m_homes.size()) {
        m_cur_home_index = 0;
    }
    Point home_loc(5, 5);
    if (m_cur_home_index < m_homes.size()) {
        home_loc = m_homes.at(m_cur_home_index).location;
    }

    auto sgn = [](const int& x) -> int {
        if (x > 0) return 1;
        if (x < 0) return -1;
        return 0;
    };

    Point base_direction(0, 0);
    if (loc.x == home_loc.x) {
        base_direction.y = sgn(loc.y - home_loc.y);
    }
    else {
        base_direction.x = sgn(loc.x - home_loc.x);
    }
    Point home_direction(-base_direction.x, -base_direction.y);
    // 医疗反着算
    if (oper.role == battle::Role::Medic) {
        base_direction = -base_direction;
    }

    // 按朝右算，后面根据方向做转换
    battle::AttackRange right_attack_range = get_attack_range(oper);

    int max_score = 0;
    Point opt_direction;

    for (const Point& direction : { Point::right(), Point::up(), Point::left(), Point::down() }) {
        int score = 0;
        for (const Point& relative_pos : right_attack_range) {
            Point absolute_pos = loc + relative_pos;

            using TileKey = TilePack::TileKey;
            // 战斗干员朝向的权重
            static const std::unordered_map<TileKey, int> TileKeyFightWeights = {
                { TileKey::Invalid, 0 },    { TileKey::Forbidden, 0 },  { TileKey::Wall, 500 },
                { TileKey::Road, 1000 },    { TileKey::Home, 500 },     { TileKey::EnemyHome, 1000 },
                { TileKey::Airport, 1000 }, { TileKey::Floor, 1000 },   { TileKey::Hole, 0 },
                { TileKey::Telin, 700 },    { TileKey::Telout, 800 },   { TileKey::Grass, 500 },
                { TileKey::DeepSea, 1000 }, { TileKey::Volcano, 1000 }, { TileKey::Healing, 1000 },
                { TileKey::Fence, 800 },
            };
            // 治疗干员朝向的权重
            static const std::unordered_map<TileKey, int> TileKeyMedicWeights = {
                { TileKey::Invalid, 0 },  { TileKey::Forbidden, 0 },  { TileKey::Wall, 1000 },
                { TileKey::Road, 1000 },  { TileKey::Home, 0 },       { TileKey::EnemyHome, 0 },
                { TileKey::Airport, 0 },  { TileKey::Floor, 0 },      { TileKey::Hole, 0 },
                { TileKey::Telin, 0 },    { TileKey::Telout, 0 },     { TileKey::Grass, 500 },
                { TileKey::DeepSea, 0 },  { TileKey::Volcano, 1000 }, { TileKey::Healing, 1000 },
                { TileKey::Fence, 1000 },
            };

            switch (oper.role) {
            case battle::Role::Medic:
                if (auto iter = m_used_tiles.find(absolute_pos);
                    iter != m_used_tiles.cend() &&
                    BattleData.get_role(iter->second) != battle::Role::Drone) // 根据哪个方向上人多决定朝向哪
                    score += 10000;
                if (auto iter = m_side_tile_info.find(absolute_pos); iter != m_side_tile_info.end())
                    score += TileKeyMedicWeights.at(iter->second.key);
                break;
            default:
                if (auto iter = m_side_tile_info.find(absolute_pos); iter != m_side_tile_info.end())
                    score += TileKeyFightWeights.at(iter->second.key);
                break;
            }
        }

        if (direction == base_direction) {
            score += 300;
        }

        if (oper.role != battle::Role::Medic && direction == home_direction) {
            score -= 500;
        }

        if (oper.role != battle::Role::Medic && direction == recommended_direction) {
            score += 2000;
        }

        if (score > max_score) {
            max_score = score;
            opt_direction = direction;
        }

        // rotate relative attack range counterclockwise
        for (Point& point : right_attack_range)
            point = { point.y, -point.x };
    }

    return std::make_pair(opt_direction, max_score);
}
