#include "BattleHelper.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <future>
#include <limits>
#include <string_view>
#include <thread>
#include <utility>

#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/AvatarCacheManager.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Task/ProcessTask.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Battle/BattlefieldClassifier.h"
#include "Vision/Battle/BattlefieldDetector.h"
#include "Vision/Battle/BattlefieldMatcher.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/Oper/OperNameAnalyzer.h"
#include "Vision/RegionOCRer.h"
#include <ranges>

#include "Arknights-Tile-Pos/TileCalc2.hpp"

using namespace asst::battle;

asst::BattleHelper::BattleHelper(Assistant* inst) :
    m_inst_helper(inst)
{
}

bool asst::BattleHelper::set_stage_name(const std::string& name)
{
    LogTraceFunction;

    if (auto result = Tile.find(name); !result || !json::open(result->second)) {
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
    m_skill_times.clear();
    m_skill_error_count.clear();
    m_last_use_skill_time.clear();
    m_last_auto_skill_use.clear();
    m_battle_assist_ready_scale.reset();
    m_battle_assist_ready_rect.reset();
    m_battle_assist_present_markers.clear();
    m_camera_count = 0;
    m_camera_shift = { 0., 0. };

    m_in_battle = false;
    m_kills = 0;
    m_total_kills = 0;
    m_stopwatch_enabled = false;
    m_cur_deployment_opers.clear();
    m_battlefield_opers.clear();
    m_used_tiles.clear();
}

bool asst::BattleHelper::calc_tiles_info(const std::string& stage_name, double shift_x, double shift_y)
{
    LogTraceFunction;

    if (auto result = Tile.find(stage_name); !result || !json::open(result->second)) {
        return false;
    }

    m_map_data = TilePack::find_level(stage_name).value_or(Map::Level {});
    auto calc_result = TilePack::calc(stage_name, shift_x, shift_y);
    m_normal_tile_info = std::move(calc_result.normal_tile_info);
    m_side_tile_info = std::move(calc_result.side_tile_info);
    m_retreat_button_pos = calc_result.retreat_button;
    m_skill_button_pos = calc_result.skill_button;
    m_has_multi_stages = calc_result.has_multi_stages;

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

template <typename T>
requires std::ranges::range<T> && asst::OperAvatarPair<std::ranges::range_value_t<T>>
std::optional<asst::BestMatcher::Result>
    analyze_oper_with_cache(const asst::battle::DeploymentOper& oper, T&& avatar_cache)
{
    using namespace asst;

    BestMatcher avatar_analyzer(oper.avatar);
    avatar_analyzer.set_method(MatchMethod::Ccoeff);
    if (oper.cooling) {
        Log.trace("start matching cooling", oper.index);
        static const auto cooling_threshold =
            Task.get<MatchTaskInfo>("BattleAvatarCoolingData")->templ_thresholds.front();
        static const auto cooling_mask_range = Task.get<MatchTaskInfo>("BattleAvatarCoolingData")->mask_ranges;
        avatar_analyzer.set_threshold(cooling_threshold);
        avatar_analyzer.set_mask_ranges(cooling_mask_range, true, true);
    }
    else {
        static const auto threshold = Task.get<MatchTaskInfo>("BattleAvatarData")->templ_thresholds.front();
        static const auto drone_threshold = Task.get<MatchTaskInfo>("BattleDroneAvatarData")->templ_thresholds.front();
        avatar_analyzer.set_threshold(oper.role == Role::Drone ? drone_threshold : threshold);
    }

    for (const auto& [name, avatar] : avatar_cache) {
        avatar_analyzer.append_templ(name, avatar);
    }

    if (avatar_analyzer.analyze()) {
        return avatar_analyzer.get_result();
    }

    return std::nullopt;
}

bool asst::BattleHelper::update_deployment_(
    std::vector<battle::DeploymentOper>& cur_opers,
    const std::vector<battle::DeploymentOper>& old_deployment_opers,
    const bool analyze_unknown)
{
    LogTraceFunction;

    // ————————————————————————————————————————
    // 准备 lambda 函数
    // ————————————————————————————————————————
    // 设置干员名与部署位类型（近战/远程）
    auto set_oper_name = [&](DeploymentOper& oper, const std::string& name) {
        oper.name = name;
        oper.location_type = BattleData.get_location_type(oper.role, name);
        oper.is_usual_location = battle::get_role_usual_location(oper.role) == oper.location_type;
    };

    // ————————————————————————————————————————
    // 初始化变量
    // ————————————————————————————————————————
    m_cur_deployment_opers = std::vector<battle::DeploymentOper>();
    m_cur_deployment_opers.reserve(cur_opers.size());
    std::vector<DeploymentOper> unknown_opers;

    // ————————————————————————————————————————
    // 匹配已知干员
    // ————————————————————————————————————————
    for (auto& oper : cur_opers) {
        bool is_analyzed = false;
        if (!old_deployment_opers.empty()) {
            auto avatar = old_deployment_opers | std::views::filter([&](const battle::DeploymentOper& temp_oper) {
                              return temp_oper.role == oper.role;
                          }) |
                          std::views::transform([&](const battle::DeploymentOper& temp_oper) {
                              return std::make_pair(temp_oper.name, temp_oper.avatar);
                          });
            const auto& result = analyze_oper_with_cache(oper, avatar);
            if (result) {
                set_oper_name(oper, result->templ_info.name);
                remove_cooling_from_battlefield(oper);
                is_analyzed = true;
            }
        }
        if (!is_analyzed) {
            // 之前的干员都没匹配上，那就把所有的干员都加进去
            const auto& result2 = analyze_oper_with_cache(oper, AvatarCache.get_avatars(oper.role));
            if (result2) {
                set_oper_name(oper, result2->templ_info.name);
                remove_cooling_from_battlefield(oper);
            }
            else {
                Log.info("unknown oper", oper.index);
                // 冷却中的未知干员不进行匹配
                if (!analyze_unknown && !oper.cooling) {
                    return false;
                }
                unknown_opers.emplace_back(oper);
            }
        }

        m_cur_deployment_opers.emplace_back(oper);

        if (oper.cooling) {
            Log.trace("stop matching cooling", oper.index);
        }
    }

    // ————————————————————————————————————————
    // 匹配未知非冷却干员
    // ————————————————————————————————————————
    if (std::ranges::count_if(unknown_opers, [](const DeploymentOper& it) { return !it.cooling; }) > 0) {
        // 一个都没匹配上的，挨个点开来看一下
        LogTraceScope("rec unknown opers");

        cv::Mat name_image;
        for (auto& oper : unknown_opers) {
            LogTraceScope("rec unknown oper: " + std::to_string(oper.index));
            if (oper.cooling) {
                Log.info("cooling oper, skip");
                // oper.name = "UnknownCooling_" + std::to_string(oper.index); // 为什么还要取名字啊喵
                continue;
            }

            Rect oper_rect = oper.rect;

            click_oper_on_deployment(oper_rect);

            name_image = m_inst_helper.ctrler()->get_image();

            std::string name = analyze_detail_page_oper_name(name_image, oper.role);
            // 这时候即使名字不合法也只能凑合用了，但是为空还是不行的
            if (name.empty()) {
                Log.error("name is empty");
            }
            else {
                set_oper_name(oper, name);
                remove_cooling_from_battlefield(oper);

                m_cur_deployment_opers[oper.index] = oper;
                AvatarCache.set_avatar(name, oper.role, oper.avatar);
            }

            // 再点一下部署栏头像取消选择
            // 注意，点完部署区的干员之后，他的头像会放大；干员的位置都被挤开了，不在原来的位置了
            // 所以要重新识别一下位置
            MultiMatcher re_matcher(name_image);
            re_matcher.set_task_info("BattleAvatarReMatch");
            re_matcher.set_templ(oper.avatar);
            if (re_matcher.analyze()) {
                if (const auto& results = re_matcher.get_result(); !results.empty()) {
                    // 遍历结果，找到 y 最小的（之前选中的） rect
                    auto min_rect_iter = std::ranges::min_element(results, [](const auto& a, const auto& b) {
                        return a.rect.y < b.rect.y;
                    });

                    oper_rect = min_rect_iter->rect;
                }
            }
            click_oper_on_deployment(oper_rect);
        }
    }

    return true;
}

bool asst::BattleHelper::update_deployment(bool init, const cv::Mat& reusable, bool need_oper_cost)
{
    LogTraceFunction;

    if (init) {
        AvatarCache.remove_confusing_avatars(); // 移除小龙等不同技能很像的召唤物，防止错误识别
        if (!wait_until_start(false)) {
            return false;
        }
    }

    cv::Mat image = init || reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    if (init) {
        auto draw_future = std::async(std::launch::async, [&]() { save_map(image); });
    }

    BattlefieldMatcher oper_analyzer(image);

    // 保全要识别开局费用，先用init判断了，之后别的地方要用的话再做cache
    if (init || need_oper_cost) {
        oper_analyzer.set_object_of_interest({ .deployment = true, .oper_cost = true });
    }
    else {
        oper_analyzer.set_object_of_interest({ .deployment = true });
    }
    auto oper_result_opt = oper_analyzer.analyze();
    if (!oper_result_opt) {
        check_in_battle(image);
        return false;
    }
    const auto old_deployment_opers = std::move(m_cur_deployment_opers);

    if (!update_deployment_(oper_result_opt->deployment, old_deployment_opers, false)) {
        // 发现未知干员，暂停游戏后再重新识别干员
        do {
            pause();
            // 在刚进入游戏的时候（画面刚刚完全亮起来的时候），点暂停是没反应的
            // 所以这里一直点，直到真的点上了为止
            if (!init || !check_pause_button()) {
                break;
            }
            std::this_thread::yield();
        } while (!m_inst_helper.need_exit());

        // 重新截图
        image = m_inst_helper.ctrler()->get_image();

        // 如果需要停止任务或者已经不在战斗中，则退出；否则默认之后的操作一直都在战斗中
        if (m_inst_helper.need_exit() || !check_in_battle(image)) {
            return false;
        }

        // 重新识别
        BattlefieldMatcher oper_analyzer2(image);
        if (init || need_oper_cost) {
            oper_analyzer2.set_object_of_interest({ .deployment = true, .oper_cost = true });
        }
        else {
            oper_analyzer2.set_object_of_interest({ .deployment = true });
        }
        oper_result_opt = oper_analyzer2.analyze();
        if (!oper_result_opt) {
            check_in_battle(image);
            return false;
        }
        update_deployment_(oper_result_opt->deployment, old_deployment_opers, true);
        pause();
        cancel_oper_selection();
        image = m_inst_helper.ctrler()->get_image();
    }

    if (init) {
        update_kills(image);
    }

    return check_in_battle(image);
}

// if side = true, get top view of the selected operator, tile size is 5x5
// has_multi_stages: does map have multi stages, e.g. TN-1 ~ TN-4
cv::Mat asst::BattleHelper::get_top_view(const cv::Mat& cam_img, bool side, bool has_multi_stages)
{
    if (!side) {
        return cv::Mat {}; // TODO
    }

    const std::vector<cv::Point2f> world_points {
        { -2.5, -2.5 },
        { 2.5, -2.5 },
        { -2.5, 2.5 },
        { 2.5, 2.5 },
    };
    const std::vector<cv::Point2f> dst_points {
        { 0, 0 },
        { 500, 0 },
        { 0, 500 },
        { 500, 500 },
    };
    std::vector<cv::Point2f> screen_points;
    for (const auto& point : world_points) {
        cv::Vec3d temp { point.x + (has_multi_stages ? m_map_data.view[0].x : 0), -point.y, Map::TileCalc2::rel_pos_z };
        auto screen_pt = Map::TileCalc2::world_to_screen(m_map_data, temp, true);
        screen_points.push_back(screen_pt);
    }

    const auto xform = cv::getPerspectiveTransform(screen_points, dst_points);
    cv::Mat result;
    cv::warpPerspective(cam_img, result, xform, cv::Size(500, 500));
    return result;
}

bool asst::BattleHelper::update_kills(const cv::Mat& image, const cv::Mat& image_prev)
{
    BattlefieldMatcher analyzer(image);
    analyzer.set_object_of_interest({ .kills = true });
    analyzer.set_image_prev(image_prev);
    if (m_total_kills) {
        analyzer.set_total_kills_prompt(m_total_kills);
    }
    auto result_opt = analyzer.analyze();
    if (!result_opt || result_opt->kills.status == BattlefieldMatcher::MatchStatus::Invalid) {
        return false;
    }
    if (result_opt->kills.status == BattlefieldMatcher::MatchStatus::Success) {
        std::tie(m_kills, m_total_kills) = result_opt->kills.value;
    }
    return true;
}

bool asst::BattleHelper::update_cost(const cv::Mat& image, const cv::Mat& image_prev)
{
    BattlefieldMatcher analyzer(image);
    analyzer.set_object_of_interest({ .costs = true });
    analyzer.set_image_prev(image_prev);
    auto result_opt = analyzer.analyze();
    if (!result_opt || result_opt->costs.status == BattlefieldMatcher::MatchStatus::Invalid) {
        return false;
    }
    else if (result_opt->costs.status == BattlefieldMatcher::MatchStatus::Success) {
        m_cost = result_opt->costs.value;
    }
    return true;
}

bool asst::BattleHelper::deploy_oper(const std::string& name, const Point& loc, DeployDirection direction)
{
    return deploy_oper(battle::Role::Unknown, name, loc, direction);
}

bool asst::BattleHelper::deploy_oper(
    battle::Role role,
    const std::string& name,
    const Point& loc,
    battle::DeployDirection direction)
{
    LogTraceFunction;

    const auto swipe_oper_task_ptr = Task.get("BattleSwipeOper");
    const auto use_oper_task_ptr = Task.get("BattleUseOper");

    auto it = std::ranges::find_if(m_cur_deployment_opers, [&](const auto& oper) {
        return (role == battle::Role::Unknown || oper.role == role) && oper.name == name;
    });

    if (it == m_cur_deployment_opers.cend()) {
        LogError << "No oper" << enum_to_string(role) << name;
        return false;
    }

    const auto& oper_role = it->role;
    const auto& oper_name = it->name;
    const Rect& oper_rect = it->rect;

    auto target_iter = m_side_tile_info.find(loc);
    if (target_iter == m_side_tile_info.cend()) {
        LogError << "No loc" << loc;
        return false;
    }
    Point target_point = target_iter->second.pos;

    int dist = static_cast<int>(
        Point::distance(target_point, { oper_rect.x + oper_rect.width / 2, oper_rect.y + oper_rect.height / 2 }));

    // 1000 是随便取的一个系数，把整数的 pre_delay 转成小数用的
    int duration = static_cast<int>(dist / 1000.0 * swipe_oper_task_ptr->pre_delay);
    // 时间太短了的压根放不上去，故意加长一点
    if (int min_duration = swipe_oper_task_ptr->special_params.at(4); duration < min_duration) {
        duration = min_duration;
    }
    bool deploy_with_pause =
        ControlFeat::support(m_inst_helper.ctrler()->support_features(), ControlFeat::SWIPE_WITH_PAUSE);
    bool depoly_when_pause_not_support =
        Task.get("BattleDeployWhenPause")->special_params[0] ==
        0; // Oversea client support, remove when all clients support this feature !!! by status102
    if (deploy_with_pause && !depoly_when_pause_not_support) {
        pause();
    }
    Point oper_point(oper_rect.x + oper_rect.width / 2, oper_rect.y + oper_rect.height / 2);
    m_inst_helper.ctrler()->swipe(
        oper_point,
        target_point,
        duration,
        SwipeExtraDirection::None,
        swipe_oper_task_ptr->special_params.at(2) / 10.0,
        swipe_oper_task_ptr->special_params.at(3) / 10.0,
        deploy_with_pause && depoly_when_pause_not_support);

    // 拖动干员朝向
    if (direction != DeployDirection::None) {
        static const std::unordered_map<DeployDirection, Point> DirectionMap = {
            { DeployDirection::Right, Point::right() }, { DeployDirection::Down, Point::down() },
            { DeployDirection::Left, Point::left() },   { DeployDirection::Up, Point::up() },
            { DeployDirection::None, Point::zero() },
        };

        // 计算往哪边拖动
        const Point& direction_target = DirectionMap.at(direction);

        // 将方向转换为实际的 swipe end 坐标点，并对滑动距离进行缩放
        const auto scale_size = m_inst_helper.ctrler()->get_scale_size();
        static const int coeff =
            static_cast<int>(swipe_oper_task_ptr->special_params.at(0) * scale_size.second / 720.0);
        Point end_point = target_point + (direction_target * coeff);

        fix_swipe_out_of_limit(
            target_point,
            end_point,
            scale_size.first,
            scale_size.second,
            swipe_oper_task_ptr->special_params.at(1));

        m_inst_helper.sleep(use_oper_task_ptr->post_delay);
        m_inst_helper.ctrler()->swipe(target_point, end_point, swipe_oper_task_ptr->post_delay);
        // 仅简单复用，该延迟含义与此处逻辑无关 by MistEO
        m_inst_helper.sleep(use_oper_task_ptr->pre_delay);
    }

    if (deploy_with_pause) {
        // m_inst_helper.ctrler()->press_esc();
        ProcessTask(this_task(), { "BattlePauseCancel" }).run();
    }

    // for SSS, multiple operator may be deployed at the same location.
    if (m_used_tiles.contains(loc)) {
        const auto& pre_oper = m_used_tiles.at(loc);
        LogInfo << "remove previous oper" << pre_oper << loc;
        m_battlefield_opers.erase(pre_oper);
        m_used_tiles.erase(loc);
    }

    // 肉鸽中，一个干员可能被部署多次
    battle::OperNameTag oper_tag { oper_role, oper_name };
    if (m_battlefield_opers.contains(oper_tag) && oper_role != battle::Role::Drone) {
        Point pre_loc = m_battlefield_opers.at(oper_tag);
        LogInfo << "remove deploy failed oper" << oper_tag << pre_loc;
        m_battlefield_opers.erase(oper_tag);
        // 擦除已使用干员,但是不擦除已使用格子
        // m_used_tiles.erase(pre_loc);
    }

    register_deployed_oper(oper_role, oper_name, loc);
    m_inst_helper.sleep(200); // 部署完会有 166 ms 的动画

    return true;
}

bool asst::BattleHelper::retreat_oper(const std::string& name)
{
    return retreat_oper(battle::Role::Unknown, name);
}

bool asst::BattleHelper::retreat_oper(battle::Role role, const std::string& name)
{
    LogTraceFunction;

    auto oper_iter = std::ranges::find_if(m_battlefield_opers, [&](const auto& pair) {
        return (role == battle::Role::Unknown || pair.first.role == role) && pair.first.name == name;
    });
    if (oper_iter == m_battlefield_opers.cend()) {
        LogError << "No oper" << name << "role:" << enum_to_string(role);
        return false;
    }

    if (!retreat_oper(oper_iter->second, false)) {
        return false;
    }

    m_battlefield_opers.erase(oper_iter);
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
    cancel_oper_selection(); // 兜底一下, 防止格子上面并没有干员, 导致点到隔壁格子
    return true;
}

bool asst::BattleHelper::set_unit_location(battle::Role role, const std::string& name, const Point& loc)
{
    LogTraceFunction;

    if (!m_normal_tile_info.contains(loc)) {
        LogError << __FUNCTION__ << "| No tile found at" << loc << "for unit" << name;
        return false;
    }

    auto oper_iter = std::ranges::find_if(m_battlefield_opers, [&](const auto& pair) {
        return (role == battle::Role::Unknown || pair.first.role == role) && pair.first.name == name;
    });

    battle::OperNameTag tag;
    if (oper_iter != m_battlefield_opers.cend()) {
        tag = oper_iter->first;
        if (oper_iter->second == loc) {
            LogInfo << __FUNCTION__ << "| Location of unit" << tag.name << "is already" << loc;
            return true;
        }
        // 单位已不在旧格，迁移时同步释放旧格占用
        m_used_tiles.erase(oper_iter->second);
        m_battlefield_opers.erase(oper_iter);
    }
    else {
        // 不在场的单位（装置、设施等）直接登记，使其参与技能用法与自动开技能流程
        tag = { role, name };
        LogInfo << __FUNCTION__ << "| Unit" << name << "not on battlefield, register it at" << loc;
    }

    // 一格只保留一条单位记录：目标格已有占用者时，旧单位的记录整条移除（其名字不再可按名引用）
    if (m_used_tiles.contains(loc)) {
        const auto& pre_oper = m_used_tiles.at(loc);
        LogInfo << __FUNCTION__ << "| remove previous oper" << pre_oper << loc;
        m_battlefield_opers.erase(pre_oper);
        m_used_tiles.erase(loc);
    }

    register_deployed_oper(tag.role, tag.name, loc);
    return true;
}

bool asst::BattleHelper::is_skill_ready(const Point& loc, const cv::Mat& reusable)
{
    cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;

    auto target_iter = m_normal_tile_info.find(loc);
    if (target_iter == m_normal_tile_info.end()) {
        Log.error("No loc", loc);
        return false;
    }
    const Point& battlefield_point = target_iter->second.pos;
    static const Rect screen_rect = { 0, 0, WindowWidthDefault, WindowHeightDefault };
    if (!screen_rect.include(battlefield_point)) {
        return false;
    }
    BattlefieldClassifier skill_analyzer(image);
    skill_analyzer.set_object_of_interest({ .skill_ready = true });
    skill_analyzer.set_base_point(battlefield_point);

    return skill_analyzer.analyze()->skill_ready.ready;
}

bool asst::BattleHelper::is_skill_ready(const std::string& name, const cv::Mat& reusable)
{
    return is_skill_ready(battle::Role::Unknown, name, reusable);
}

bool asst::BattleHelper::is_skill_ready(battle::Role role, const std::string& name, const cv::Mat& reusable)
{
    auto oper_iter = std::ranges::find_if(m_battlefield_opers, [&](const auto& pair) {
        return (role == battle::Role::Unknown || pair.first.role == role) && pair.first.name == name;
    });
    if (oper_iter == m_battlefield_opers.cend()) {
        LogError << "No oper" << name << "role:" << enum_to_string(role);
        return false;
    }
    return is_skill_ready(oper_iter->second, reusable);
}

bool asst::BattleHelper::use_skill(const std::string& name, int timeout_ms)
{
    return use_skill(battle::Role::Unknown, name, timeout_ms);
}

bool asst::BattleHelper::use_skill(battle::Role role, const std::string& name, int timeout_ms)
{
    LogTraceFunction;

    auto oper_iter = std::ranges::find_if(m_battlefield_opers, [&](const auto& pair) {
        return (role == battle::Role::Unknown || pair.first.role == role) && pair.first.name == name;
    });
    if (oper_iter == m_battlefield_opers.cend()) {
        LogError << "No oper" << name << "role:" << enum_to_string(role);
        return false;
    }

    return use_skill(oper_iter->second, timeout_ms);
}

bool asst::BattleHelper::use_skill(const Point& loc, int timeout_ms)
{
    LogTraceFunction;

    return click_oper_on_battlefield(loc) && click_skill(timeout_ms) && m_inst_helper.sleep(200);
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

bool asst::BattleHelper::check_skip_plot_button(const cv::Mat& reusable)
{
    cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;

    Matcher battle_plot_analyzer(image);
    battle_plot_analyzer.set_task_info("SkipThePreBattlePlot");
    bool ret = battle_plot_analyzer.analyze().has_value();
    if (ret) {
        ProcessTask(this_task(), { "SkipThePreBattlePlot" }).run();
    }
    return ret;
}

bool asst::BattleHelper::check_avatar_dialog(const cv::Mat& reusable)
{
    cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;

    Matcher battle_plot_analyzer(image);
    battle_plot_analyzer.set_task_info("BattleAvatarDialog");
    bool ret = battle_plot_analyzer.analyze().has_value();
    if (ret) {
        ProcessTask(this_task(), { "BattleAvatarDialog" }).run();
    }
    return ret;
}

bool asst::BattleHelper::check_in_speedup(const cv::Mat& reusable)
{
    cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    Matcher analyzer(image);
    analyzer.set_task_info("BattleSpeedUpCheck");
    return analyzer.analyze().has_value();
}

bool asst::BattleHelper::check_in_battle(const cv::Mat& reusable, bool weak)
{
    cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    if (weak) {
        BattlefieldMatcher analyzer(image);
        auto result = analyzer.analyze();
        m_in_battle = result.has_value();
        if (m_in_battle && !result->pause_button) {
            if (check_skip_plot_button(image)) {
                if (m_in_speedup && !check_in_speedup()) {
                    speed_up(); // 跳过剧情会退出2倍速
                }
            }
            else if (check_avatar_dialog(image)) {
                if (m_in_speedup && !check_in_speedup()) {
                    speed_up(); // 跳过剧情会退出2倍速
                    m_inst_helper.sleep(Config.get_options().task_delay);
                    int times = 0;
                    while (times < 20 && !m_inst_helper.need_exit() && !check_in_speedup()) {
                        speed_up(); // 跳过剧情会退出2倍速
                        m_inst_helper.sleep(Config.get_options().task_delay);
                        times++;
                    }
                }
            }
        }
    }
    else {
        check_skip_plot_button(image);
        m_in_battle = check_pause_button(image);
    }
    return m_in_battle;
}

bool asst::BattleHelper::wait_until_start(bool weak)
{
    LogTraceFunction;

    const auto timeout_duration = std::chrono::seconds(Config.get_options().battle_start_timeout_seconds);
    const auto start_time = std::chrono::steady_clock::now();

    cv::Mat image = m_inst_helper.ctrler()->get_image();
    while (!m_inst_helper.need_exit() && !check_in_battle(image, weak)) {
        if (std::chrono::steady_clock::now() - start_time > timeout_duration) {
            Log.warn("Timeout reached while waiting to start the battle.");
            return false;
        }

        std::this_thread::yield();
        image = m_inst_helper.ctrler()->get_image();
    }
    return true;
}

bool asst::BattleHelper::wait_until_end(bool weak, bool do_strategy)
{
    LogTraceFunction;

    cv::Mat image = m_inst_helper.ctrler()->get_image();
    while (!m_inst_helper.need_exit() && check_in_battle(image, weak)) {
        if (do_strategy) {
            do_strategic_action(image);
        }
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

bool asst::BattleHelper::run_battle_assist(
    bool auto_use_skills,
    bool auto_speed_up,
    std::chrono::milliseconds skill_scan_interval)
{
    LogTraceFunction;

    auto image = m_inst_helper.ctrler()->get_image();
    if (!check_in_battle(image, false)) {
        LogWarn << __FUNCTION__ << "battle is not active";
        return false;
    }

    if (auto_speed_up) {
        m_in_speedup = check_in_speedup(image);
        if (m_in_speedup) {
            LogInfo << __FUNCTION__ << "2x battle speed already enabled";
        }
        else if (speed_up()) {
            m_in_speedup = true;
            LogInfo << __FUNCTION__ << "enabled 2x battle speed";
        }
    }

    using clock = std::chrono::steady_clock;
    auto last_skill_scan = clock::now() - skill_scan_interval;

    while (!m_inst_helper.need_exit()) {
        image = m_inst_helper.ctrler()->get_image();
        if (!check_in_battle(image, false)) {
            break;
        }

        if (auto_use_skills && clock::now() - last_skill_scan >= skill_scan_interval) {
            use_all_ready_skills_on_battlefield(image);
            last_skill_scan = clock::now();
        }
        m_inst_helper.sleep(200);
    }
    return true;
}

bool asst::BattleHelper::use_all_ready_skills_on_battlefield(const cv::Mat& reusable)
{
    const cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    const double image_scale_x = static_cast<double>(image.cols) / WindowWidthDefault;
    const double image_scale_y = static_cast<double>(image.rows) / WindowHeightDefault;

    struct SkillMarker
    {
        Point center;
        Rect rect;
        float score = 0.0f;
        bool from_fallback = false;
    };

    cv::Mat hsv;
    cv::Mat yellow_mask;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(18, 120, 140), cv::Scalar(40, 255, 255), yellow_mask);
    cv::morphologyEx(
        yellow_mask,
        yellow_mask,
        cv::MORPH_CLOSE,
        cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(yellow_mask, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);
    std::vector<SkillMarker> markers;
    const auto skill_roi_move = Task.get<MatchTaskInfo>("BattleSkillReady")->rect_move;
    const auto scale_x = [image_scale_x](double value) {
        return std::max(1, static_cast<int>(std::lround(value * image_scale_x)));
    };
    const auto scale_y = [image_scale_y](double value) {
        return std::max(1, static_cast<int>(std::lround(value * image_scale_y)));
    };
    const auto marker_ready_score_for_image = [&](const cv::Mat& source, const Point& marker_center) {
        const Point classifier_base {
            marker_center.x - (skill_roi_move.x + skill_roi_move.width / 2),
            marker_center.y - (skill_roi_move.y + skill_roi_move.height / 2),
        };
        BattlefieldClassifier classifier(source);
        classifier.set_object_of_interest({ .skill_ready = true });
        classifier.set_base_point(classifier_base);
        const auto classifier_result = classifier.analyze();
        if (!classifier_result || !classifier_result->skill_ready.ready) {
            return 0.0f;
        }
        return classifier_result->skill_ready.score;
    };
    const auto marker_ready_score = [&](const Point& marker_center) {
        return marker_ready_score_for_image(image, marker_center);
    };
    for (size_t contour_index = 0; contour_index < contours.size(); ++contour_index) {
        // Ignore holes; they are examined through their parent yellow contour below.
        if (hierarchy[contour_index][3] != -1) {
            continue;
        }

        const auto& contour = contours[contour_index];
        const double area = cv::contourArea(contour);
        const cv::Rect marker_rect = cv::boundingRect(contour);
        if (marker_rect.width <= 0 || marker_rect.height <= 0) {
            continue;
        }

        const double aspect_ratio = static_cast<double>(marker_rect.width) / marker_rect.height;
        const double fill_ratio = area / static_cast<double>(marker_rect.width * marker_rect.height);
        if (area < 55.0 * image_scale_x * image_scale_y || marker_rect.width < scale_x(14) ||
            marker_rect.height < scale_y(14) || marker_rect.width > scale_x(72) ||
            marker_rect.height > scale_y(72) || aspect_ratio < 0.58 || aspect_ratio > 1.5 ||
            fill_ratio < 0.18 || fill_ratio > 0.85) {
            continue;
        }

        const Point marker_center {
            marker_rect.x + marker_rect.width / 2,
            marker_rect.y + marker_rect.height / 2,
        };
        const bool duplicate = std::ranges::any_of(markers, [&](const SkillMarker& marker) {
            return std::abs(marker.center.x - marker_center.x) < scale_x(30) &&
                   std::abs(marker.center.y - marker_center.y) < scale_y(30);
        });
        if (duplicate) {
            continue;
        }

        // Center the classifier ROI on the detected marker itself. This avoids relying on an
        // operator-relative crop, which fails for top-screen HUD overlap and tightly packed units.
        const float ready_score = marker_ready_score(marker_center);
        if (ready_score < 0.75f) {
            continue;
        }

        markers.emplace_back(SkillMarker {
            marker_center,
            Rect { marker_rect.x, marker_rect.y, marker_rect.width, marker_rect.height },
            ready_score,
            false,
        });
    }

    struct UnitCandidate
    {
        Point center;
        Rect rect;
        float score;
    };

    BattlefieldDetector detector(image);
    detector.set_object_of_interest({ .operators = true });
    const auto detector_result = detector.analyze();
    std::vector<UnitCandidate> units;
    if (detector_result) {
        for (const auto& operator_result : detector_result->operators) {
            if (operator_result.score < 0.3f) {
                continue;
            }

            const Rect& oper_rect = operator_result.rect;
            units.emplace_back(UnitCandidate {
                Point {
                    oper_rect.x + oper_rect.width / 2,
                    oper_rect.y + oper_rect.height / 2,
                },
                oper_rect,
                operator_result.score,
            });
        }
    }

    // The marker's vertical distance from the health bar changes with zoom and unit layout.
    // Scanning several positions above each detected battlefield unit recovers icons whose
    // contour was merged or hidden. Reserve-card entries have no battlefield health bar and can
    // therefore never be added by this fallback.
    static constexpr std::array<int, 5> marker_scan_offsets { 55, 80, 105, 130, 155 };
    for (const auto& unit : units) {
        float best_score = 0.0f;
        Point best_marker_center;
        for (const int offset : marker_scan_offsets) {
            const Point marker_center {
                unit.center.x,
                unit.center.y - scale_y(offset),
            };
            if (marker_center.y < scale_y(16) || marker_center.y >= image.rows - scale_y(16)) {
                continue;
            }
            const float ready_score = marker_ready_score(marker_center);
            if (ready_score > best_score) {
                best_score = ready_score;
                best_marker_center = marker_center;
            }
        }
        if (best_score < 0.75f) {
            continue;
        }

        const bool duplicate = std::ranges::any_of(markers, [&](const SkillMarker& marker) {
            return std::abs(marker.center.x - best_marker_center.x) < scale_x(30) &&
                   std::abs(marker.center.y - best_marker_center.y) < scale_y(30);
        });
        if (duplicate) {
            continue;
        }
        markers.emplace_back(SkillMarker {
            best_marker_center,
            Rect {
                best_marker_center.x - scale_x(16),
                best_marker_center.y - scale_y(16),
                scale_x(32),
                scale_y(32),
            },
            best_score,
            true,
        });
    }

    std::ranges::sort(markers, {}, [](const SkillMarker& marker) {
        return std::pair { marker.center.y, marker.center.x };
    });
    std::ranges::sort(units, {}, [](const UnitCandidate& unit) {
        return std::pair { unit.center.y, unit.center.x };
    });

    LogDebug << __FUNCTION__ << "skill markers" << markers.size() << "detected units" << units.size()
             << VAR(image_scale_x) << VAR(image_scale_y);
    for (const auto& marker : markers) {
        LogDebug << __FUNCTION__ << "ready marker" << marker.center << marker.rect << marker.score << "fallback"
                 << marker.from_fallback;
    }

    const auto marker_cooldown_key = [](const Point& point) {
        constexpr int bucket_size = 24;
        return Point {
            point.x / bucket_size * bucket_size,
            point.y / bucket_size * bucket_size,
        };
    };
    std::set<Point> current_markers;
    for (const auto& marker : markers) {
        current_markers.insert(marker_cooldown_key(marker.center));
    }
    for (const auto& previous : m_battle_assist_present_markers) {
        if (!current_markers.contains(previous)) {
            m_last_auto_skill_use.erase(previous);
        }
    }
    m_battle_assist_present_markers = std::move(current_markers);

    bool used = false;
    const auto now = std::chrono::steady_clock::now();
    constexpr auto same_tile_cooldown = std::chrono::seconds(8);
    // Live runs show that +80 is the reliable operator-body offset most often, while +40
    // remains useful for shorter or higher-positioned sprites. Try the frequent hit first so
    // the common case no longer requires a wasted click before the detail panel opens.
    static constexpr std::array<int, 4> click_offsets_y { 80, 40, 110, 140 };
    struct ReadyMatch
    {
        Rect rect;
        float score = 0.0f;
        double scale = 1.0;
        std::string_view templ_name;
        bool cache_hit = false;
    };
    const auto find_ready_row = [&](const cv::Mat& screenshot) -> std::optional<ReadyMatch> {
        static constexpr std::string_view arrow_name = "BattleSkillReadyOnClick-TopView2.png";
        static constexpr std::string_view row_name = "BattleSkillReadyOnClick-TopView.png";
        static constexpr float threshold = 0.78f;
        static constexpr std::array common_arrow_scales { 0.65, 0.75, 0.85 };
        static constexpr std::array fallback_scales { 0.55, 0.95, 1.05, 1.15, 1.25, 1.35, 1.45 };
        const Rect search_roi {
            scale_x(600),
            scale_y(250),
            scale_x(430),
            scale_y(400),
        };
        const cv::Mat arrow_template = TemplResource::get_instance().get_templ(std::string(arrow_name));
        if (arrow_template.empty()) {
            return std::nullopt;
        }
        const cv::Mat row_template = TemplResource::get_instance().get_templ(std::string(row_name));

        const auto match_scaled =
            [&](const cv::Mat& templ, std::string_view templ_name, double scale, const Rect& roi)
            -> std::optional<ReadyMatch> {
            cv::Mat scaled_templ;
            cv::resize(
                templ,
                scaled_templ,
                cv::Size(),
                scale,
                scale,
                scale < 1.0 ? cv::INTER_AREA : cv::INTER_CUBIC);
            Matcher ready_matcher(screenshot);
            ready_matcher.set_templ(scaled_templ);
#ifdef ASST_DEBUG
            ready_matcher.set_threshold(0.0f);
#else
            ready_matcher.set_threshold(threshold);
#endif
            ready_matcher.set_roi(roi);
            ready_matcher.set_method(MatchMethod::Ccoeff);
            const auto result = ready_matcher.analyze();
            if (!result) {
                return std::nullopt;
            }
            LogDebug << __FUNCTION__ << "READY candidate" << templ_name << "scale" << scale << "score"
                     << result->score << "rect" << result->rect << "roi" << roi;
            if (result->score < threshold) {
                return std::nullopt;
            }
            ReadyMatch match {
                result->rect,
                static_cast<float>(result->score),
                scale,
                templ_name,
                false,
            };
            if (match.templ_name == arrow_name) {
                const double arrow_scale = static_cast<double>(match.rect.width) / 20.0;
                match.rect.width = std::lround(95.0 * arrow_scale);
                match.rect.height = std::lround(22.0 * arrow_scale);
                match.scale = arrow_scale;
            }
            return match;
        };
        const auto remember = [&](ReadyMatch match) {
            m_battle_assist_ready_scale = match.scale;
            m_battle_assist_ready_rect = match.rect;
            return match;
        };

        if (m_battle_assist_ready_scale && m_battle_assist_ready_rect) {
            const Rect& cached = *m_battle_assist_ready_rect;
            const Rect fast_roi {
                std::clamp(cached.x - scale_x(24), 0, screenshot.cols - 1),
                std::clamp(cached.y - scale_y(24), 0, screenshot.rows - 1),
                scale_x(110),
                scale_y(80),
            };
            if (auto match =
                    match_scaled(arrow_template, arrow_name, *m_battle_assist_ready_scale, fast_roi)) {
                match->cache_hit = true;
                return remember(*match);
            }
        }

        std::optional<ReadyMatch> best_match;
        for (const double scale : common_arrow_scales) {
            if (auto match = match_scaled(arrow_template, arrow_name, scale, search_roi);
                match && (!best_match || match->score > best_match->score)) {
                best_match = match;
            }
        }
        if (best_match && best_match->score >= 0.9f) {
            return remember(*best_match);
        }

        for (const double scale : fallback_scales) {
            if (auto match = match_scaled(arrow_template, arrow_name, scale, search_roi);
                match && (!best_match || match->score > best_match->score)) {
                best_match = match;
            }
        }
        if (!best_match && !row_template.empty()) {
            for (const double scale : common_arrow_scales) {
                if (auto match = match_scaled(row_template, row_name, scale, search_roi);
                    match && (!best_match || match->score > best_match->score)) {
                    best_match = match;
                }
            }
            for (const double scale : fallback_scales) {
                if (auto match = match_scaled(row_template, row_name, scale, search_roi);
                    match && (!best_match || match->score > best_match->score)) {
                    best_match = match;
                }
            }
        }
        return best_match ? std::optional<ReadyMatch>(remember(*best_match)) : std::nullopt;
    };
#ifdef ASST_DEBUG
    const auto save_skill_panel_crop = [&](const cv::Mat& screenshot,
                                           const SkillMarker& marker,
                                           int click_offset,
                                           std::string_view phase,
                                           const std::optional<ReadyMatch>& match) {
        const Rect panel_roi {
            scale_x(600),
            scale_y(250),
            scale_x(430),
            scale_y(400),
        };
        std::string suffix = std::to_string(marker.center.x) + "_" + std::to_string(marker.center.y) + "_off" +
                             std::to_string(click_offset);
        if (match) {
            suffix += match->cache_hit ? "_cache_hit" : "_search_hit";
        }
        else {
            suffix += "_miss";
        }
        utils::save_debug_image(
            make_roi(screenshot, panel_roi),
            utils::path("debug") / "battle_assist" / std::string(phase),
            true,
            "skill panel",
            suffix);
    };
#endif
    for (const auto& marker : markers) {
        if (m_inst_helper.need_exit()) {
            break;
        }
        const Point cooldown_key = marker_cooldown_key(marker.center);
        if (auto iter = m_last_auto_skill_use.find(cooldown_key);
            iter != m_last_auto_skill_use.cend() && now - iter->second < same_tile_cooldown) {
            continue;
        }

        for (const int click_offset_y : click_offsets_y) {
            if (m_inst_helper.need_exit()) {
                break;
            }
            const Point click_point {
                marker.center.x,
                marker.center.y + scale_y(click_offset_y),
            };
            if (click_point.y >= image.rows) {
                continue;
            }
            if (!m_inst_helper.ctrler()->click(click_point)) {
                continue;
            }
            const auto selection_time = std::chrono::steady_clock::now();
            m_inst_helper.sleep(280);

            cv::Mat detail_image = m_inst_helper.ctrler()->get_image();
            auto ready_result = find_ready_row(detail_image);
            if (!ready_result) {
                // The detail panel can finish animating just after the first screenshot. Check
                // once more before clearing the selection and retrying at another offset.
                m_inst_helper.sleep(120);
                detail_image = m_inst_helper.ctrler()->get_image();
                ready_result = find_ready_row(detail_image);
            }
#ifdef ASST_DEBUG
            save_skill_panel_crop(detail_image, marker, click_offset_y, "before_click", ready_result);
#endif
            bool ready_selected = false;
            if (ready_result) {
                ready_selected = true;
                m_last_auto_skill_use[cooldown_key] = now;
                const Rect& ready_rect = ready_result->rect;
                const Point skill_center {
                    ready_rect.x + ready_rect.width / 2,
                    ready_rect.y - std::lround(ready_rect.height * 51.5 / 22.0),
                };
                if (m_inst_helper.ctrler()->click(skill_center)) {
                    used = true;
                    const auto latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                std::chrono::steady_clock::now() - selection_time)
                                                .count();
                    LogInfo << __FUNCTION__ << "clicked skill icon above READY" << skill_center << "READY rect"
                            << ready_rect << "score" << ready_result->score << "template"
                            << ready_result->templ_name << "scale" << ready_result->scale << "cache"
                            << ready_result->cache_hit << "marker" << marker.center << "fallback"
                            << marker.from_fallback << "unit click" << click_point << "selection latency"
                            << latency_ms << "ms";
#ifdef ASST_DEBUG
                    m_inst_helper.sleep(120);
                    const cv::Mat after_click_image = m_inst_helper.ctrler()->get_image();
                    const auto after_click_ready = find_ready_row(after_click_image);
                    save_skill_panel_crop(after_click_image, marker, click_offset_y, "after_click", after_click_ready);
                    LogDebug << __FUNCTION__ << "after skill click READY present"
                             << static_cast<bool>(after_click_ready) << "score"
                             << (after_click_ready ? after_click_ready->score : 0.0f) << "cache"
                             << (after_click_ready ? after_click_ready->cache_hit : false) << "marker"
                             << marker.center;
#endif
                }
            }
            else {
                LogDebug << __FUNCTION__ << "READY row not found, try deeper click below marker"
                         << click_point << "marker" << marker.center << "fallback" << marker.from_fallback;
            }

            m_inst_helper.sleep(250);
            // BattleCancelSelection clicks this exact position. Do it directly to keep the loop
            // independent of nested ProcessTask state.
            m_inst_helper.ctrler()->click({ 640, 10 });
            m_inst_helper.sleep(500);
#ifdef ASST_DEBUG
            if (ready_selected) {
                const cv::Mat battlefield_after = m_inst_helper.ctrler()->get_image();
                const Rect marker_crop {
                    std::clamp(marker.center.x - scale_x(80), 0, battlefield_after.cols - 1),
                    std::clamp(marker.center.y - scale_y(80), 0, battlefield_after.rows - 1),
                    scale_x(160),
                    scale_y(160),
                };
                const Rect safe_marker_crop {
                    marker_crop.x,
                    marker_crop.y,
                    std::min(marker_crop.width, battlefield_after.cols - marker_crop.x),
                    std::min(marker_crop.height, battlefield_after.rows - marker_crop.y),
                };
                const float marker_score_after =
                    marker_ready_score_for_image(battlefield_after, marker.center);
                const std::string suffix = std::to_string(marker.center.x) + "_" +
                                           std::to_string(marker.center.y) + "_" +
                                           std::to_string(std::lround(marker_score_after * 1000));
                utils::save_debug_image(
                    make_roi(battlefield_after, safe_marker_crop),
                    utils::path("debug") / "battle_assist" / "battlefield_after",
                    true,
                    "battlefield marker after skill click",
                    suffix);
                LogDebug << __FUNCTION__ << "after cancel marker score" << marker_score_after << "marker"
                         << marker.center << "fallback" << marker.from_fallback << "cooldown key"
                         << cooldown_key;
            }
#endif
            if (ready_selected) {
                break;
            }
        }
    }
    return used;
}

bool asst::BattleHelper::use_all_ready_skill(const cv::Mat& reusable)
{
    // TODO: 可配置延迟时间
    static constexpr auto min_frame_interval = std::chrono::milliseconds(1500);

    bool used = false;
    const auto now = std::chrono::steady_clock::now();
    const cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    for (const auto& [oper_tag, loc] : m_battlefield_opers) {
        const auto& skill_it = std::ranges::find_if(m_skill_usage, [&](const auto& pair) {
            return (pair.first.role == battle::Role::Unknown || pair.first.role == oper_tag.role) &&
                   pair.first.name == oper_tag.name;
        });
        battle::OperNameTag skill_tag;
        if (skill_it == m_skill_usage.cend()) {
            skill_tag = oper_tag;
        }
        else {
            skill_tag = skill_it->first;
        }
        auto& usage = m_skill_usage[skill_tag];
        auto& times = m_skill_times[skill_tag];
        auto& retry = m_skill_error_count[skill_tag];
        auto& last_use_time = m_last_use_skill_time[oper_tag];
        if (usage != SkillUsage::Possibly && usage != SkillUsage::Times) {
            continue;
        }

        if (auto interval = now - last_use_time; min_frame_interval > interval) {
            LogInfo << oper_tag.name << "analyze skill too fast, interval time:"
                    << std::chrono::duration_cast<std::chrono::milliseconds>(interval).count() << " ms";
            continue;
        }

        if (!is_skill_ready(loc, image)) {
            continue;
        }

        LogInfo << "Skill" << oper_tag.name << "is ready";

        // 识别到了，但点进去发现没有。一般来说是识别错了
        if (!use_skill(loc, 0)) {
            LogWarn << "Skill" << oper_tag.name << "is not ready";
            static const bool save_infinitely = std::filesystem::exists("DEBUG_skill_ready.txt");
            if (!save_infinitely) {
                constexpr int MaxRetry = 3;
                if (++retry >= MaxRetry) {
                    LogWarn << "Do not use skill anymore" << oper_tag.name;
                    usage = SkillUsage::NotUse;
                }
            }
            continue;
        }
        used = true;
        retry = 0;
        m_last_use_skill_time[oper_tag] = now;

        if (usage == SkillUsage::Times) {
            times--;
            if (times == 0) {
                usage = SkillUsage::TimesUsed;
            }
        }
        // image = m_inst_helper.ctrler()->get_image();
    }

    return used;
}

bool asst::BattleHelper::check_and_use_skill(const std::string& name, bool& has_error, const cv::Mat& reusable)
{
    return check_and_use_skill(battle::Role::Unknown, name, has_error, reusable);
}

bool asst::BattleHelper::check_and_use_skill(
    battle::Role role,
    const std::string& name,
    bool& has_error,
    const cv::Mat& reusable)
{
    auto oper_iter = std::ranges::find_if(m_battlefield_opers, [&](const auto& pair) {
        return (role == battle::Role::Unknown || pair.first.role == role) && pair.first.name == name;
    });
    if (oper_iter == m_battlefield_opers.cend()) {
        LogError << "No oper" << name << "role" << enum_to_string(role);
        return false;
    }
    return check_and_use_skill(oper_iter->second, has_error, reusable);
}

bool asst::BattleHelper::check_and_use_skill(const Point& loc, bool& has_error, const cv::Mat& reusable)
{
    const cv::Mat image = reusable.empty() ? m_inst_helper.ctrler()->get_image() : reusable;
    if (!is_skill_ready(loc, image)) {
        return false;
    }
    has_error = !use_skill(loc, 0);
    return true;
}

void asst::BattleHelper::save_map(const cv::Mat& image)
{
    LogTraceFunction;

    auto draw = image.clone();

    for (const auto& [loc, info] : m_normal_tile_info) {
        cv::circle(draw, cv::Point(info.pos.x, info.pos.y), 5, cv::Scalar(0, 255, 0), -1);
        cv::putText(draw, loc.to_string(), cv::Point(info.pos.x - 30, info.pos.y), 1, 1.5, cv::Scalar(0, 0, 255), 2);
    }

    std::string suffix;
    if (++m_camera_count > 1) {
        suffix = "-" + std::to_string(m_camera_count);
    }
    std::vector<int> jpeg_params = { cv::IMWRITE_JPEG_QUALITY, 50, cv::IMWRITE_JPEG_OPTIMIZE, 1 };
    utils::save_debug_image(draw, utils::path("debug") / "map", true, "", m_stage_name + suffix, "jpeg", jpeg_params);
}

bool asst::BattleHelper::click_oper_on_deployment(const std::string& name)
{
    LogTraceFunction;

    auto rect_opt = get_oper_rect_on_deployment(battle::Role::Unknown, name);
    if (!rect_opt) {
        return false;
    }

    return click_oper_on_deployment(*rect_opt);
}

bool asst::BattleHelper::click_oper_on_deployment(battle::Role role, const std::string& name)
{
    LogTraceFunction;

    auto rect_opt = get_oper_rect_on_deployment(role, name);
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
    return click_oper_on_battlefield(battle::Role::Unknown, name);
}

bool asst::BattleHelper::click_oper_on_battlefield(battle::Role role, const std::string& name)
{
    LogTraceFunction;

    auto oper_iter = std::ranges::find_if(m_battlefield_opers, [&](const auto& pair) {
        return (role == battle::Role::Unknown || pair.first.role == role) && pair.first.name == name;
    });
    if (oper_iter == m_battlefield_opers.cend()) {
        LogError << "No oper" << name << "role" << enum_to_string(role);
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
    bool deploy_with_pause =
        ControlFeat::support(m_inst_helper.ctrler()->support_features(), ControlFeat::SWIPE_WITH_PAUSE);

    if (deploy_with_pause) {
        ProcessTask(this_task(), { "BattlePause" }).run();
    }
    // return ProcessTask(this_task(), { "BattleOperRetreatJustClick" }).run();
    bool ret = m_inst_helper.ctrler()->click(m_retreat_button_pos);

    if (deploy_with_pause) {
        ProcessTask(this_task(), { "BattlePauseCancel" }).run();
    }
    return ret;
}

bool asst::BattleHelper::click_skill(int timeout_ms)
{
    LogTraceFunction;
    const auto start_time = std::chrono::steady_clock::now();
    bool deploy_with_pause =
        ControlFeat::support(m_inst_helper.ctrler()->support_features(), ControlFeat::SWIPE_WITH_PAUSE);

    bool pausing = false;
    if (timeout_ms == 0 && deploy_with_pause) {
        pausing = ProcessTask(this_task(), { "BattlePause" }).run();
    }

    cv::Mat top_view;
    cv::Mat image;
    int retry = 0;
    while (!m_inst_helper.need_exit()) {
        image = m_inst_helper.ctrler()->get_image();
        if (retry > 0 && (retry % 10 == 0) && !check_in_battle(image)) {
            return false;
        }
        top_view = get_top_view(image, true, m_has_multi_stages);
        Matcher skill_analyzer { top_view };
        skill_analyzer.set_task_info("BattleSkillReadyOnClick-TopView");
        skill_analyzer.set_roi({ 250, 250, 250, 250 });
        if (skill_analyzer.analyze()) {
            m_inst_helper.ctrler()->click(m_skill_button_pos);
            if (pausing) {
                ProcessTask(this_task(), { "BattlePauseCancel" }).run();
            }
            return true;
        }
        if (timeout_ms > -1) {
            const auto elapsed_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time)
                    .count();
            if (elapsed_ms >= timeout_ms) {
                break;
            }
        }
        m_inst_helper.sleep(Config.get_options().task_delay);
        ++retry;
    }

    // this means false positive in skill ready detection

#ifdef ASST_DEBUG
    if (!top_view.empty()) {
        // false positive 样本用作技能就绪识别的数据集，不做自动清理
        utils::save_debug_image(top_view, utils::path("debug") / "skill", false, "", m_stage_name);
    }
#endif
    cancel_oper_selection();
    if (pausing) {
        ProcessTask(this_task(), { "BattlePauseCancel" }).run();
    }
    return false;
}

bool asst::BattleHelper::cancel_oper_selection()
{
    return ProcessTask(this_task(), { "BattleCancelSelection" }).run();
}

void asst::BattleHelper::fix_swipe_out_of_limit(
    Point& p1,
    Point& p2,
    int width,
    int height,
    int max_distance,
    double radian)
{
    Point direct = Point::zero();
    int distance = 0;
    if (p2.y > height) {
        // 下边界超限
        direct = Point::up();
        distance = p2.y - height;
    }
    else if (p2.x > width) {
        // 右边界超限
        direct = Point::left();
        distance = p2.x - width;
    }
    else if (p2.y < 0) {
        // 上边界超限
        direct = Point::down();
        distance = -p2.y;
    }
    else if (p2.x < 0) {
        // 左边界超限
        direct = Point::right();
        distance = -p2.x;
    }
    else {
        return;
    }

    std::tuple<double, double> adjust_scale = {
        direct.x * std::cos(radian) - direct.y * std::sin(radian),
        direct.y * std::cos(radian) + direct.x * std::sin(radian),
    };

    // 旋转后偏移值会不够，计算补偿比例
    double adjust_more = std::get<0>(adjust_scale) * direct.x + std::get<1>(adjust_scale) * direct.y;

    Point adjust = {
        static_cast<int>(std::get<0>(adjust_scale) / adjust_more * distance),
        static_cast<int>(std::get<1>(adjust_scale) / adjust_more * distance),
    };

    if (auto point_distance = Point::distance(adjust, { 0, 0 }); point_distance > max_distance) {
        adjust = {
            static_cast<int>(adjust.x * max_distance / point_distance),
            static_cast<int>(adjust.y * max_distance / point_distance),
        };
    }

    Log.info(__FUNCTION__, "swipe end_point out of limit, start:", p1, ", end:", p2, ", adjust:", adjust);
    p1 += adjust;
    p2 += adjust;
}

bool asst::BattleHelper::move_camera(const std::pair<double, double>& delta, bool keep_kills)
{
    LogTraceFunction;
    Log.info("move", delta.first, delta.second);

    update_kills(m_inst_helper.ctrler()->get_image());

    // keep_kills 为 false 时（引航者试炼转场）：等待当前波次结束并归零击杀数；
    // 为 true 时（同一波内移动镜头）：跳过等待与归零，击杀数保持连续
    if (!keep_kills) {
        // 还没转场的时候
        if (m_kills != 0) {
            wait_until_end(false);
        }

        m_kills = 0;
        m_total_kills = 0;
    }

    m_camera_shift.first += delta.first;
    m_camera_shift.second += delta.second;

    calc_tiles_info(m_stage_name, -m_camera_shift.first, m_camera_shift.second);
    return update_deployment(true);
}

std::string asst::BattleHelper::analyze_detail_page_oper_name(const cv::Mat& image, battle::Role role)
{
    const auto& replace_task = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    const auto& task = Task.get<OcrTaskInfo>(oper_name_ocr_task_name());

    // 使用 OperNameAnalyzer 处理左对齐文本
    OperNameAnalyzer preproc_analyzer(image);
    preproc_analyzer.set_task_info(task);
    preproc_analyzer.set_text_alignment(OperNameAnalyzer::TextAlignment::Left); // 左对齐
    const auto& params = task->special_params;
    preproc_analyzer.set_bin_threshold(params[0]);
    preproc_analyzer.set_bin_expansion(params[1]);
    preproc_analyzer.set_bin_trim_threshold(params[2], params[3]);
    preproc_analyzer.set_bottom_line_height(params[4]);
    preproc_analyzer.set_width_threshold(params[5]);
    preproc_analyzer.set_replace(replace_task->replace_map, replace_task->replace_full);
    auto preproc_result_opt = preproc_analyzer.analyze();

    if (preproc_result_opt && !BattleData.is_name_invalid(role, preproc_result_opt->text)) {
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

    if (!BattleData.is_name_invalid(role, det_name)) {
        return det_name;
    }

    det_analyzer.save_img(utils::path("debug") / utils::path("battle"));
    return std::string();
}

std::optional<asst::Rect>
    asst::BattleHelper::get_oper_rect_on_deployment(battle::Role role, const std::string& name) const
{
    auto oper_iter = std::ranges::find_if(m_cur_deployment_opers, [&](const auto& oper) {
        return (role == battle::Role::Unknown || oper.role == role) && oper.name == name;
    });
    if (oper_iter == m_cur_deployment_opers.end()) {
        LogError << "No oper" << name << "with role" << enum_to_string(role);
        return std::nullopt;
    }

    return oper_iter->rect;
}

int asst::BattleHelper::elapsed_time()
{
    if (!m_stopwatch_enabled) {
        return -1;
    }
    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_stopwatch_start_time).count();
    if (elapsed_ms > std::numeric_limits<int>::max()) {
        Log.error(__FUNCTION__, "| elapsed time exceeds int maximum");
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(elapsed_ms);
}

void asst::BattleHelper::register_deployed_oper(battle::Role role, const std::string& name, const Point& loc)
{
    m_used_tiles.emplace(loc, battle::OperNameTag { role, name });
    m_battlefield_opers.emplace(battle::OperNameTag { role, name }, loc);
    m_last_use_skill_time.emplace(battle::OperNameTag { role, name }, std::chrono::steady_clock::time_point());
}

void asst::BattleHelper::remove_cooling_from_battlefield(const battle::DeploymentOper& oper)
{
    if (!oper.cooling) {
        return;
    }

    auto iter = std::ranges::find_if(m_battlefield_opers, [&](const auto& pair) {
        return pair.first.role == oper.role && pair.first.name == oper.name;
    });
    if (iter == m_battlefield_opers.end()) {
        return;
    }
    auto loc = iter->second;
    m_used_tiles.erase(loc);
    m_battlefield_opers.erase(iter);
}
