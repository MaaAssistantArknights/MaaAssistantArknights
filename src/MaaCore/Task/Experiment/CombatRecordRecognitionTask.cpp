#include "CombatRecordRecognitionTask.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/TilePack.h"
#include "Config/TaskData.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Utils/Ranges.hpp"
#include "Vision/Battle/BattleFormationAnalyzer.h"
#include "Vision/Battle/BattlefieldClassifier.h"
#include "Vision/Battle/BattlefieldDetector.h"
#include "Vision/Battle/BattlefieldMatcher.h"
#include "Vision/BestMatcher.h"
#include "Vision/RegionOCRer.h"

#include <unordered_map>
#include <unordered_set>

bool asst::CombatRecordRecognitionTask::set_video_path(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) {
        Log.error(__FUNCTION__, "filename not exists", path);
        return false;
    }
    m_video_path = path;
    return true;
}

bool asst::CombatRecordRecognitionTask::_run()
{
    LogTraceFunction;

    auto release_video = [](cv::VideoCapture* video) {
        if (video && video->isOpened()) {
            video->release();
        }
    };
    auto crt_path = utils::path_to_crt_string(m_video_path);
    m_video_ptr = std::shared_ptr<cv::VideoCapture>(new cv::VideoCapture(crt_path), release_video);

    if (!m_video_ptr->isOpened()) {
        Log.error(__FUNCTION__, "video_io open failed", m_video_path);
        return false;
    }
    m_video_fps = m_video_ptr->get(cv::CAP_PROP_FPS);
    m_video_frame_count = static_cast<size_t>(m_video_ptr->get(cv::CAP_PROP_FRAME_COUNT));
    m_battle_start_frame = 0;
    m_scale = WindowHeightDefault / m_video_ptr->get(cv::CAP_PROP_FRAME_HEIGHT);

#ifdef ASST_DEBUG
    cv::namedWindow(DrawWindow, cv::WINDOW_AUTOSIZE);
#endif // ASST_DEBUG

    if (!analyze_formation()) {
        Log.error(__FUNCTION__, "failed to analyze formation");
        return false;
    }

    if (!analyze_stage()) {
        Log.error(__FUNCTION__, "unknown stage");
        return false;
    }

    if (!analyze_deployment()) {
        Log.error(__FUNCTION__, "failed to match deployment");
        return false;
    }

    if (!slice_video()) {
        Log.error(__FUNCTION__, "failed to slice");
        return false;
    }

    ClipInfo* pre_valid = nullptr;
    for (auto iter = m_clips.begin(); iter != m_clips.end(); ++iter) {
        auto& clip = *iter;

        if (!clip.deployment_changed && iter != m_clips.begin()) {
            compare_skill(clip, *(iter - 1));
            continue;
        }

        if (!analyze_clip(clip, pre_valid)) {
            Log.error(__FUNCTION__, "failed to analyze clip");
            return false;
        }
        pre_valid = &clip;
    }

    Log.info("full copilot json", m_copilot_json.to_string());

    std::string filename = "MaaAI_" + m_stage_name + "_" + utils::path_to_utf8_string(m_video_path.stem()) + "_" +
                           utils::get_time_filestem() + ".json";
    auto filepath = UserDir.get() / "cache" / "CombatRecord" / utils::path(filename);
    std::filesystem::create_directories(filepath.parent_path());
    std::ofstream osf(filepath);
    osf << m_copilot_json.format();
    osf.close();

    auto cb_json = basic_info_with_what("Finished");
    cb_json["details"]["filename"] = utils::path_to_utf8_string(filepath);
    callback(AsstMsg::SubTaskExtraInfo, cb_json);

#ifdef ASST_DEBUG
    cv::destroyWindow(DrawWindow);
#endif // ASST_DEBUG

    return true;
}

bool asst::CombatRecordRecognitionTask::analyze_formation()
{
    LogTraceFunction;
    callback(AsstMsg::SubTaskStart, basic_info_with_what("OcrFormation"));

    const int skip_count = m_video_fps > m_formation_fps ? static_cast<int>(m_video_fps / m_formation_fps) - 1 : 0;

    BattleFormationAnalyzer formation_ananlyzer;
    int no_changes_count = 0;
    for (size_t i = 0; i < m_video_frame_count; i += skip_frames(skip_count) + 1) {
        cv::Mat frame;
        *m_video_ptr >> frame;
        if (frame.empty()) {
            Log.error(i, "frame is empty");
            callback(AsstMsg::SubTaskError, basic_info_with_what("OcrFormation"));
            return false;
        }

        cv::resize(frame, frame, cv::Size(), m_scale, m_scale, cv::INTER_AREA);

        formation_ananlyzer.set_image(frame);
        auto formation_opt = formation_ananlyzer.analyze();
        show_img(formation_ananlyzer);
        // 有些视频会有个过渡或者动画啥的，只取一帧识别的可能不全。多识别几帧
        if (formation_opt) {
            if (formation_opt->size() > m_formation.size()) {
                for (const auto& [name, avatar] : *formation_opt) {
                    m_formation.insert_or_assign(name, avatar);
                }
            }
            else if (++no_changes_count > 5) {
                m_formation_end_frame = i;
                break;
            }
        }
        else if (!m_formation.empty()) {
            m_formation_end_frame = i;
            break;
        }
    }

    Log.info("Formation:", m_formation | views::keys);
    auto cb_info = basic_info_with_what("OcrFormation");
    auto& cb_formation = cb_info["details"]["formation"];
    for (const auto& [name, avatar] : m_formation) {
        std::vector<battle::OperUsage> opers;
        opers.emplace_back(battle::OperUsage { name, 0, battle::SkillUsage::NotUse });
        json::object oper_json { { "name", name }, { "skill", 0 }, { "skill_usage", 0 } };
        m_copilot_json["opers"].emplace(std::move(oper_json));

        cb_formation.emplace(name);
        asst::imwrite(utils::path("debug/video_export/formation/") / utils::path(name + ".png"), avatar);
    }
    callback(AsstMsg::SubTaskCompleted, cb_info);

    return true;
}

bool asst::CombatRecordRecognitionTask::analyze_stage()
{
    LogTraceFunction;

    callback(AsstMsg::SubTaskStart, basic_info_with_what("OcrStage"));

    const auto stage_name_task_ptr = Task.get("BattleStageName");
    const int skip_count = m_video_fps > m_stage_ocr_fps ? static_cast<int>(m_video_fps / m_stage_ocr_fps) - 1 : 0;

    for (size_t i = m_formation_end_frame; i < m_video_frame_count; i += skip_frames(skip_count) + 1) {
        cv::Mat frame;
        *m_video_ptr >> frame;
        if (frame.empty()) {
            Log.error(i, "frame is empty");
            callback(AsstMsg::SubTaskError, basic_info_with_what("OcrStage"));
            return false;
        }

        cv::resize(frame, frame, cv::Size(), m_scale, m_scale, cv::INTER_AREA);

        RegionOCRer stage_analyzer(frame);
        stage_analyzer.set_task_info(stage_name_task_ptr);
        bool analyzed = stage_analyzer.analyze().has_value();
        show_img(stage_analyzer);

        if (!analyzed) {
            // BattlefieldMatcher battle_analyzer(frame);
            // if (battle_analyzer.analyze()) {
            //     Log.error(i, "already start button, but still failed to analyze stage name");
            //     m_stage_ocr_end_frame = i;
            //     callback(AsstMsg::SubTaskError, basic_info_with_what("OcrStage"));
            //     return false;
            // }
            continue;
        }
        const std::string& text = stage_analyzer.get_result().text;

        if (text.empty() || !Tile.find(text)) {
            continue;
        }

        m_stage_name = text;
        m_stage_ocr_end_frame = i;
        break;
    }

    Log.info("Stage", m_stage_name);
    if (m_stage_name.empty() || !Tile.find(m_stage_name)) {
        callback(AsstMsg::SubTaskError, basic_info_with_what("OcrStage"));
        return false;
    }
    auto calc_result = Tile.calc(m_stage_name);
    m_normal_tile_info = std::move(calc_result.normal_tile_info);

    m_copilot_json["stage_name"] = m_stage_name;
    m_copilot_json["minimum_required"] = "v4.0.0";
    m_copilot_json["doc"]["title"] = "MAA AI - " + m_stage_name;
    m_copilot_json["doc"]["details"] =
        "Built at: " + utils::get_format_time() + "\n" + utils::path_to_utf8_string(m_video_path);

    callback(AsstMsg::SubTaskCompleted, basic_info_with_what("OcrStage"));
    return true;
}

bool asst::CombatRecordRecognitionTask::analyze_deployment()
{
    LogTraceFunction;
    callback(AsstMsg::SubTaskStart, basic_info_with_what("MatchDeployment"));

    const int skip_count = m_video_fps > m_deployment_fps ? static_cast<int>(m_video_fps / m_deployment_fps) - 1 : 0;

    BattlefieldMatcher oper_analyzer;
    oper_analyzer.set_object_of_interest({ .deployment = true });

    std::vector<battle::DeploymentOper> deployment;
    for (size_t i = m_stage_ocr_end_frame; i < m_video_frame_count; i += skip_frames(skip_count) + 1) {
        cv::Mat frame;
        *m_video_ptr >> frame;
        if (frame.empty()) {
            Log.error(i, "frame is empty");
            callback(AsstMsg::SubTaskError, basic_info_with_what("MatchDeployment"));
            return false;
        }

        cv::resize(frame, frame, cv::Size(), m_scale, m_scale, cv::INTER_AREA);

        oper_analyzer.set_image(frame);
        auto oper_result_opt = oper_analyzer.analyze();
        bool analyzed = oper_result_opt && oper_result_opt->pause_button;
        show_img(oper_analyzer);
        if (analyzed) {
            m_battle_start_frame = i;
            deployment = std::move(oper_result_opt->deployment);
            break;
        }
    }

    auto avatar_task_ptr = Task.get("BattleAvatarDataForFormation");
    for (const auto& [name, formation_avatar] : m_formation) {
        BestMatcher best_match_analyzer(formation_avatar);
        best_match_analyzer.set_task_info(avatar_task_ptr);

        std::unordered_set<battle::Role> roles = { BattleData.get_role(name) };
        if (name == "阿米娅") {
            roles.emplace(battle::Role::Warrior);
        }

        // 编队界面，有些视频会有些花里胡哨的特效遮挡啥的，所以尽量减小点模板尺寸
        auto crop_roi = make_rect<cv::Rect>(avatar_task_ptr->rect_move);
        // 小车的缩放太离谱了
        const size_t scale_ends = BattleData.get_rarity(name) == 1 ? 200 : 125;
        std::unordered_map<std::string, cv::Mat> candidate;
        for (const auto& oper : deployment) {
            if (!roles.contains(oper.role)) {
                continue;
            }
            cv::Mat crop_avatar = oper.avatar(crop_roi);
            // 从编队到待部署区，每个干员的缩放大小都不一样，暴力跑一遍
            // TODO: 不知道gamedata里有没有这个缩放数据，直接去拿
            for (size_t i = 100; i < scale_ends; ++i) {
                double avatar_scale = i / 100.0;
                const auto resize_method = avatar_scale < 1.0 ? cv::INTER_AREA : cv::INTER_LINEAR;
                cv::Mat resized_avatar;
                cv::resize(crop_avatar, resized_avatar, cv::Size(), avatar_scale, avatar_scale, resize_method);
                std::string flag = name + "|" + std::to_string(oper.index) + "|" + std::to_string(i);
                best_match_analyzer.append_templ(flag, resized_avatar);
                candidate.emplace(flag, oper.avatar);
            }
        }
        bool analyzed = best_match_analyzer.analyze().has_value();
        // show_img(best_match_analyzer);
        if (!analyzed) {
            Log.warn(m_battle_start_frame, "failed to match", name);
            continue;
        }
        m_all_avatars.emplace(name, candidate.at(best_match_analyzer.get_result().templ_info.name));
    }
    callback(AsstMsg::SubTaskCompleted, basic_info_with_what("MatchDeployment"));

    return !m_all_avatars.empty();
}

bool asst::CombatRecordRecognitionTask::slice_video()
{
    LogTraceFunction;
    callback(AsstMsg::SubTaskStart, basic_info_with_what("Slice"));

    const int skip_count = m_video_fps > m_deployment_fps ? static_cast<int>(m_video_fps / m_deployment_fps) - 1 : 0;

    int not_in_battle_count = 0;
    bool in_segment = false;

    cv::Mat frame;
    cv::Mat pre_frame;

    size_t i = m_battle_start_frame;
    size_t ends = m_video_frame_count - skip_count - 10;

    int latest_kills = -1;
    int total_kills = -1;

    auto battle_over = [&]() {
        if (m_clips.empty()) {
            return;
        }
        if (m_battle_end_frame == 0) {
            m_battle_end_frame = i;
        }
        if (!in_segment) {
            return;
        }
        auto& pre_clip = m_clips.back();
        pre_clip.end_frame_index = i - skip_count;
        pre_clip.end_frame = pre_frame;
        in_segment = false;
    };
    for (; i < ends; i += skip_frames(skip_count) + 1, pre_frame = frame) {
        cv::Mat temp;
        // 这里如果不用temp，frame的所有权会出错
        *m_video_ptr >> temp;
        frame = temp;
        if (frame.empty()) {
            Log.warn(i, "frame is empty");
            battle_over();
            break;
        }
        cv::resize(frame, frame, cv::Size(), m_scale, m_scale, cv::INTER_AREA);

        BattlefieldMatcher analyzer(frame);
        analyzer.set_object_of_interest({
            .deployment = true,
            .kills = true,
            .speed_button = true,
        });

        analyzer.set_total_kills_prompt(total_kills);
        auto result_opt = analyzer.analyze();
        show_img(analyzer);

        if (!result_opt) {
            battle_over();
            if (++not_in_battle_count > 10) {
                break;
            }
            continue;
        }
        m_battle_end_frame = 0;
        not_in_battle_count = 0;

        if (result_opt->kills) {
            auto& [cur_kills, cur_total_kills] = *result_opt->kills;
            if (cur_kills != latest_kills) {
                m_frame_kills.emplace_back(std::make_pair(i, cur_kills));
            }
            latest_kills = cur_kills;
            total_kills = cur_total_kills;
        }

        const auto& cur_opers = result_opt->deployment;
        bool continuity = true;
        int pre_distance = 0;
        for (auto iter = cur_opers.begin(); iter != cur_opers.end(); ++iter) {
            if (iter == cur_opers.begin()) {
                continue;
            }
            int distance = std::abs(iter->rect.x - (iter - 1)->rect.x);
            if (pre_distance && std::abs(distance - pre_distance) > 5) {
                continuity = false;
            }
            pre_distance = distance;
        }

        bool oper_is_clicked = !result_opt->speed_button || !result_opt->pause_button;
        bool oper_auto_retreat =
            in_segment && continuity && !m_clips.empty() && cur_opers.size() != m_clips.back().deployment.size();

        if (oper_is_clicked || oper_auto_retreat) {
            if (m_clips.empty()) {
                continue;
            }
            auto& pre_clip = m_clips.back();
            if (pre_clip.ends_oper_name.empty()) {
                pre_clip.ends_oper_name = analyze_detail_page_oper_name(frame);
            }
            if (!in_segment) {
                continue;
            }
            pre_clip.end_frame_index = i - skip_count;
            pre_clip.end_frame = pre_frame;
            in_segment = false;

            continue;
        }
        else if (!continuity) {
            Log.warn(i, "opers is not continuity");
            continue;
        }
        else if (!in_segment) {
            ClipInfo info;
            info.start_frame_index = i; // 后处理会加个 offset
            info.end_frame_index = i;
            info.deployment = cur_opers;
            info.start_frame = frame;
            m_clips.emplace_back(std::move(info));

            in_segment = true;
        }
    }
    battle_over();

    if (m_clips.empty()) {
        callback(AsstMsg::SubTaskError, basic_info_with_what("Slice"));
        return false;
    }

    // post process clips
    constexpr int offset_ms = 300;
    const int offset_frame = static_cast<int>(offset_ms * m_video_fps / 1000.0);

    for (auto iter = m_clips.begin(); iter != m_clips.end();) {
        ClipInfo& clip = *iter;
        if (clip.end_frame_index <= clip.start_frame_index) {
            Log.warn(__FUNCTION__, "deployment has no changes or frame error", clip.start_frame_index,
                     clip.end_frame_index);
            iter = m_clips.erase(iter);
            continue;
        }

        size_t new_start = clip.start_frame_index + offset_frame;
        if (new_start < clip.end_frame_index) {
            clip.start_frame_index = new_start;
        }

        if (iter == m_clips.begin()) {
            ++iter;
            continue;
        }

        ClipInfo& pre_clip = *(iter - 1);
        bool deployment_changed = false;
        if (iter != m_clips.begin() && clip.deployment.size() == pre_clip.deployment.size()) {
            for (size_t j = 0; j < clip.deployment.size(); ++j) {
                deployment_changed |= clip.deployment[j].role != pre_clip.deployment[j].role;
            }
        }
        else {
            deployment_changed = true;
        }
        iter->deployment_changed = deployment_changed;
        ++iter;
    }
    m_video_ptr->set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(m_battle_start_frame));

    callback(AsstMsg::SubTaskCompleted, basic_info_with_what("Slice"));
    return true;
}

bool asst::CombatRecordRecognitionTask::analyze_clip(ClipInfo& clip, ClipInfo* pre_clip_ptr)
{
    LogTraceFunction;

    if (!detect_operators(clip, pre_clip_ptr)) {
        return false;
    }
    if (!classify_direction(clip, pre_clip_ptr)) {
        return false;
    }
    if (!process_changes(clip, pre_clip_ptr)) {
        return false;
    }

    return true;
}

bool asst::CombatRecordRecognitionTask::compare_skill(ClipInfo& clip, ClipInfo& pre_clip)
{
    LogTraceFunction;

    callback(AsstMsg::SubTaskStart, basic_info_with_what("CompSkill"));

    const std::string oper_name = pre_clip.ends_oper_name;
    const Point target_location = m_operator_locations[oper_name];
    const Point target_position = m_normal_tile_info[target_location].pos;
    BattlefieldClassifier analyzer(pre_clip.end_frame);
    analyzer.set_object_of_interest({ .skill_ready = true });
    analyzer.set_base_point(target_position);
    bool pre_ready = analyzer.analyze()->skill_ready.ready;
    show_img(analyzer);

    if (!pre_ready) {
        // TODO: 有可能是点开之后等着技能转好，这种情况比较难处理
        return true;
    }

    // 有些干员明明点完技能了，但图标还会持续几百毫秒才消失
    // 所以这里要跳过一段时间
    constexpr int skip_ms = 500;
    size_t cls_begin = clip.start_frame_index + static_cast<size_t>(skip_ms * m_video_fps / 1000.0);
    if (cls_begin > clip.end_frame_index) {
        Log.warn("skip too much");
        cls_begin = clip.start_frame_index + (clip.end_frame_index - clip.start_frame_index) / 2;
    }
    const size_t begin_skip = cls_begin - static_cast<size_t>(m_video_ptr->get(cv::CAP_PROP_POS_FRAMES));
    skip_frames(begin_skip);

    cv::Mat frame;
    *m_video_ptr >> frame;
    if (frame.empty()) {
        Log.error("frame is empty");
        callback(AsstMsg::SubTaskError, basic_info_with_what("CompSkill"));
        return false;
    }
    cv::resize(frame, frame, cv::Size(), m_scale, m_scale, cv::INTER_AREA);
    analyzer.set_image(frame);
    bool cur_ready = analyzer.analyze()->skill_ready.ready;

    if (pre_ready && !cur_ready) {
        json::object condition = analyze_action_condition(clip, &pre_clip);
        json::object skill_json = condition | json::object {
            { "type", "Skill" },
            { "location", json::array { target_location.x, target_location.y } },
            { "name", oper_name },
        };
        Log.info("skill json", skill_json.to_string());
        auto& actions_json = m_copilot_json["actions"].as_array();
        actions_json.emplace_back(std::move(skill_json));
    }

    callback(AsstMsg::SubTaskCompleted, basic_info_with_what("CompSkill"));
    return true;
}

bool asst::CombatRecordRecognitionTask::detect_operators(ClipInfo& clip, [[maybe_unused]] ClipInfo* pre_clip_ptr)
{
    LogTraceFunction;

    callback(AsstMsg::SubTaskStart, basic_info_with_what("DetectOperators"));

    const size_t frame_count = clip.end_frame_index - clip.start_frame_index;

    /* detect operators on the battefield */
    using DetectionResult = std::unordered_set<Point>;
    std::unordered_map<DetectionResult, size_t, ContainerHasher<DetectionResult>> oper_det_samping;
    const Rect det_box_move = Task.get("BattleOperBoxRectMove")->rect_move;

    constexpr size_t OperDetSamplingCount = 20;
    const size_t skip_count =
        frame_count > (OperDetSamplingCount + 1) ? frame_count / (OperDetSamplingCount + 1) - 1 : 0;

    const size_t det_begin = clip.start_frame_index + skip_count;
    const size_t det_end = clip.end_frame_index - skip_count;

    const size_t begin_skip = det_begin - static_cast<size_t>(m_video_ptr->get(cv::CAP_PROP_POS_FRAMES));
    skip_frames(begin_skip);

    for (size_t i = det_begin; i <= det_end; i += skip_frames(skip_count) + 1) {
        cv::Mat frame;
        *m_video_ptr >> frame;
        if (frame.empty()) {
            Log.error(i, "frame is empty");
            callback(AsstMsg::SubTaskError, basic_info_with_what("DetectOperators"));
            return false;
        }

        cv::resize(frame, frame, cv::Size(), m_scale, m_scale, cv::INTER_AREA);
        BattlefieldDetector analyzer(frame);
        analyzer.set_object_of_interest({ .operators = true });
        auto result_opt = analyzer.analyze();
        show_img(analyzer);

        DetectionResult cur_locations;
        auto tiles = m_normal_tile_info | views::values;
        for (const auto& box : result_opt->operators) {
            Rect rect = box.rect.move(det_box_move);
            auto iter = ranges::find_if(tiles, [&](const TilePack::TileInfo& t) { return rect.include(t.pos); });
            if (iter == tiles.end()) {
                Log.warn(i, __FUNCTION__, "no pos", box.rect.to_string(), rect);
                continue;
            }
            cur_locations.emplace((*iter).loc);
        }
        oper_det_samping[std::move(cur_locations)] += 1;

        clip.random_frames.emplace_back(frame); // for classify_direction
    }

    /* 取众数 */
    auto oper_det_iter = ranges::max_element(oper_det_samping,
                                             [&](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });
    if (oper_det_iter == oper_det_samping.end()) {
        Log.error(__FUNCTION__, "oper_det_samping is empty");
        callback(AsstMsg::SubTaskError, basic_info_with_what("DetectOperators"));
        return false;
    }

    for (const Point& loc : oper_det_iter->first) {
        clip.battlefield.emplace(loc, BattlefieldOper {});
    }

    callback(AsstMsg::SubTaskCompleted, basic_info_with_what("DetectOperators"));
    return true;
}

bool asst::CombatRecordRecognitionTask::classify_direction(ClipInfo& clip, ClipInfo* pre_clip_ptr)
{
    LogTraceFunction;

    if (!pre_clip_ptr) {
        Log.info("first clip, skip");
        callback(AsstMsg::SubTaskCompleted, basic_info_with_what("ClassifyDirection"));
        return true;
    }

    std::vector<Point> newcomer;
    for (const Point& loc : clip.battlefield | views::keys) {
        if (pre_clip_ptr->battlefield.contains(loc)) {
            continue;
        }
        newcomer.emplace_back(loc);
    }
    if (newcomer.empty()) {
        return true;
    }
    callback(AsstMsg::SubTaskStart, basic_info_with_what("ClassifyDirection"));

    /* classify direction */
    using Raw = BattlefieldClassifier::DeployDirectionResult::Raw;
    constexpr size_t ClsSize = BattlefieldClassifier::DeployDirectionResult::ClsSize;
    std::unordered_map<Point, Raw> dir_cls_sampling;

    for (const cv::Mat& frame : clip.random_frames) {
        BattlefieldClassifier analyzer(frame);
        analyzer.set_object_of_interest({ .skill_ready = false, .deploy_direction = true });
        for (const auto& loc : newcomer) {
            analyzer.set_base_point(m_normal_tile_info.at(loc).pos);
            auto result_opt = analyzer.analyze();
            show_img(analyzer);
            for (size_t i = 0; i < ClsSize; ++i) {
                dir_cls_sampling[loc][i] += result_opt->deploy_direction.raw[i];
            }
        }
    }

    for (const auto& [loc, sampling] : dir_cls_sampling) {
        auto class_id = std::max_element(sampling.begin(), sampling.end()) - sampling.begin();

        clip.battlefield[loc].direction = static_cast<battle::DeployDirection>(class_id);
        clip.battlefield[loc].new_here = true;
    }
    callback(AsstMsg::SubTaskCompleted, basic_info_with_what("ClassifyDirection"));
    return true;
}

bool asst::CombatRecordRecognitionTask::process_changes(ClipInfo& clip, ClipInfo* pre_clip_ptr)
{
    LogTraceFunction;
    std::ignore = m_video_ptr;
    std::ignore = clip;

    if (!pre_clip_ptr) {
        Log.info("first clip, skip");
        return true;
    }

    json::object condition = analyze_action_condition(clip, pre_clip_ptr);

    auto& actions_json = m_copilot_json["actions"].as_array();

    if (pre_clip_ptr->deployment.size() > clip.deployment.size() ||
        pre_clip_ptr->battlefield.size() < clip.battlefield.size()) {
        // 部署
        std::vector<std::string> deployed;
        ananlyze_deployment_names(clip);
        ananlyze_deployment_names(*pre_clip_ptr);
        for (const auto& pre_oper : pre_clip_ptr->deployment) {
            auto iter = ranges::find_if(clip.deployment, [&](const auto& oper) { return oper.name == pre_oper.name; });
            if (iter != clip.deployment.end()) {
                continue;
            }
            deployed.emplace_back(pre_oper.name);
        }
        Log.info("deployed", deployed);

        if (deployed.empty()) {
            Log.warn("Unknown dployed, will use pre tails_page's name", pre_clip_ptr->ends_oper_name);
        }

        auto deployed_iter = deployed.begin();
        for (const auto& [loc, oper] : clip.battlefield) {
            if (!oper.new_here) {
                continue;
            }
            std::string name = deployed_iter == deployed.end() ? pre_clip_ptr->ends_oper_name : *(deployed_iter++);
            if (name.empty()) {
                name = "Unknown_EndsEmpty";
            }

            static const std::unordered_map<battle::DeployDirection, std::string> DirectionNames = {
                { battle::DeployDirection::Right, "Right" }, { battle::DeployDirection::Down, "Down" },
                { battle::DeployDirection::Left, "Left" },   { battle::DeployDirection::Up, "Up" },
                { battle::DeployDirection::None, "None" },
            };
            const std::string& direction = DirectionNames.at(oper.direction);

            json::object deploy_json = condition | json::object {
                { "type", "Deploy" },
                { "name", name }, // 这里正常应该只有一个人，多了就只能抽奖了（
                { "location", json::array { loc.x, loc.y } },
                { "direction", direction },
            };
            Log.info("deploy json", deploy_json.to_string());
            actions_json.emplace_back(std::move(deploy_json));

            m_operator_locations.insert_or_assign(name, loc);
            m_location_operators.insert_or_assign(loc, name);
        }
    }
    else if (pre_clip_ptr->deployment.size() < clip.deployment.size() ||
             pre_clip_ptr->battlefield.size() > clip.battlefield.size()) {
        // 撤退
        for (const auto& [pre_loc, pre_oper] : pre_clip_ptr->battlefield) {
            if (clip.battlefield.contains(pre_loc)) {
                continue;
            }

            std::string name = m_location_operators[pre_loc];
            json::object retreat_json = condition | json::object {
                { "type", "Retreat" },
                { "location", json::array { pre_loc.x, pre_loc.y } },
                { "name", name },
            };
            Log.info("retreat json", retreat_json.to_string());
            actions_json.emplace_back(std::move(retreat_json));

            m_location_operators.erase(pre_loc);
            m_operator_locations.erase(name);
        }
    }
    else {
        Log.warn("Unknown changes, deployment:", pre_clip_ptr->deployment.size(), clip.deployment.size(),
                 "battlefield:", pre_clip_ptr->battlefield.size(), clip.battlefield.size());
    }

    return true;
}

void asst::CombatRecordRecognitionTask::ananlyze_deployment_names(ClipInfo& clip)
{
    LogTraceFunction;

    for (auto& oper : clip.deployment) {
        if (!oper.name.empty()) {
            continue;
        }
        BestMatcher avatar_analyzer(oper.avatar);
        static const double threshold = Task.get<MatchTaskInfo>("BattleAvatarDataForVideo")->templ_thresholds.front();
        avatar_analyzer.set_method(MatchMethod::Ccoeff);
        avatar_analyzer.set_threshold(threshold);
        // static const double drone_threshold = Task.get<MatchTaskInfo>("BattleDroneAvatarData")->templ_threshold;
        // avatar_analyzer.set_threshold(oper.role == battle::Role::Drone ? drone_threshold : threshold);

        for (const auto& [name, avatar] : m_all_avatars) {
            std::unordered_set<battle::Role> roles = { BattleData.get_role(name) };
            if (name == "阿米娅") {
                roles.emplace(battle::Role::Warrior);
            }
            if (roles.contains(oper.role)) {
                avatar_analyzer.append_templ(name, avatar);
            }
        }
        bool analyzed = avatar_analyzer.analyze().has_value();
        // show_img(avatar_analyzer.get_draw());
        if (analyzed) {
            oper.name = avatar_analyzer.get_result().templ_info.name;
        }
        else {
            oper.name = "UnknownDeployment";
        }
    }
}

json::object asst::CombatRecordRecognitionTask::analyze_action_condition(ClipInfo& clip, ClipInfo* pre_clip_ptr)
{
    LogTraceFunction;

    if (m_frame_kills.empty()) {
        return {};
    }
    size_t target_frame_index = pre_clip_ptr->end_frame_index;
    size_t kill_frame_index = 0;
    int kills = 0;
    for (auto iter = m_frame_kills.begin() + 1; iter != m_frame_kills.end(); ++iter) {
        const auto& [cur_index, _] = *iter;
        if (cur_index < target_frame_index) {
            continue;
        }
        std::tie(kill_frame_index, kills) = *(iter - 1);
        break;
    }
    if (!kill_frame_index) {
        return {};
    }

    json::object condition {
        { "kills", kills },
    };

    BattlefieldMatcher analyzer(pre_clip_ptr->end_frame); // 开始执行这次操作的画面
    analyzer.set_object_of_interest({ .costs = true });
    auto end_result_opt = analyzer.analyze();
    if (!end_result_opt || !end_result_opt->costs) {
        m_pre_action_costs = -1;
        return condition;
    }
    int start_costs = end_result_opt->costs.value();
    int cost_changes = start_costs - m_pre_action_costs;
    if (m_pre_action_costs >= 0 && cost_changes > 0) {
        condition.emplace("cost_changes", cost_changes);
    }

    analyzer.set_image(clip.start_frame); // 这次操作执行完了的画面
    auto start_result_opt = analyzer.analyze();
    m_pre_action_costs = (start_result_opt && start_result_opt->costs) ? start_result_opt->costs.value() : start_costs;

    return condition;
}

size_t asst::CombatRecordRecognitionTask::skip_frames(size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        cv::Mat ignore;
        *m_video_ptr >> ignore;
    }
    return count;
}

std::string asst::CombatRecordRecognitionTask::analyze_detail_page_oper_name(const cv::Mat& frame)
{
    const auto& replace_task = Task.get<OcrTaskInfo>("CharsNameOcrReplace");

    RegionOCRer preproc_analyzer(frame);
    preproc_analyzer.set_task_info("BattleOperName");
    preproc_analyzer.set_replace(replace_task->replace_map, replace_task->replace_full);
    auto preproc_result_opt = preproc_analyzer.analyze();

    if (preproc_result_opt && !BattleData.is_name_invalid(preproc_result_opt->text)) {
        return preproc_result_opt->text;
    }

    Log.warn("ocr with preprocess got a invalid name, try to use detect model");
    OCRer det_analyzer(frame);
    det_analyzer.set_task_info("BattleOperName");
    det_analyzer.set_replace(replace_task->replace_map, replace_task->replace_full);
    auto det_result_opt = det_analyzer.analyze();
    if (!det_result_opt) {
        return {};
    }
    sort_by_score_(*det_result_opt);
    const auto& det_name = det_result_opt->front().text;

    return BattleData.is_name_invalid(det_name) ? std::string() : det_name;
}

void asst::CombatRecordRecognitionTask::show_img(const asst::VisionHelper& analyzer)
{
#ifdef ASST_DEBUG
    show_img(analyzer.get_draw());
#else
    std::ignore = analyzer;
#endif
}

void asst::CombatRecordRecognitionTask::show_img(const cv::Mat& img)
{
#ifdef ASST_DEBUG
    cv::imshow(DrawWindow, img);
    cv::waitKey(1);
#else
    std::ignore = img;
#endif
}
