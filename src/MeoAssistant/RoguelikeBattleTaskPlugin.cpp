#include "RoguelikeBattleTaskPlugin.h"

#include <chrono>
#include <future>
#include "AsstRanges.hpp"

#include "NoWarningCV.h"

#include "BattleImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"
#include "ProcessTask.h"
#include "OcrWithPreprocessImageAnalyzer.h"
#include "Resource.h"
#include "Logger.hpp"
#include "RuntimeStatus.h"
#include "MatchImageAnalyzer.h"

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
    auto start_time = std::chrono::steady_clock::now();
    while (!need_exit()) {
        // 不在战斗场景，且已使用过了干员，说明已经打完了，就结束循环
        if (!auto_battle() && m_opers_used) {
            break;
        }
        if (std::chrono::steady_clock::now() - start_time > 5min) {
            timeout = true;
            break;
        }
    }
    // 超过时间限制了，一般是某个怪被干员卡住了，一直不结束。
    if (timeout) {
        Log.info("Timeout, retreat!");
        all_melee_retreat();

        start_time = std::chrono::steady_clock::now();
        while (!need_exit()) {
            if (!auto_battle()) {
                break;
            }
            if (std::chrono::steady_clock::now() - start_time > 1min) {
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
    ProcessTask(*this, { "Roguelike1WaitBattleStart" }).set_task_delay(0).set_retry_times(0).run();
}

bool asst::RoguelikeBattleTaskPlugin::get_stage_info()
{
    LogTraceFunction;

    wait_for_start();

    const auto& tile = Resrc.tile();
    bool calced = false;

    if (m_stage_name.empty()) {
        const auto stage_name_task_ptr = Task.get("BattleStageName");
        sleep(stage_name_task_ptr->pre_delay);

        constexpr int StageNameRetryTimes = 50;
        for (int i = 0; i != StageNameRetryTimes; ++i) {
            cv::Mat image = m_ctrler->get_image();
            OcrWithPreprocessImageAnalyzer name_analyzer(image);

            name_analyzer.set_task_info(stage_name_task_ptr);
            if (!name_analyzer.analyze()) {
                continue;
            }

            for (const auto& tr : name_analyzer.get_result()) {
                auto side_info = tile.calc(tr.text, true);
                if (side_info.empty()) {
                    continue;
                }
                m_stage_name = tr.text;
                m_side_tile_info = std::move(side_info);
                m_normal_tile_info = tile.calc(m_stage_name, false);
                calced = true;
                break;
            }
            if (calced) {
                break;
            }
            std::this_thread::yield();
        }
    }
    else {
        m_side_tile_info = tile.calc(m_stage_name, true);
        m_normal_tile_info = tile.calc(m_stage_name, false);
        calced = true;
    }

    auto opt = Resrc.roguelike().get_stage_data(m_stage_name);
    if (opt && !opt->replacement_home.empty()) {
        m_homes = opt->replacement_home;
        std::string log_str = "[ ";
        for (auto& home : m_homes) {
            if (!m_normal_tile_info.contains(home)) {
                Log.error("No replacement home point", home.x, home.y);
            }
            log_str += "( " + std::to_string(home.x) + ", " + std::to_string(home.y) + " ), ";
        }
        log_str += "]";
        Log.info("replacement home:", log_str);
    }
    else {
        for (const auto& [loc, side] : m_normal_tile_info) {
            if (side.key == TilePack::TileKey::Home) {
                m_homes.emplace_back(loc);
            }
        }
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

    if (calced) {
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

    //if (int hp = battle_analyzer.get_hp();
    //    hp != 0) {
    //    bool used_skills = false;
    //    if (hp < m_pre_hp) {    // 说明漏怪了，漏怪就开技能（
    //        for (const Rect& rect : battle_analyzer.get_ready_skills()) {
    //            used_skills = true;
    //            if (!use_skill(rect)) {
    //                break;
    //            }
    //        }
    //    }
    //    m_pre_hp = hp;
    //    if (used_skills) {
    //        return true;
    //    }
    //}

    const cv::Mat& image = m_ctrler->get_image();
    if (try_possible_skill(image)) {
        return true;
    }

    BattleImageAnalyzer battle_analyzer(image);
    battle_analyzer.set_target(BattleImageAnalyzer::Target::Roguelike);

    if (!battle_analyzer.analyze()) {
        return false;
    }

    battle_analyzer.sort_opers_by_cost();
    const auto& opers = battle_analyzer.get_opers();
    if (opers.empty()) {
        return true;
    }
    // 如果已经放了一些人了，就不要再有费就下了
    if (m_used_tiles.size() >= std::max(m_homes.size(), static_cast<size_t>(2))) {
        size_t available_count = 0;
        size_t not_cooling_count = 0;
        for (const auto& oper : opers) {
            if (oper.available) {
                available_count += 1;
            }
            if (!oper.cooling) {
                not_cooling_count += 1;
            }
        }
        // 超过一半的人费用都没好，那就不下人
        if (available_count <= not_cooling_count / 2) {
            Log.trace("already used", m_used_tiles.size(), ", now_total", opers.size(),
                ", available", available_count, ", not_cooling", not_cooling_count);
            return true;
        }
    }

    static const std::array<BattleRole, 9> RoleOrder = {
        BattleRole::Warrior,
        BattleRole::Pioneer,
        BattleRole::Medic,
        BattleRole::Tank,
        BattleRole::Sniper,
        BattleRole::Caster,
        BattleRole::Support,
        BattleRole::Special,
        BattleRole::Drone
    };
    const auto use_oper_task_ptr = Task.get("BattleUseOper");
    const auto swipe_oper_task_ptr = Task.get("BattleSwipeOper");

    // 点击当前最合适的干员
    BattleRealTimeOper opt_oper;
    bool oper_found = false;

    // 一个人都没有的时候，先下个地面单位（如果有的话）
    // 第二个人下奶（如果有的话）
    bool wait_melee = false;
    bool wait_medic = false;
    if (m_used_tiles.empty() || m_used_tiles.size() == 1) {
        for (const auto& op : opers) {
            if (m_used_tiles.empty()) {
                if (op.role == BattleRole::Warrior ||
                    op.role == BattleRole::Pioneer ||
                    op.role == BattleRole::Tank) {
                    wait_melee = true;
                }
            }
            else if (m_used_tiles.size() == 1 && m_homes.size() == 1) {
                if (op.role == BattleRole::Medic) {
                    wait_medic = true;
                }
            }
        }
    }

    for (auto role : RoleOrder) {
        if (wait_melee) {
            if (role != BattleRole::Warrior &&
                role != BattleRole::Pioneer &&
                role != BattleRole::Tank) {
                continue;
            }
        }
        else if (wait_medic) {
            if (role != BattleRole::Medic) {
                continue;
            }
        }

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
    m_ctrler->click(opt_oper.rect);
    sleep(use_oper_task_ptr->pre_delay);

    OcrWithPreprocessImageAnalyzer oper_name_analyzer(m_ctrler->get_image());
    oper_name_analyzer.set_task_info("BattleOperName");
    oper_name_analyzer.set_replace(
        Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map);

    std::string oper_name = "Unknown";
    if (oper_name_analyzer.analyze()) {
        oper_name_analyzer.sort_result_by_score();
        oper_name = oper_name_analyzer.get_result().front().text;
        opt_oper.name = oper_name;
    }

    // 计算最优部署位置及方向
    const auto& [placed_loc, direction] = calc_best_plan(opt_oper);

    // 将干员拖动到场上
    Point placed_point = m_side_tile_info.at(placed_loc).pos;
    Rect placed_rect(placed_point.x, placed_point.y, 1, 1);
    int dist = static_cast<int>(Point::distance(
        placed_point, { opt_oper.rect.x + opt_oper.rect.width / 2, opt_oper.rect.y + opt_oper.rect.height / 2 }));
    // 1000 是随便取的一个系数，把整数的 pre_delay 转成小数用的
    int duration = static_cast<int>(swipe_oper_task_ptr->pre_delay / 800.0 * dist * log10(dist));
    m_ctrler->swipe(opt_oper.rect, placed_rect, duration, true, 0);
    sleep(use_oper_task_ptr->rear_delay);

    // 将方向转换为实际的 swipe end 坐标点
    constexpr int coeff = 500;
    Point end_point = placed_point + (direction * coeff);

    m_ctrler->swipe(placed_point, end_point, swipe_oper_task_ptr->rear_delay, true, 100);

    m_used_tiles.emplace(placed_loc, oper_name);
    m_opers_used = true;
    ++m_cur_home_index;

    return true;
}

bool asst::RoguelikeBattleTaskPlugin::speed_up()
{
    return ProcessTask(*this, { "Roguelike1BattleSpeedUp" }).run();
}

bool asst::RoguelikeBattleTaskPlugin::use_skill(const Rect& rect)
{
    m_ctrler->click(rect);
    sleep(Task.get("BattleUseOper")->pre_delay);

    ProcessTask task(*this, { "BattleUseSkill" });
    task.set_retry_times(0);
    return task.run();
}

bool asst::RoguelikeBattleTaskPlugin::retreat(const Point& point)
{
    m_ctrler->click(point);
    sleep(Task.get("BattleUseOper")->pre_delay);

    return ProcessTask(*this, { "BattleOperRetreat" }).run();
}

bool asst::RoguelikeBattleTaskPlugin::abandon()
{
    return ProcessTask(*this, { "Roguelike1BattleExitBegin" }).run();
}

void asst::RoguelikeBattleTaskPlugin::all_melee_retreat()
{
    for (const auto& loc : m_used_tiles | views::keys) {
        auto& tile_info = m_normal_tile_info[loc];
        auto& type = tile_info.buildable;
        if (type == Loc::Melee || type == Loc::All) {
            if (!retreat(tile_info.pos)) {
                continue;
            }
        }
    }
}

void asst::RoguelikeBattleTaskPlugin::clear()
{
    m_opers_used = false;
    m_pre_hp = 0;
    m_homes.clear();
    decltype(m_key_kills) empty;
    m_key_kills.swap(empty);
    m_cur_home_index = 0;
    m_stage_name.clear();
    m_side_tile_info.clear();
    m_used_tiles.clear();
    m_kills = 0;
    m_total_kills = 0;

    for (auto& [key, status] : m_restore_status) {
        m_status->set_number(key, status);
    }
    m_restore_status.clear();
}

bool asst::RoguelikeBattleTaskPlugin::try_possible_skill(const cv::Mat& image)
{
    if (!check_key_kills(image)) {
        return false;
    }

    auto task_ptr = Task.get("BattleAutoSkillFlag");
    const Rect& skill_roi_move = task_ptr->rect_move;

    MatchImageAnalyzer analyzer(image);
    analyzer.set_task_info(task_ptr);
    bool used = false;
    for (auto& [loc, name] : m_used_tiles) {
        std::string status_key = "RoguelikeSkillUsage-" + name;
        auto usage = BattleSkillUsage::Possibly;
        auto usage_opt = m_status->get_number(status_key);
        if (usage_opt) {
            usage = static_cast<BattleSkillUsage>(usage_opt.value());
        }

        if (usage != BattleSkillUsage::Possibly
            && usage != BattleSkillUsage::Once) {
            continue;
        }
        const Point pos = m_normal_tile_info.at(loc).pos;
        const Rect pos_rect(pos.x, pos.y, 1, 1);
        const Rect roi = pos_rect.move(skill_roi_move);
        analyzer.set_roi(roi);
        if (!analyzer.analyze()) {
            continue;
        }
        m_ctrler->click(pos_rect);
        sleep(Task.get("BattleUseOper")->pre_delay);
        bool ret = ProcessTask(*this, { "BattleSkillReadyOnClick" }).set_retry_times(2).run();
        if (!ret) {
            ProcessTask(*this, { "BattleCancelSelection" }).set_retry_times(0).run();
        }
        used |= ret;
        if (usage == BattleSkillUsage::Once) {
            m_status->set_number(status_key, static_cast<int64_t>(BattleSkillUsage::OnceUsed));
            m_restore_status[status_key] = static_cast<int64_t>(BattleSkillUsage::Once);
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
        image = m_ctrler->get_image();
        officially_begin_analyzer.set_image(image);
        if (officially_begin_analyzer.analyze()) {
            break;
        }
        std::this_thread::yield();
    }

    BattleImageAnalyzer oper_analyzer;
    oper_analyzer.set_target(BattleImageAnalyzer::Target::Oper);
    while (!need_exit() && !check_time()) {
        image = m_ctrler->get_image();
        oper_analyzer.set_image(image);
        if (oper_analyzer.analyze()) {
            break;
        }
        std::this_thread::yield();
    }

    std::filesystem::create_directory("map");
    for (const auto& [loc, info] : m_normal_tile_info) {
        std::string text = "( " + std::to_string(loc.x) + ", " + std::to_string(loc.y) + " )";
        cv::putText(image, text, cv::Point(info.pos.x - 30, info.pos.y), 1, 1.2, cv::Scalar(0, 0, 255), 2);
    }

    // 识别一帧总击杀数
    BattleImageAnalyzer kills_analyzer(image);
    kills_analyzer.set_target(BattleImageAnalyzer::Target::Kills);
    if (kills_analyzer.analyze()) {
        m_kills = kills_analyzer.get_kills();
        m_total_kills = kills_analyzer.get_total_kills();
    }

#ifdef WIN32
    cv::imwrite("map/" + utils::utf8_to_ansi(m_stage_name) + ".png", image);
#else
    cv::imwrite("map/" + m_stage_name + ".png", image);
#endif

    return true;
}

//asst::Rect asst::RoguelikeBattleTaskPlugin::get_placed_by_cv()
//{
//    BattlePerspectiveImageAnalyzer placed_analyzer(m_ctrler->get_image());

//    placed_analyzer.set_src_homes(m_home_cache);
//    if (!placed_analyzer.analyze()) {
//        return Rect();
//    }
//    Point nearest_point = placed_analyzer.get_nearest_point();
//    Rect placed_rect(nearest_point.x, nearest_point.y, 1, 1);
//    return placed_rect;
//}

asst::RoguelikeBattleTaskPlugin::DeployInfo asst::RoguelikeBattleTaskPlugin::calc_best_plan(const BattleRealTimeOper& oper)
{
    auto buildable_type = Loc::All;
    switch (oper.role) {
    case BattleRole::Medic:
    case BattleRole::Support:
    case BattleRole::Sniper:
    case BattleRole::Caster:
        buildable_type = Loc::Ranged;
        break;
    case BattleRole::Pioneer:
    case BattleRole::Warrior:
    case BattleRole::Tank:
        buildable_type = Loc::Melee;
        break;
    case BattleRole::Special:
    case BattleRole::Drone:
    default:
        //// 特种和无人机，有的只能放地面，有的又只能放高台，不好判断
        //// 笨办法，都试试，总有一次能成的
    //{
    //    static Loc static_loc = Loc::Melee;
    //    loc = static_loc;
    //    if (static_loc == Loc::Melee) {
    //        static_loc = Loc::Ranged;
    //    }
    //    else {
    //        static_loc = Loc::Melee;
    //    }
    //}
        // 大部分无人机都是可以摆在地上的，摆烂了
        buildable_type = Loc::Melee;
        break;
    }

    if (m_cur_home_index >= m_homes.size()) {
        m_cur_home_index = 0;
    }

    Point home(5, 5);   // 实在找不到家门了，随便取个点当家门用算了，一般是地图的中间
    if (m_cur_home_index < m_homes.size()) {
        home = m_homes.at(m_cur_home_index);
    }

    auto comp_dist = [&](const Point& lhs, const Point& rhs) -> bool {
        int lhs_y_dist = std::abs(lhs.y - home.y);
        int lhs_dist = std::abs(lhs.x - home.x) + lhs_y_dist;
        int rhs_y_dist = std::abs(rhs.y - home.y);
        int rhs_dist = std::abs(rhs.x - home.x) + rhs_y_dist;
        // 距离一样选择 x 轴上的，因为一般的地图都是横向的长方向
        return lhs_dist == rhs_dist ? lhs_y_dist < rhs_y_dist : lhs_dist < rhs_dist;
    };

    // 把所有可用的点按距离排个序
    std::vector<Point> available_locations;
    for (const auto& [loc, tile] : m_normal_tile_info) {
        if ((tile.buildable == buildable_type || tile.buildable == Loc::All)
            && !m_used_tiles.contains(loc)) {
            available_locations.emplace_back(loc);
        }
    }
    if (available_locations.empty()) {
        Log.error("No available locations");
        if (m_used_tiles.empty()) {
            Log.error("No used tiles");
            return {};
        }
        m_used_tiles.clear();
        return calc_best_plan(oper);
    }

    ranges::sort(available_locations, comp_dist);

    // 取距离最近的N个点，计算分数。然后使用得分最高的点
    constexpr int CalcPointCount = 4;
    if (available_locations.size() > CalcPointCount) {
        available_locations.erase(available_locations.begin() + CalcPointCount, available_locations.end());
    }

    Point best_location;
    Point best_direction;
    int max_score = INT_MIN;

    const auto& near_loc = available_locations.at(0);
    int min_dist = std::abs(near_loc.x - home.x) + std::abs(near_loc.y - home.y);

    for (const auto& loc : available_locations) {
        auto cur_result = calc_best_direction_and_score(loc, oper);
        // 离得远的要扣分
        constexpr int DistWeights = -1050;
        int extra_dist = std::abs(loc.x - home.x) + std::abs(loc.y - home.y) - min_dist;
        int extra_dist_score = DistWeights * extra_dist;
        if (oper.role == BattleRole::Medic) {    // 医疗干员离得远无所谓
            extra_dist_score = 0;
        }

        if (cur_result.second + extra_dist_score > max_score) {
            max_score = cur_result.second + extra_dist_score;
            best_location = loc;
            best_direction = cur_result.first;
        }
    }
    return { best_location, best_direction };
}

std::pair<asst::Point, int> asst::RoguelikeBattleTaskPlugin::calc_best_direction_and_score(Point loc, const BattleRealTimeOper& oper)
{
    LogTraceFunction;

    // 根据家门的方向计算一下大概的朝向
    if (m_cur_home_index >= m_homes.size()) {
        m_cur_home_index = 0;
    }
    Point home_loc(5, 5);
    if (m_cur_home_index < m_homes.size()) {
        home_loc = m_homes.at(m_cur_home_index);
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
    // 医疗反着算
    if (oper.role == BattleRole::Medic) {
        base_direction = -base_direction;
    }

    int64_t elite = m_status->get_number(RuntimeStatus::RoguelikeCharElitePrefix + oper.name).value_or(0);
    // 按朝右算，后面根据方向做转换
    BattleAttackRange right_attack_range = Resrc.battle_data().get_range(oper.name, elite);

    if (right_attack_range == BattleDataConfiger::EmptyRange) {
        switch (oper.role) {
        case BattleRole::Support:
            right_attack_range = {
                Point(-1, -1),Point(0, -1),Point(1, -1),Point(2, -1),
                Point(-1, 0), Point(0, 0), Point(1, 0), Point(2, 0),
                Point(-1, 1), Point(0, 1), Point(1, 1), Point(2, 1),
            };
            break;
        case BattleRole::Caster:
            right_attack_range = {
                Point(0, -1),Point(1, -1),Point(2, -1),
                Point(0, 0), Point(1, 0), Point(2, 0), Point(3, 0),
                Point(0, 1), Point(1, 1), Point(2, 1),
            };
            break;
        case BattleRole::Medic:
        case BattleRole::Sniper:
            right_attack_range = {
                Point(0, -1),Point(1, -1),Point(2, -1),Point(3, -1),
                Point(0, 0), Point(1, 0), Point(2, 0), Point(3, 0),
                Point(0, 1), Point(1, 1), Point(2, 1), Point(3, 1),
            };
            break;
        case BattleRole::Warrior:
            right_attack_range = {
                Point(0, 0), Point(1, 0), Point(2, 0)
            };
            break;
        case BattleRole::Special:
        case BattleRole::Tank:
        case BattleRole::Pioneer:
        case BattleRole::Drone:
            right_attack_range = {
                Point(0, 0), Point(1, 0)
            };
            break;
        default:
            break;
        }
    }

    int max_score = 0;
    Point opt_direction;

    for (const Point& direction : { Point::right(), Point::up(), Point::left(), Point::down() }) {
        int score = 0;
        for (const Point& relative_pos : right_attack_range) {
            Point absolute_pos = loc + relative_pos;

            using TileKey = TilePack::TileKey;
            // 战斗干员朝向的权重
            static const std::unordered_map<TileKey, int> TileKeyFightWeights = {
                    { TileKey::Invalid, 0 },
                    { TileKey::Forbidden, 0 },
                    { TileKey::Wall, 500 },
                    { TileKey::Road, 1000 },
                    { TileKey::Home, 500 },
                    { TileKey::EnemyHome, 1000 },
                    { TileKey::Floor, 1000 },
                    { TileKey::Hole, 0 },
                    { TileKey::Telin, 700 },
                    { TileKey::Telout, 800 },
                    { TileKey::Volcano, 1000 },
                    { TileKey::Healing, 1000 },
            };
            // 治疗干员朝向的权重
            static const std::unordered_map<TileKey, int> TileKeyMedicWeights = {
                    { TileKey::Invalid, 0 },
                    { TileKey::Forbidden, 0 },
                    { TileKey::Wall, 1000 },
                    { TileKey::Road, 1000 },
                    { TileKey::Home, 0 },
                    { TileKey::EnemyHome, 0 },
                    { TileKey::Floor, 0 },
                    { TileKey::Hole, 0 },
                    { TileKey::Telin, 0 },
                    { TileKey::Telout, 0 },
                    { TileKey::Volcano, 1000 },
                    { TileKey::Healing, 1000 },
            };

            switch (oper.role) {
            case BattleRole::Medic:
                if (m_used_tiles.contains(absolute_pos)) // 根据哪个方向上人多决定朝向哪
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

        if (direction == base_direction)
            score += 50;

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
