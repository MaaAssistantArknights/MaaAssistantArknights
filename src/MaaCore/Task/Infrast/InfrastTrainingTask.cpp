#include "InfrastTrainingTask.h"

#include <algorithm>
#include <format>
#include <ranges>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/OperProgress/OperProgressFormation.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/BestMatcher.h"
#include "Vision/FeatureMatcher.h"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

// 防御滑动上限，正常到底由 unchanged_pages 判定；须覆盖全量干员列表（数百名、数十屏），避免截断正常扫描
static constexpr int MaxOperatorPages = 100;

bool asst::InfrastTrainingTask::_run()
{
    m_all_available_opers.clear();

    set_product("SkillLevel");

    swipe_to_the_left_of_main_ui();

    if (!enter_facility(0, false)) {
        swipe_to_right_of_main_ui();
        if (!enter_facility()) {
            return false;
        }
    }

    if (m_is_custom) {
        return custom_shift();
    }

    auto status = analyze_status();
    if (!status) {
        return false;
    }

    if (m_continue_training && *status == TrainingStatus::Completed && m_level != 3) { // 继续训练
        click_bottom_left_tab();
        FeatureMatcher choose_skill_analyzer(ctrler()->get_image());
        choose_skill_analyzer.set_task_info("InfrastTrainingChooseSkillRec");
        choose_skill_analyzer.set_templ(m_skill_img);
        if (!choose_skill_analyzer.analyze()) {
            Log.error(__FUNCTION__, "choose skill failed");
            return false;
        }

        continue_train(skill_index_from_rect(choose_skill_analyzer.get_result().front().rect));
    }

    return true;
}

std::optional<asst::InfrastTrainingTask::TrainingStatus> asst::InfrastTrainingTask::analyze_status()
{
    const auto& image = ctrler()->get_image();
    RegionOCRer idle_analyzer(image);
    idle_analyzer.set_task_info("InfrastTrainingIdle");
    if (idle_analyzer.analyze()) {
        json::value cb_info = basic_info_with_what("InfrastTrainingIdle");
        callback(AsstMsg::SubTaskExtraInfo, cb_info);
        return TrainingStatus::Idle;
    }

    {
        const auto& replace_map = Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map;
        std::vector<std::pair<std::string, std::string>> task_replace =
            Task.get<OcrTaskInfo>("InfrastTrainingOperatorAndSkill")->replace_map;
        std::ranges::copy(replace_map, std::back_inserter(task_replace));
        RegionOCRer name_analyzer(image);
        name_analyzer.set_task_info("InfrastTrainingOperatorAndSkill");
        name_analyzer.set_replace(task_replace);
        name_analyzer.set_use_raw(true);
        if (!name_analyzer.analyze()) {
            Log.error(__FUNCTION__, "operator name recognition failed");
            return std::nullopt;
        }

        std::string name_str = name_analyzer.get_result().text;
        size_t separation_pos = name_str.find('\n');
        if (separation_pos == std::string::npos) {
            Log.error(__FUNCTION__, "separate string failed");
            return std::nullopt;
        }

        // '\n'前为干员名，'\n'后为技能名
        m_operator_name = name_str.substr(0, separation_pos);
    }

    {
        RegionOCRer skill_analyzer(image);
        skill_analyzer.set_task_info("InfrastTrainingOperatorAndSkill");
        skill_analyzer.set_use_raw(true);
        if (!skill_analyzer.analyze()) {
            Log.error(__FUNCTION__, "skill name recognition failed");
            return std::nullopt;
        }

        std::string skill_str = skill_analyzer.get_result().text;
        size_t separation_pos = skill_str.find('\n');
        if (separation_pos == std::string::npos) {
            Log.error(__FUNCTION__, "separate string failed");
            return std::nullopt;
        }

        m_skill_name = skill_str.substr(separation_pos + 1);
    }

    Rect roi = Task.get("InfrastTrainingSkillImg")->roi;
    m_skill_img = image(make_rect<cv::Rect>(roi));

    // TODO: 根据角色职业增加换班功能
    // m_operator_role = BattleData.get_role(m_operator_name);

    if (!level_analyze(image)) {
        Log.error(__FUNCTION__, "analyze level failed");
        return std::nullopt;
    }

    if (training_completed()) {
        json::value cb_info = basic_info_with_what("InfrastTrainingCompleted");
        cb_info["details"] = json::object {
            { "operator", m_operator_name },
            { "skill", m_skill_name },
            { "level", m_level },
        };
        callback(AsstMsg::SubTaskExtraInfo, cb_info);
        return TrainingStatus::Completed;
    }

    const auto& time_opt = time_left_analyze(image);
    if (!time_opt) {
        return std::nullopt;
    }

    json::value info = basic_info_with_what("InfrastTrainingTimeLeft");
    info["details"] = json::object {
        { "operator", m_operator_name },
        { "skill", m_skill_name },
        { "level", m_level },
        { "time", *time_opt },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);

    return TrainingStatus::Processing;
}

bool asst::InfrastTrainingTask::level_analyze(const cv::Mat& image)
{
    const std::string task_name = "InfrastTrainingLevel";

    BestMatcher analyzer(image);
    analyzer.set_task_info(task_name);
    for (int i = 1; i <= 3; ++i) {
        std::string level_temp_name = task_name + std::to_string(i) + ".png";
        analyzer.append_templ(level_temp_name);
    }
    if (!analyzer.analyze()) {
        return false;
    }
    const auto& res = analyzer.get_result();
    utils::chars_to_number(res.templ_info.name.substr(task_name.size(), 1), m_level);
    Log.info(__FUNCTION__, "level has been set to ", m_level);

    return true;
}

bool asst::InfrastTrainingTask::training_completed()
{
    return ProcessTask(*this, { "InfrastTrainingCompleted" }).run();
}

std::optional<std::string> asst::InfrastTrainingTask::time_left_analyze(const cv::Mat& image)
{
    LogTraceFunction;

    static const std::string TaskName = "InfrastTrainingTime";
    const auto task_info = Task.get<OcrTaskInfo>(TaskName);
    const auto& mask_params = task_info->special_params;
    if (mask_params.size() != 4) {
        LogError << __FUNCTION__ << "unexpected timer separator mask parameters" << mask_params.size();
        return std::nullopt;
    }

    cv::Mat timer_image = image.clone();
    const cv::Rect image_bounds(0, 0, timer_image.cols, timer_image.rows);
    for (size_t i = 0; i < mask_params.size(); i += 2) {
        const int x_offset = mask_params[i];
        const int width = mask_params[i + 1];
        if (x_offset < 0 || width <= 0 || x_offset + width > task_info->roi.width) {
            LogError << __FUNCTION__ << "invalid timer separator mask" << x_offset << width;
            return std::nullopt;
        }

        const Rect mask_rect(task_info->roi.x + x_offset, task_info->roi.y, width, task_info->roi.height);
        const cv::Rect visible_mask = make_rect<cv::Rect>(mask_rect) & image_bounds;
        if (!visible_mask.empty()) {
            timer_image(visible_mask).setTo(cv::Scalar::all(0));
        }
    }

    RegionOCRer analyzer(timer_image);
    analyzer.set_task_info(TaskName);
    analyzer.set_use_raw(true);
    if (!analyzer.analyze()) {
        return std::nullopt;
    }
    const auto& text = analyzer.get_result().text;
    if (text.size() != 6 || !std::ranges::all_of(text, [](char ch) { return ch >= '0' && ch <= '9'; })) {
        LogError << __FUNCTION__ << "timer OCR did not return six digits" << text;
        return std::nullopt;
    }

    const int minute = (text[2] - '0') * 10 + text[3] - '0';
    const int second = (text[4] - '0') * 10 + text[5] - '0';
    if (minute >= 60 || second >= 60) {
        LogError << __FUNCTION__ << "timer OCR returned an invalid time" << text;
        return std::nullopt;
    }

    return std::format("{}{}:{}{}:{}{}", text[0], text[1], text[2], text[3], text[4], text[5]);
}

asst::InfrastTrainingTask& asst::InfrastTrainingTask::set_continue_training(bool continue_training) noexcept
{
    m_continue_training = continue_training;
    return *this;
}

bool asst::InfrastTrainingTask::continue_train(int index)
{
    static const std::vector<std::string> continue_train_task = { "InfrastTrainingContinue1",
                                                                  "InfrastTrainingContinue2",
                                                                  "InfrastTrainingContinue3" };
    return ProcessTask { *this, { continue_train_task[index - 1] } }.run();
}

int asst::InfrastTrainingTask::skill_index_from_rect(const Rect& r)
{
    int cy = r.y + r.height / 2;
    if (cy <= 300) {
        return 1;
    }
    if (cy <= 500) {
        return 2;
    }
    return 3;
}

bool asst::InfrastTrainingTask::custom_shift()
{
    LogTraceFunction;

    // 任何环节失败统一返回 false，由 _run 沿 TaskRetryTimes 重跑：重跑从 analyze_status 重新识别状态，
    // 继续训练已点的房间重入时进入 Processing 分支天然跳过换训练位；协助者选择重入时重做一遍、结果幂等

    auto status = analyze_status();
    if (!status) {
        return false;
    }

    // 换班意图一次判定（KeepUnchanged 时基类内部已发通知，Skip 只记日志，本房间不再重复调用）；
    // 继续训练是独立于排班表的设施级选项，空数组 / skip / 空意图只拦换人，与会客室线索流程同构
    const bool shift_opers = check_room_shift_intent() == RoomShiftIntent::Shift;

    // 点 tab 后实测确认已到专精页：专精页上的选人入口都是固定坐标点击，未确认就点等于盲点
    const auto enter_mastery_page = [this]() {
        click_bottom_left_tab();
        if (!wait_for_mastery_page()) {
            LogWarn << "failed to enter the mastery page";
            return false;
        }
        return true;
    };

    // 协助者选择失败视为房间处理失败，交由 _run 的重跑链重试
    const auto change_assistant = [this]() {
        if (!select_assistant()) {
            LogWarn << "assistant selection failed";
            return false;
        }
        return true;
    };

    if (*status == TrainingStatus::Processing) {
        if (!shift_opers) {
            // 非换班意图：训练中无人可换，专精页与协助者都不动
            return true;
        }
        // 专精训练期间训练位干员锁定，只按配置更换协助者；
        // 设施页底部只能读状态，选人等操作都在左下角 tab 切换后的专精页上
        LogInfo << "training in progress, only change the assistant";
        if (!enter_mastery_page()) {
            return false;
        }
        return change_assistant();
    }

    if (m_continue_training && *status == TrainingStatus::Completed && m_level != 3) {
        // Completed 的领取点击已由 analyze_status 内的 training_completed 完成，训练位上仍是刚练完
        // 的干员。逻辑独立复刻自 _run 默认分支，保持默认流程不动，
        // 材料不足等原因导致点不动时，自然回落到按配置换人
        if (!enter_mastery_page()) {
            return false;
        }
        FeatureMatcher choose_skill_analyzer(ctrler()->get_image());
        choose_skill_analyzer.set_task_info("InfrastTrainingChooseSkillRec");
        choose_skill_analyzer.set_templ(m_skill_img);
        const bool continued = choose_skill_analyzer.analyze() &&
                               continue_train(skill_index_from_rect(choose_skill_analyzer.get_result().front().rect));
        if (continued) {
            // 继续专精开训后训练位同样锁定；非换班意图不换协助者
            if (!shift_opers) {
                return true;
            }
            return change_assistant();
        }
        // 点继续训练但确认点不动时界面停在材料消耗面板上，识别到面板先关闭再回落换人；
        // 识别不到（仅按钮未命中、仍在专精页）不点返回，避免误退到设施页
        Matcher confirm_analyzer(ctrler()->get_image());
        confirm_analyzer.set_task_info("InfrastTrainingConfirm");
        if (confirm_analyzer.analyze()) {
            click_return_button();
        }
        LogInfo << "continue training not available, fall back to custom shift";
    }

    if (!shift_opers) {
        // 非换班意图到此为止：继续训练已照做（或无训练可继续），人不动
        return true;
    }

    // 空闲、未开启继续训练或继续训练失败回落：按配置进驻训练位干员，只换人不开新训；
    // 是否已在专精页不做推断，统一点 tab 后实测验证
    if (!enter_mastery_page()) {
        return false;
    }
    if (!change_trainee()) {
        return false;
    }
    return change_assistant();
}

bool asst::InfrastTrainingTask::change_trainee()
{
    LogTraceFunction;

    if (current_room_config().trainee_names.empty()) {
        LogInfo << "no trainee configured";
        return true;
    }

    // trainee_names 是候选池：按选择列表顺序取第一个出现的干员（专精全满的不在列表中，等效跳过）
    const auto& trainee_names = current_room_config().trainee_names;
    const int delay = Task.get("BattleQuickFormationOCR")->post_delay;
    bool found = false;
    // 名单确定性缺失（专精全满）扫一遍即知，再试一轮只兜 OCR 偶发漏识别
    for (int attempt = 0; attempt < 2 && !found && !need_exit(); ++attempt) {
        // 无论训练位当前有没有干员都点击进入选择
        ProcessTask(*this, { "InfrastTrainingSelectTrainee" }).run();

        // 训练位选择面板与快速编队同一套 UI：展开职业栏后回到 ｢全部｣ ，不做职业筛选，从第一页按列表顺序扫描
        if (ProcessTask(*this, { "BattleQuickFormationExpandRole" }).set_retry_times(3).run()) {
            sleep(500); // 等待职业栏展开动画结束
        }
        ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" })
            .set_retry_times(0)
            .run();

        std::string last_oper_name;
        int unchanged_pages = 0;
        int swiped_pages = 0;
        while (!need_exit()) {
            const auto opers_result = oper_progress::analyze_formation_opers(ctrler()->get_image());
            // 页面有效 = 能识别到干员，且末位干员与上一页不同（相同说明列表已滑到底未移动），首轮无上一页恒视为有效
            const bool page_moved =
                !opers_result.empty() && (last_oper_name.empty() || last_oper_name != opers_result.back().name);
            if (!page_moved) {
                // 连续两轮识别无位移判定已到列表末尾；等待滑动动画结束后再认一次，排除回弹误判
                if (++unchanged_pages >= 2) {
                    break;
                }
                sleep(500);
                continue;
            }
            unchanged_pages = 0;
            last_oper_name = opers_result.back().name;

            const auto target =
                std::ranges::find_if(opers_result, [&trainee_names](const oper_progress::QuickFormationOperInfo& oper) {
                    return std::ranges::find(trainee_names, oper.name) != trainee_names.cend();
                });
            if (target != opers_result.cend()) {
                if (!target->selected) {
                    ctrler()->click(target->flag_rect);
                    sleep(delay);
                }
                found = true;
                break;
            }
            if (++swiped_pages > MaxOperatorPages) {
                LogWarn << "trainee selection swiped too many pages, give up";
                break;
            }
            ProcessTask(*this, { "BattleFormationOperListSlowlySwipeToTheRight" }).run();
        }

        // 未命中时空选确认同样用于关闭面板
        ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" })
            .set_retry_times(0)
            .run();
        ProcessTask(*this, { "InfrastCloseQuickFormationExpandRole", "Stop" }).run();
        if (!ProcessTask(*this, { "BattleQuickFormationConfirm" }).set_retry_times(0).run() && found) {
            LogWarn << "failed to confirm trainee selection";
        }

        // 每轮关闭/确认面板后实测等待回到专精页：第二轮的打开按钮与后续协助者选择都是
        // 专精页上的固定坐标点击，不等返回就点等于盲点
        if (!wait_for_mastery_page()) {
            LogWarn << "failed to return to mastery page after trainee selection";
            if (!found) {
                // 此路径提前 return 不会走到循环外的统一发送，先补发未命中回调再退出，
                // 保证两条退出路径的未命中信息都先送达
                json::value info = basic_info_with_what("CustomInfrastTraineeNotFound");
                info["details"]["candidates"] = json::array(trainee_names);
                callback(AsstMsg::SubTaskExtraInfo, info);
            }
            return false;
        }
    }

    if (!found) {
        LogWarn << "no candidate in the trainee selection list";
        json::value info = basic_info_with_what("CustomInfrastTraineeNotFound");
        info["details"]["candidates"] = json::array(trainee_names);
        callback(AsstMsg::SubTaskExtraInfo, info);
    }
    return true;
}

bool asst::InfrastTrainingTask::wait_for_mastery_page()
{
    LogTraceFunction;
    // OCR 识别 ｢协助者｣ 文字即视为当前在专精页，作为切 tab、关面板等固定坐标点击后的统一实测依据
    return ProcessTask(*this, { "InfrastTrainingMasteryPage" }).set_retry_times(10).run();
}

bool asst::InfrastTrainingTask::select_assistant()
{
    LogTraceFunction;

    if (current_room_config().assistant_names.empty()) {
        return true;
    }

    if (!ProcessTask(*this, { "OperProgress@MasterySelectTrainer" }).run()) {
        return false;
    }

    // 协助者面板与其他基建设施共用干员列表页，复用自定义选人机制：
    // 临时把协助者名单作为备选干员，选中一位即停止（max_num_of_opers 为 1）
    auto& room_config = current_room_config();
    const auto origin_config = room_config;
    room_config.names.clear();
    room_config.candidates = origin_config.assistant_names;
    room_config.selected = 0;
    const bool selected = swipe_and_select_custom_opers(false, true);
    // 全员未命中（拼写错误、未拥有等）时复核对空名单直接放行，选出 0 人仍走空选确认，
    // 发 CustomInfrastAssistantNotFound 回调与日志便于排查配置，保持现任协助者是设计行为
    const bool no_candidate_matched = selected && room_config.selected == 0;
    room_config = origin_config;

    if (!selected) {
        // 复核失败（如名单干员正在其他设施工作）：退出面板不确认，避免把错误选择固化
        click_return_button();
        return false;
    }
    if (no_candidate_matched) {
        LogWarn << "no candidate in the assistant selection list";
        json::value info = basic_info_with_what("CustomInfrastAssistantNotFound");
        info["details"]["candidates"] = json::array(origin_config.assistant_names);
        // 0 人选出的原因：名单有误未命中，或命中但被选人阶段排除（正在其他设施工作、
        // 心情低于阈值），后两者带名上报供 GUI 区分提示，避免把计划内排除误报成配置错误
        if (!m_working_excluded_candidates.empty()) {
            info["details"]["working_excluded"] = json::array(m_working_excluded_candidates);
        }
        if (!m_low_mood_excluded_candidates.empty()) {
            info["details"]["low_mood_excluded"] = json::array(m_low_mood_excluded_candidates);
        }
        callback(AsstMsg::SubTaskExtraInfo, info);
    }

    if (!ProcessTask(*this, { "InfrastTrainingAssistantConfirm" }).run()) {
        LogWarn << "failed to confirm assistant selection";
        return false;
    }
    return true;
}
