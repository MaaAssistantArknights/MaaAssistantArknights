#include "InfrastTrainingTask.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/BestMatcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

bool asst::InfrastTrainingTask::_run()
{
    m_all_available_opers.clear();

    set_product("SkillLevel");

    swipe_to_the_left_of_main_ui();

    if (!enter_facility()) {
        swipe_to_right_of_main_ui();
        if (!enter_facility()) {
            return false;
        }
    }

    auto status = analyze_status();
    if (!status) {
        return false;
    }

    if (m_continue_training && *status == TrainingStatus::Completed && m_level != 3) { // 继续训练
        click_bottom_left_tab();
        OCRer choose_skill_analyzer(ctrler()->get_image());
        choose_skill_analyzer.set_task_info("InfrastTrainingChooseSkillRec");
        choose_skill_analyzer.set_required({ m_skill_name });
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

    const auto& replace_map = Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map;
    std::vector<std::pair<std::string, std::string>> task_replace =
        Task.get<OcrTaskInfo>("InfrastTrainingOperatorAndSkill")->replace_map;
    ranges::copy(replace_map, std::back_inserter(task_replace));
    RegionOCRer rec_analyzer(image);
    rec_analyzer.set_task_info("InfrastTrainingOperatorAndSkill");
    rec_analyzer.set_replace(task_replace);
    rec_analyzer.set_use_raw(true);
    if (!rec_analyzer.analyze()) {
        Log.error(__FUNCTION__, "recognition failed");
        return std::nullopt;
    }

    std::string raw_str = rec_analyzer.get_result().text;
    size_t separation_pos = raw_str.find('\n');
    if (separation_pos == std::string::npos) {
        Log.error(__FUNCTION__, "separate string failed");
        return std::nullopt;
    }

    // '\n'前为干员名，'\n'后为技能名
    m_operator_name = raw_str.substr(0, separation_pos);
    m_skill_name = raw_str.substr(separation_pos + 1, raw_str.length() - separation_pos + 1);

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
    ProcessTask task(*this, { "InfrastTrainingProcessing", "InfrastTrainingCompleted" });
    return task.run() && task.get_last_task_name() == "InfrastTrainingCompleted";
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
