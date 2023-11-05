#include "InfrastTrainingTask.h"

#include "Task/ProcessTask.h"
#include "Controller/Controller.h"
#include "Vision/RegionOCRer.h"
#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"

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

    return true;
}

bool asst::InfrastTrainingTask::analyze_status()
{
    cv::Mat image = ctrler()->get_image();
    RegionOCRer idle_analyzer(image);
    idle_analyzer.set_task_info("InfrastTrainingIdle");
    if (idle_analyzer.analyze()) {
        return true;
    }

    RegionOCRer rec_analyzer(image);
    rec_analyzer.set_task_info("InfrastTrainingOperatorAndSkill");
    if (!rec_analyzer.analyze()) {
        Log.error(__FUNCTION__, "recognition failed");
        return false;
    }

    std::string raw_str = rec_analyzer.get_result().text;
    size_t separation_pos = raw_str.find("]");
    if (separation_pos == std::string::npos) {
        Log.error(__FUNCTION__, "separate string failed");
        return false;
    }

    // "]"前为干员名，"]"后为技能名
    m_operator_name = raw_str.substr(1, separation_pos - 1);
    m_skill_name = raw_str.substr(separation_pos + 1, raw_str.length() - separation_pos + 1);

    Log.info(__FUNCTION__, "operator name set as", m_operator_name);
    Log.info(__FUNCTION__, "skill name set as", m_skill_name);

    if (training_completed()) {
        json::value cb_info = basic_info();
        cb_info["what"] = "InfrastTrainingCompleted";
        cb_info["details"] = json::object {
            { "operator", m_operator_name },
            { "skill", m_skill_name },
        };
        callback(AsstMsg::SubTaskExtraInfo, cb_info);
        return true;
    }

    RegionOCRer progress_analyzer(image);
    progress_analyzer.set_task_info("InfrastTrainingProgressRec");
    if (!progress_analyzer.analyze()) {
        Log.error(__FUNCTION__, "progress recognition failed");
        return false;
    }

    raw_str = progress_analyzer.get_result().text;

    std::regex re(R"(\d+)");
    std::smatch match;
    if (!std::regex_search(raw_str, match, re)) {
        Log.error(__FUNCTION__, "regex_search failed");
        return false;
    }
    std::string str_progress = match.str();
    Log.info(__FUNCTION__, "str_progress", str_progress);

    int progress_percent = 0;
    if (!utils::chars_to_number(str_progress, progress_percent)) {
        Log.error(__FUNCTION__, "chars_to_number failed");
        return false;
    }

    {
        json::value cb_info = basic_info();
        cb_info["what"] = "InfrastTrainingInProgress";
        cb_info["details"] = json::object {
            { "operator", m_operator_name },
            { "skill", m_skill_name },
            { "progress", progress_percent },
        };
        callback(AsstMsg::SubTaskExtraInfo, cb_info);
    }

    return true;
}

bool asst::InfrastTrainingTask::training_completed()
{
    ProcessTask task(*this, { "InfrastTrainingCompleted" });
    return task.run();
}
