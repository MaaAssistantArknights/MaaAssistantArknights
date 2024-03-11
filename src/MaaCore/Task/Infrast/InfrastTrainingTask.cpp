#include "InfrastTrainingTask.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/BestMatcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

#include <regex>

bool asst::InfrastTrainingTask::_run()
{
    m_all_available_opers.clear();

    set_product("SkillLevel");

    swipe_to_the_right_of_main_ui();

    if (!enter_facility()) {
        return false;
    }
    enter_facility();

    if (!analyze_status()) return false;

    if (m_continue_training && m_level != 3) {
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

bool asst::InfrastTrainingTask::analyze_status()
{
    cv::Mat image = ctrler()->get_image();
    RegionOCRer idle_analyzer(image);
    idle_analyzer.set_task_info("InfrastTrainingIdle");
    if (idle_analyzer.analyze()) {
        json::value cb_info = basic_info();
        cb_info["what"] = "InfrastTrainingIdle";
        callback(AsstMsg::SubTaskExtraInfo, cb_info);
        m_continue_training = false;
        return true;
    }

    RegionOCRer rec_analyzer(image);
    rec_analyzer.set_task_info("InfrastTrainingOperatorAndSkill");
    if (!rec_analyzer.analyze()) {
        Log.error(__FUNCTION__, "recognition failed");
        return false;
    }

    std::string raw_str = rec_analyzer.get_result().text;
    size_t separation_pos = raw_str.find(']');
    if (separation_pos == std::string::npos) {
        Log.error(__FUNCTION__, "separate string failed");
        return false;
    }

    // ']'前为干员名，']'后为技能名
    // 如果没识别到'['，则说明干员名是第一个字符
    size_t starting_pos = (raw_str[0] == '[') ? 1 : 0;
    auto operator_name = raw_str.substr(starting_pos, separation_pos - 1);
    for (const auto& replace_map = Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map;
         const auto& [regex, new_str] : replace_map) {
        if (std::regex_search(operator_name, std::regex(regex))) {
            operator_name = new_str;
        }
    }
    m_operator_name = operator_name;
    m_skill_name = raw_str.substr(separation_pos + 1, raw_str.length() - separation_pos + 1);

    // TODO: 根据角色职业增加换班功能
    // m_operator_role = BattleData.get_role(m_operator_name);

    if (!level_analyze(image)) {
        Log.error(__FUNCTION__, "analyze level failed");
        return false;
    }

    if (training_completed()) {
        json::value cb_info = basic_info();
        cb_info["what"] = "InfrastTrainingCompleted";
        cb_info["details"] = json::object {
            { "operator", m_operator_name },
            { "skill", m_skill_name },
            { "level", m_level },
        };
        callback(AsstMsg::SubTaskExtraInfo, cb_info);
        return true;
    }

    m_continue_training = false;

    if (!time_left_analyze(image)) return false;

    {
        json::value cb_info = basic_info();
        cb_info["what"] = "InfrastTrainingTimeLeft";
        cb_info["details"] = json::object {
            { "operator", m_operator_name }, { "skill", m_skill_name }, { "level", m_level },
            { "hh", time_left[0] },          { "mm", time_left[1] },    { "ss", time_left[2] },
        };
        callback(AsstMsg::SubTaskExtraInfo, cb_info);
    }

    return true;
}

bool asst::InfrastTrainingTask::level_analyze(cv::Mat image)
{
    const std::string task_name = "InfrastTrainingLevel";

    BestMatcher analyzer(image);
    analyzer.set_task_info(task_name);
    for (int i = 1; i <= 3; ++i) {
        std::string level_temp_name = task_name + std::to_string(i) + ".png";
        analyzer.append_templ(level_temp_name);
    }
    if (!analyzer.analyze()) return false;
    const auto& res = analyzer.get_result();
    utils::chars_to_number(res.templ_info.name.substr(task_name.size(), 1), m_level);
    Log.info(__FUNCTION__, "level has been set to ", m_level);

    return true;
}

bool asst::InfrastTrainingTask::training_completed()
{
    ProcessTask task(*this, { "InfrastTrainingCompleted" });
    return task.run();
}

bool asst::InfrastTrainingTask::time_left_analyze(cv::Mat image)
{
    LogTraceFunction;

    RegionOCRer progress_analyzer(image);
    std::regex re(R"(\d+)");
    std::smatch match;

    for (int i = 0; i < 3; ++i) {
        progress_analyzer.set_task_info("InfrastTrainingTimeRec" + std::to_string(i));
        if (!progress_analyzer.analyze()) return false;

        std::string raw_str = progress_analyzer.get_result().text;
        Log.info(__FUNCTION__, raw_str);
        if (!std::regex_search(raw_str, match, re)) {
            Log.error(__FUNCTION__, "regex_search failed");
            return false;
        }
        std::string str_time = match.str();

        if (!utils::chars_to_number(str_time, time_left[i])) {
            Log.error(__FUNCTION__, "chars_to_number failed");
            return false;
        }
    }

    return true;
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
    if (cy <= 300) return 1;
    if (cy <= 500) return 2;
    return 3;
}
