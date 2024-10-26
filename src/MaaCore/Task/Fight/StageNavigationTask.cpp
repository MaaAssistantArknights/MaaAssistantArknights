#include "StageNavigationTask.h"

#include <regex>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

bool asst::StageNavigationTask::set_stage_name(const std::string& stage_name)
{
    LogTraceFunction;

    clear();

    // 已有关卡优先使用 tasks.json 中的逻辑
    if (Task.get(stage_name)) {
        m_is_directly = true;
        m_directly_task = stage_name;
        Log.info("directly task", m_directly_task);
        return true;
    }
    m_is_directly = false;

    static const std::regex stage_regex(R"(^([A-Za-z]{0,3})(\d{1,2})-(\d{1,2})(?:-?(\w+))*$)");
    std::smatch stage_sm;
    if (!std::regex_match(stage_name, stage_sm, stage_regex)) {
        Log.error("The stage name is not in invalid, or is not main line stage", stage_name);
        return false;
    }

    // 10-1 -> [1] "", [2] "10", [3] "1", [4] ""
    // JT8-2 -> [1] "JT", [2] "8", [3] "2", [4] ""
    // H10-1-Hard -> [1] "H", [2] "10", [3] "1", [4] "Hard"
    const std::string& stage_prefix = stage_sm[1].str();
    const std::string& chapter = stage_sm[2].str();
    const std::string& stage_index = stage_sm[3].str();
    const std::string& difficulty = stage_sm[4].str();

    static const std::string episode_task_prefix = "Episode";
    m_chapter_task = episode_task_prefix + chapter;
    Log.info("chapter task", m_chapter_task);
    if (!Task.get(m_chapter_task)) {
        Log.error("chapter task not exists", m_chapter_task);
        return false;
    }

    if (!difficulty.empty()) {
        std::string upper_difficulty = difficulty;
        upper_difficulty[0] = static_cast<char>(::toupper(upper_difficulty[0]));
        for (size_t i = 1; i < upper_difficulty.size(); ++i) {
            upper_difficulty[i] = static_cast<char>(::tolower(upper_difficulty[i]));
        }
        static const std::string difficulty_task_prefix = "ChapterDifficulty";
        m_difficulty_task = difficulty_task_prefix + upper_difficulty;
        Log.info("difficulty task", m_difficulty_task);
        if (!Task.get(m_difficulty_task)) {
            Log.error("difficulty task not exists", m_difficulty_task);
            return false;
        }
    }

    std::string upper_prefix = stage_prefix;
    ranges::transform(upper_prefix, upper_prefix.begin(), [](const char ch) -> char {
        return static_cast<char>(::toupper(ch));
    });
    m_stage_code = upper_prefix + chapter + "-" + stage_index;
    Log.info("stage code", m_stage_code);

    return true;
}

bool asst::StageNavigationTask::_run()
{
    LogTraceFunction;

    if (m_is_directly) {
        return ProcessTask(*this, { m_directly_task }).set_retry_times(RetryTimesDefault).run();
    }

    return chapter_wayfinding() && swipe_and_find_stage();
}

void asst::StageNavigationTask::clear() noexcept
{
    m_is_directly = false;
    m_directly_task.clear();
    m_chapter_task.clear();
    m_difficulty_task.clear();
    m_stage_code.clear();
}

bool asst::StageNavigationTask::chapter_wayfinding()
{
    LogTraceFunction;

    if (!ProcessTask(*this, { m_chapter_task }).set_retry_times(RetryTimesDefault).run()) {
        return false;
    }

    if (!m_difficulty_task.empty()) {
        return ProcessTask(*this, { m_difficulty_task }).set_retry_times(RetryTimesDefault).run();
    }

    return true;
}

bool asst::StageNavigationTask::swipe_and_find_stage()
{
    LogTraceFunction;

    Task.get<OcrTaskInfo>(m_stage_code + "@ClickStageName")->text = { m_stage_code };
    std::string replace_m_stage_code = m_stage_code;
    utils::string_replace_all_in_place(replace_m_stage_code, { { "-", "" } });
    Task.get<OcrTaskInfo>(m_stage_code + "@ClickedCorrectStage")->text = { m_stage_code, replace_m_stage_code };
    return ProcessTask(*this, { m_stage_code + "@StageNavigationBegin" }).set_retry_times(RetryTimesDefault).run();
}
