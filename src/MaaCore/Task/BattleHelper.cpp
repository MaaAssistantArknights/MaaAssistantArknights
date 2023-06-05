#include "BattleHelper.h"

#include <future>
#include <thread>

#include "Config/Miscellaneous/AvatarCacheManager.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Vision/Battle/BattlefieldClassifier.h"
#include "Vision/Battle/BattlefieldMatcher.h"
#include "Vision/BestMatcher.h"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

using namespace asst::battle;

asst::BattleHelper::BattleHelper(Assistant* inst) : m_inst_helper(inst) {}

bool asst::BattleHelper::set_stage_name(const std::string& name)
{
    LogTraceFunction;

    if (!Tile.find(name)) {
        return false;
    }
    m_stage_name = name;
    return true;
}

void asst::BattleHelper::clear()
{
    m_side_tile_info.clear();
    m_normal_tile_info.clear();
    m_skill_usage.clear();
    m_skill_error_count.clear();
    m_camera_count = 0;
    m_camera_shift = { 0., 0. };

    m_in_battle = false;
    m_kills = 0;
    m_total_kills = 0;
    m_cur_deployment_opers.clear();
    m_battlefield_opers.clear();
    m_used_tiles.clear();
}

bool asst::BattleHelper::calc_tiles_info(const std::string& stage_name, double shift_x, double shift_y)
{
    LogTraceFunction;

    if (!Tile.find(stage_name)) {
        return false;
    }

    m_normal_tile_info = Tile.calc(stage_name, false, shift_x, shift_y);
    m_side_tile_info = Tile.calc(stage_name, true, shift_x, shift_y);

    return true;
}

bool asst::BattleHelper::pause()
{
    LogTraceFunction;

    return ProcessTask(this_task(), { "BattlePause" }).run();
}

bool asst::BattleHelper::speed_up()
{
    LogTraceFunction;

    return ProcessTask(this_task(), { "BattleSpeedUp" }).run();
}

bool asst::BattleHelper::abandon()
{
    return ProcessTask(this_task(), { "RoguelikeBattleExitBegin" }).run();
}

bool asst::BattleHelper::update_deployment(bool init, const cv::Mat& reusable)
{
    LogTraceFunction;

    if (init) {
        wait_until_start(false);
    }

    cv::Mat image = init || reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;

    if (init) {
        auto draw_future = std::async(std::launch::async, [&]() { save_map(image); });
    }

    BattlefieldMatcher oper_analyzer(image);
    oper_analyzer.set_object_of_interest({ .deployment = true });
    auto oper_result_opt = oper_analyzer.analyze();
    if (!oper_result_opt) {
        check_in_battle(image);
        return false;
    }
    m_cur_deployment_opers.clear();

    auto remove_cooling_from_battlefield = [&](const DeploymentOper& oper) {
        if (!oper.cooling) {
            return;
        }
        auto iter = m_battlefield_opers.find(oper.name);
        if (iter == m_battlefield_opers.end()) {
            return;
        }
        auto loc = iter->second;
        m_used_tiles.erase(loc);
        m_battlefield_opers.erase(iter);
    };

    auto set_oper_name = [&](DeploymentOper& oper, const std::string& name) {
        oper.name = name;
        oper.location_type = BattleData.get_location_type(name);
        oper.is_unusual_location = battle::get_role_usual_location(oper.role) == oper.location_type;
    };

    auto& cur_opers = oper_result_opt->deployment;
    std::vector<DeploymentOper> unknown_opers;

    for (auto& oper : cur_opers) {
        BestMatcher avatar_analyzer(oper.avatar);
        if (oper.cooling) {
            Log.trace("start matching cooling", oper.index);
            static const double cooling_threshold = Task.get<MatchTaskInfo>("BattleAvatarCoolingData")->templ_threshold;
            static const auto cooling_mask_range = Task.get<MatchTaskInfo>("BattleAvatarCoolingData")->mask_range;
            avatar_analyzer.set_threshold(cooling_threshold);
            avatar_analyzer.set_mask_range(cooling_mask_range.first, cooling_mask_range.second, true, true);
        }
        else {
            static const double threshold = Task.get<MatchTaskInfo>("BattleAvatarData")->templ_threshold;
            static const double drone_threshold = Task.get<MatchTaskInfo>("BattleDroneAvatarData")->templ_threshold;
            avatar_analyzer.set_threshold(oper.role == Role::Drone ? drone_threshold : threshold);
        }

        auto& avatar_cache = AvatarCache.get_avatars(oper.role);
        for (const auto& [name, avatar] : avatar_cache) {
            avatar_analyzer.append_templ(name, avatar);
        }
        if (avatar_analyzer.analyze()) {
            set_oper_name(oper, avatar_analyzer.get_result().templ_info.name);
            m_cur_deployment_opers.insert_or_assign(oper.name, oper);
            remove_cooling_from_battlefield(oper);
        }
        else {
            Log.info("unknown oper", oper.index);
            unknown_opers.emplace_back(oper);
        }

        if (oper.cooling) {
            Log.trace("stop matching cooling", oper.index);
        }
    }

    if (!unknown_opers.empty() || init) {
        // 一个都没匹配上的，挨个点开来看一下
        LogTraceScope("rec unknown opers");

        // 暂停游戏准备识别干员
        do {
            pause();
            // 在刚进入游戏的时候（画面刚刚完全亮起来的时候），点暂停是没反应的
            // 所以这里一直点，直到真的点上了为止
            if (!init || !check_pause_button()) {
                break;
            }
            std::this_thread::yield();
        } while (!m_inst_helper.need_exit());

        if (!check_in_battle(image)) {
            return false;
        }

        for (auto& oper : unknown_opers) {
            LogTraceScope("rec unknown oper: " + std::to_string(oper.index));
            if (oper.cooling) {
                Log.info("cooling oper, skip");
                oper.name = "UnknownCooling_" + std::to_string(oper.index);
                continue;
            }

            click_oper_on_deployment(oper.rect);

            cv::Mat name_image = m_inst_helper.ctrler()->get_image();
            if (!check_in_battle(name_image)) {
                return false;
            }

            std::string name = analyze_detail_page_oper_name(name_image);
            // 这时候即使名字不合法也只能凑合用了，但是为空还是不行的
            if (name.empty()) {
                Log.error("name is empty");
                continue;
            }
            set_oper_name(oper, name);
            remove_cooling_from_battlefield(oper);

            m_cur_deployment_opers.insert_or_assign(name, oper);
            AvatarCache.set_avatar(name, oper.role, oper.avatar);
        }
        pause();
        if (!unknown_opers.empty()) {
            cancel_oper_selection();
        }

        image = m_inst_helper.ctrler()->get_image();
    }

    if (init) {
        update_kills(image);
    }

    return check_in_battle(image);
}

bool asst::BattleHelper::update_kills(const cv::Mat& reusable)
{
    cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    BattlefieldMatcher analyzer(image);
    analyzer.set_object_of_interest({ .kills = true });
    if (m_total_kills) {
        analyzer.set_total_kills_prompt(m_total_kills);
    }
    auto result_opt = analyzer.analyze();
    if (!result_opt || !result_opt->kills) {
        return false;
    }
    std::tie(m_kills, m_total_kills) = result_opt->kills.value();
    return true;
}

bool asst::BattleHelper::update_cost(const cv::Mat& reusable)
{
    cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    BattlefieldMatcher analyzer(image);
    analyzer.set_object_of_interest({ .costs = true });
    auto result_opt = analyzer.analyze();
    if (!result_opt || !result_opt->costs) {
        return false;
    }
    m_cost = result_opt->costs.value();
    return true;
}

bool asst::BattleHelper::deploy_oper(const std::string& name, const Point& loc, DeployDirection direction)
{
    LogTraceFunction;

    const auto swipe_oper_task_ptr = Task.get("BattleSwipeOper");
    const auto use_oper_task_ptr = Task.get("BattleUseOper");

    auto rect_opt = get_oper_rect_on_deployment(name);
    if (!rect_opt) {
        return false;
    }
    const Rect& oper_rect = *rect_opt;

    auto target_iter = m_side_tile_info.find(loc);
    if (target_iter == m_side_tile_info.cend()) {
        Log.error("No loc", loc);
        return false;
    }
    const Point& target_point = target_iter->second.pos;

    int dist = static_cast<int>(
        Point::distance(target_point, { oper_rect.x + oper_rect.width / 2, oper_rect.y + oper_rect.height / 2 }));

    // 1000 是随便取的一个系数，把整数的 pre_delay 转成小数用的
    int duration = static_cast<int>(dist / 1000.0 * swipe_oper_task_ptr->pre_delay);
    // 时间太短了的压根放不上去，故意加长一点
    if (int min_duration = swipe_oper_task_ptr->special_params.at(3); duration < min_duration) {
        duration = min_duration;
    }
    bool deploy_with_pause =
        ControlFeat::support(m_inst_helper.ctrler()->support_features(), ControlFeat::SWIPE_WITH_PAUSE);
    Point oper_point(oper_rect.x + oper_rect.width / 2, oper_rect.y + oper_rect.height / 2);
    m_inst_helper.ctrler()->swipe(oper_point, target_point, duration, false, swipe_oper_task_ptr->special_params.at(1),
                                  swipe_oper_task_ptr->special_params.at(2), deploy_with_pause);

    // 拖动干员朝向
    if (direction != DeployDirection::None) {
        static const std::unordered_map<DeployDirection, Point> DirectionMap = {
            { DeployDirection::Right, Point(1, 0) }, { DeployDirection::Down, Point(0, 1) },
            { DeployDirection::Left, Point(-1, 0) }, { DeployDirection::Up, Point(0, -1) },
            { DeployDirection::None, Point(0, 0) },
        };

        // 计算往哪边拖动
        const Point& direction_target = DirectionMap.at(direction);

        // 将方向转换为实际的 swipe end 坐标点
        static const int coeff = swipe_oper_task_ptr->special_params.at(0);
        Point end_point = target_point + (direction_target * coeff);

        m_inst_helper.sleep(use_oper_task_ptr->post_delay);
        m_inst_helper.ctrler()->swipe(target_point, end_point, swipe_oper_task_ptr->post_delay);
        m_inst_helper.sleep(use_oper_task_ptr->pre_delay);
    }

    if (deploy_with_pause) {
        m_inst_helper.ctrler()->press_esc();
    }

    m_battlefield_opers.emplace(name, loc);
    m_used_tiles.emplace(loc, name);

    return true;
}

bool asst::BattleHelper::retreat_oper(const std::string& name)
{
    LogTraceFunction;

    auto oper_iter = m_battlefield_opers.find(name);
    if (oper_iter == m_battlefield_opers.cend()) {
        Log.error("No oper", name);
        return false;
    }

    if (!retreat_oper(oper_iter->second, false)) {
        return false;
    }

    m_battlefield_opers.erase(name);
    return true;
}

bool asst::BattleHelper::retreat_oper(const Point& loc, bool manually)
{
    LogTraceFunction;

    if (!click_oper_on_battlefield(loc) || !click_retreat()) {
        return false;
    }

    m_used_tiles.erase(loc);
    if (manually) {
        std::erase_if(m_battlefield_opers, [&loc](const auto& pair) -> bool { return pair.second == loc; });
    }
    return true;
}

bool asst::BattleHelper::use_skill(const std::string& name, bool keep_waiting)
{
    LogTraceFunction;

    auto oper_iter = m_battlefield_opers.find(name);
    if (oper_iter == m_battlefield_opers.cend()) {
        Log.error("No oper", name);
        return false;
    }

    return use_skill(oper_iter->second, keep_waiting);
}

bool asst::BattleHelper::use_skill(const Point& loc, bool keep_waiting)
{
    LogTraceFunction;

    return click_oper_on_battlefield(loc) && click_skill(keep_waiting);
}

bool asst::BattleHelper::check_pause_button(const cv::Mat& reusable)
{
    cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    Matcher battle_flag_analyzer(image);
    battle_flag_analyzer.set_task_info("BattleOfficiallyBegin");
    bool ret = battle_flag_analyzer.analyze().has_value();

    BattlefieldMatcher battle_flag_analyzer_2(image);
    auto battle_result_opt = battle_flag_analyzer_2.analyze();
    ret &= battle_result_opt && battle_result_opt->pause_button;
    return ret;
}

bool asst::BattleHelper::check_in_battle(const cv::Mat& reusable, bool weak)
{
    cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    if (weak) {
        BattlefieldMatcher analyzer(image);
        m_in_battle = analyzer.analyze().has_value();
    }
    else {
        m_in_battle = check_pause_button(image);
    }
    return m_in_battle;
}

bool asst::BattleHelper::wait_until_start(bool weak)
{
    LogTraceFunction;

    cv::Mat image = m_inst_helper.ctrler()->get_image();
    while (!m_inst_helper.need_exit() && !check_in_battle(image, weak)) {
        do_strategic_action(image);
        std::this_thread::yield();

        image = m_inst_helper.ctrler()->get_image();
    }
    return true;
}

bool asst::BattleHelper::wait_until_end(bool weak)
{
    LogTraceFunction;

    cv::Mat image = m_inst_helper.ctrler()->get_image();
    while (!m_inst_helper.need_exit() && check_in_battle(image, weak)) {
        do_strategic_action(image);
        std::this_thread::yield();

        image = m_inst_helper.ctrler()->get_image();
    }
    return true;
}

bool asst::BattleHelper::do_strategic_action(const cv::Mat& reusable)
{
    cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    return use_all_ready_skill(image);
}

bool asst::BattleHelper::use_all_ready_skill(const cv::Mat& reusable)
{
    bool used = false;
    cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    for (const auto& [name, loc] : m_battlefield_opers) {
        auto& usage = m_skill_usage[name];
        if (usage != SkillUsage::Possibly && usage != SkillUsage::Once) {
            continue;
        }
        bool has_error = false;
        if (!check_and_use_skill(loc, has_error, image)) {
            continue;
        }
        // 识别到了，但点进去发现没有。一般来说是识别错了
        if (has_error) {
            Log.warn("Skill", name, "is not ready");
            constexpr int MaxRetry = 3;
            if (++m_skill_error_count[name] >= MaxRetry) {
                Log.warn("Do not use skill anymore", name);
                usage = SkillUsage::NotUse;
            }
            continue;
        }
        used = true;
        m_skill_error_count[name] = 0;
        if (usage == SkillUsage::Once) {
            usage = SkillUsage::OnceUsed;
        }
        image = m_inst_helper.ctrler()->get_image();
    }

    return used;
}

bool asst::BattleHelper::check_and_use_skill(const std::string& name, bool& has_error, const cv::Mat& reusable)
{
    auto oper_iter = m_battlefield_opers.find(name);
    if (oper_iter == m_battlefield_opers.cend()) {
        Log.error("No oper", name);
        return false;
    }
    return check_and_use_skill(oper_iter->second, has_error, reusable);
}

bool asst::BattleHelper::check_and_use_skill(const Point& loc, bool& has_error, const cv::Mat& reusable)
{
    cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    BattlefieldClassifier skill_analyzer(image);
    skill_analyzer.set_object_of_interest({ .skill_ready = true });

    auto target_iter = m_normal_tile_info.find(loc);
    if (target_iter == m_normal_tile_info.end()) {
        Log.error("No loc", loc);
        return false;
    }
    const Point& battlefield_point = target_iter->second.pos;
    skill_analyzer.set_base_point(battlefield_point);
    if (!skill_analyzer.analyze()->skill_ready.ready) {
        return false;
    }

    has_error = !use_skill(loc, false);
    return true;
}

void asst::BattleHelper::save_map(const cv::Mat& image)
{
    LogTraceFunction;

    using namespace asst::utils::path_literals;
    const auto& MapRelativeDir = "debug"_p / "map"_p;

    auto draw = image.clone();
    for (const auto& [loc, info] : m_normal_tile_info) {
        cv::circle(draw, cv::Point(info.pos.x, info.pos.y), 5, cv::Scalar(0, 255, 0), -1);
        cv::putText(draw, loc.to_string(), cv::Point(info.pos.x - 30, info.pos.y), 1, 1.2, cv::Scalar(0, 0, 255), 2);
    }

    std::string suffix;
    if (++m_camera_count > 1) {
        suffix = "-" + std::to_string(m_camera_count);
    }
    asst::imwrite(MapRelativeDir / asst::utils::path(m_stage_name + suffix + ".png"), draw);
}

bool asst::BattleHelper::click_oper_on_deployment(const std::string& name)
{
    LogTraceFunction;

    auto rect_opt = get_oper_rect_on_deployment(name);
    if (!rect_opt) {
        return false;
    }

    return click_oper_on_deployment(*rect_opt);
}

bool asst::BattleHelper::click_oper_on_deployment(const Rect& rect)
{
    LogTraceFunction;

    const auto use_oper_task_ptr = Task.get("BattleUseOper");
    m_inst_helper.ctrler()->click(rect);
    m_inst_helper.sleep(use_oper_task_ptr->pre_delay);

    return true;
}

bool asst::BattleHelper::click_oper_on_battlefield(const std::string& name)
{
    LogTraceFunction;

    auto oper_iter = m_battlefield_opers.find(name);
    if (oper_iter == m_battlefield_opers.cend()) {
        Log.error("No oper", name);
        return false;
    }
    return click_oper_on_battlefield(oper_iter->second);
}

bool asst::BattleHelper::click_oper_on_battlefield(const Point& loc)
{
    LogTraceFunction;

    const auto use_oper_task_ptr = Task.get("BattleUseOper");

    auto target_iter = m_normal_tile_info.find(loc);
    if (target_iter == m_normal_tile_info.end()) {
        Log.error("No loc", loc);
        return false;
    }
    const Point& target_point = target_iter->second.pos;

    m_inst_helper.ctrler()->click(target_point);
    m_inst_helper.sleep(use_oper_task_ptr->pre_delay);

    return true;
}

bool asst::BattleHelper::click_retreat()
{
    LogTraceFunction;

    return ProcessTask(this_task(), { "BattleOperRetreatJustClick" }).run();
}

bool asst::BattleHelper::click_skill(bool keep_waiting)
{
    LogTraceFunction;

    ProcessTask skill_task(this_task(), { "BattleSkillReadyOnClick", "BattleSkillReadyOnClick-SquareMap",
                                          "BattleSkillStopOnClick", "BattleSkillStopOnClick-SquareMap" });
    skill_task.set_task_delay(0);

    if (keep_waiting) {
        return skill_task.set_retry_times(1000).run();
    }
    else {
        bool ret = skill_task.set_retry_times(5).run();
        if (!ret) {
            cancel_oper_selection();
        }
        return ret;
    }
}

bool asst::BattleHelper::cancel_oper_selection()
{
    return ProcessTask(this_task(), { "BattleCancelSelection" }).run();
}

bool asst::BattleHelper::move_camera(const std::pair<double, double>& delta)
{
    LogTraceFunction;
    Log.info("move", delta.first, delta.second);

    update_kills();

    // 还没转场的时候
    if (m_kills != 0) {
        wait_until_end(false);
    }

    m_kills = 0;
    m_total_kills = 0;

    m_camera_shift.first += delta.first;
    m_camera_shift.second += delta.second;

    calc_tiles_info(m_stage_name, -m_camera_shift.first, m_camera_shift.second);
    return update_deployment(true);
}

std::string asst::BattleHelper::analyze_detail_page_oper_name(const cv::Mat& image)
{
    const auto& replace_task = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    const auto& task = Task.get<OcrTaskInfo>(oper_name_ocr_task_name());

    RegionOCRer preproc_analyzer(image);
    preproc_analyzer.set_task_info(task);
    preproc_analyzer.set_replace(replace_task->replace_map, replace_task->replace_full);
    auto preproc_result_opt = preproc_analyzer.analyze();

    if (preproc_result_opt && !BattleData.is_name_invalid(preproc_result_opt->text)) {
        return preproc_result_opt->text;
    }

    Log.warn("ocr with preprocess got a invalid name, try to use detect model");
    OCRer det_analyzer(image);
    det_analyzer.set_task_info(task);
    det_analyzer.set_replace(replace_task->replace_map, replace_task->replace_full);
    auto det_result_opt = det_analyzer.analyze();
    if (!det_result_opt) {
        return {};
    }
    sort_by_score_(*det_result_opt);
    const auto& det_name = det_result_opt->front().text;

    return BattleData.is_name_invalid(det_name) ? std::string() : det_name;
}

std::optional<asst::Rect> asst::BattleHelper::get_oper_rect_on_deployment(const std::string& name) const
{
    LogTraceFunction;

    auto oper_iter = m_cur_deployment_opers.find(name);
    if (oper_iter == m_cur_deployment_opers.cend()) {
        Log.error("No oper", name);
        return std::nullopt;
    }

    return oper_iter->second.rect;
}
