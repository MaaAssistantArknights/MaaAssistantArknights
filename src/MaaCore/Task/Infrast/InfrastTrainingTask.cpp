#include "InfrastTrainingTask.h"

#include <algorithm>
#include <ranges>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/OperProgress/OperProgressFormation.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/BestMatcher.h"
#include "Vision/FeatureMatcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

bool asst::InfrastTrainingTask::_run()
{
    m_all_available_opers.clear();

    set_product("SkillLevel");

    if (m_is_custom && current_room_config().skip) {
        LogInfo << "skip this room";
        return true;
    }

    swipe_to_the_left_of_main_ui();

    if (!enter_facility()) {
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
    RegionOCRer analyzer(image);
    analyzer.set_task_info("InfrastTrainingTime");
    analyzer.set_use_raw(true);
    if (!analyzer.analyze()) {
        return std::nullopt;
    }
    const auto& text = analyzer.get_result().text;
    if (text.empty() || text.find(":") == std::string::npos) {
        Log.error(__FUNCTION__, "time left analyze failed");
        return std::nullopt;
    }
    return text;
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

    auto status = analyze_status();
    if (!status) {
        return false;
    }

    // 协助者是增益项，选择失败只告警，不阻断训练室处理
    const auto change_assistant = [this]() {
        if (!select_assistant()) {
            LogWarn << "assistant selection failed";
        }
    };

    if (*status == TrainingStatus::Processing) {
        // 专精训练期间训练位干员锁定，只按配置更换协助者
        LogInfo << "training in progress, only change the assistant";
        change_assistant();
        return true;
    }

    if (m_continue_training && *status == TrainingStatus::Completed && m_level != 3) {
        // Completed 的领取点击已由 analyze_status 内的 training_completed 完成，训练位上仍是刚练完
        // 的干员。优先沿用默认模式的继续训练：主界面截取的技能图标在专精页匹配出槽位，接着练同一
        // 技能的下一阶段。缺料等点不动时自然回落到按配置换人
        click_bottom_left_tab();
        FeatureMatcher choose_skill_analyzer(ctrler()->get_image());
        choose_skill_analyzer.set_task_info("InfrastTrainingChooseSkillRec");
        choose_skill_analyzer.set_templ(m_skill_img);
        const bool continued = choose_skill_analyzer.analyze() &&
                               continue_train(skill_index_from_rect(choose_skill_analyzer.get_result().front().rect));
        if (continued) {
            // 继续专精开训后训练位同样锁定
            change_assistant();
            return true;
        }
        LogInfo << "continue training not available, fall back to custom shift";
    }

    // 空闲或未开启继续训练：按配置进驻训练位干员，只换人不开新训，与默认模式的空闲行为一致
    change_trainee();
    change_assistant();
    return true;
}

void asst::InfrastTrainingTask::change_trainee()
{
    LogTraceFunction;

    if (current_room_config().trainee_names.empty()) {
        LogInfo << "no trainee configured";
        return;
    }

    // 无论训练位当前有没有干员都点击进入选择
    ProcessTask(*this, { "InfrastTrainingSelectTrainee" }).run();

    // 训练位选择面板与快速编队同一套 UI：展开职业栏后回到 ｢全部｣，不做职业筛选，从第一页按列表顺序扫描
    if (ProcessTask(*this, { "BattleQuickFormationExpandRole" }).set_retry_times(3).run()) {
        sleep(500); // 等待职业栏展开动画结束
    }
    ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" }).set_retry_times(0).run();

    // trainee_names 是候选池：按选择列表顺序取第一个出现的干员（专精全满的不在列表中，等效跳过）
    const auto& trainee_names = current_room_config().trainee_names;
    const int delay = Task.get("BattleQuickFormationOCR")->post_delay;
    std::string last_oper_name;
    int unchanged_pages = 0;
    bool found = false;
    while (!need_exit()) {
        const auto opers_result = oper_progress::analyze_formation_opers(ctrler()->get_image());
        // 页面有效 = 能识别到干员，且末位干员与上一页不同（相同说明列表已滑到底未移动）
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
        ProcessTask(*this, { "BattleFormationOperListSlowlySwipeToTheRight" }).run();
    }

    // 复位 ｢全部｣ 并收起职业栏，确认选择回到训练室设施页；未命中时空选确认同样用于关闭面板
    ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" }).set_retry_times(0).run();
    ProcessTask(*this, { "InfrastCloseQuickFormationExpandRole", "Stop" }).run();
    if (!ProcessTask(*this, { "BattleQuickFormationConfirm" }).set_retry_times(0).run() && found) {
        LogWarn << "failed to confirm trainee selection";
    }

    if (!found) {
        LogWarn << "no candidate in the trainee selection list";
    }
}

bool asst::InfrastTrainingTask::select_assistant()
{
    LogTraceFunction;

    if (current_room_config().assistant_names.empty()) {
        // 未配置协助者，保持现状
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
    // 全员未命中（拼写错误、未拥有等）时复核对空名单直接放行，选出 0 人却报成功，记录日志便于排查配置
    const bool no_candidate_matched = selected && room_config.selected == 0;
    room_config = origin_config;

    if (!selected) {
        // 复核失败（如名单干员正在其他设施工作）：退出面板不确认，避免把错误选择固化
        click_return_button();
        return false;
    }
    if (no_candidate_matched) {
        LogWarn << "no candidate in the assistant selection list";
    }

    if (!ProcessTask(*this, { "InfrastDormConfirmButton" }).run() ||
        !ProcessTask(*this, { "InfrastTrainingMasteryPage" }).run()) {
        LogWarn << "failed to confirm assistant selection";
        return false;
    }
    return true;
}
