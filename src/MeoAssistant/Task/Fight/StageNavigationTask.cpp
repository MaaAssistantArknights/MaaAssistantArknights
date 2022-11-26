#include "StageNavigationTask.h"

#include <regex>

#include "Config/TaskData.h"
#include "Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OcrImageAnalyzer.h"

bool asst::StageNavigationTask::set_stage_name(const std::string& stage_name)
{
    LogTraceFunction;

    clear();

    // 已有关卡优先使用 tasks.json 中的逻辑
    if (Task.get(stage_name)) {
        m_is_directly = true;
        m_directly_task = stage_name;
        Log.error("directly task", m_directly_task);
        return true;
    }

    m_is_directly = false;
    // 10-1 -> [1] "", [2] "10", [3] "1", [4] ""
    // JT8-2 -> [1] "JT", [2] "8", [3] "2", [4] ""
    // 10-1-Hard -> [1] "", [2] "10", [3] "1", [4] "Hard"
    static const std::regex StageRegex(R"(^([A-Za-z]{0,3})(\d{1,2})-(\d{1,2})(?:-(\w+))*$)");
    std::smatch stage_sm;
    if (!std::regex_match(stage_name, stage_sm, StageRegex)) {
        Log.error("Unknown stage", stage_name);
        return false;
    }
    const std::string& stage_prefix = stage_sm[1].str();
    const std::string& chapter = stage_sm[2].str();
    const std::string& stage_index = stage_sm[3].str();
    const std::string& difficulty = stage_sm[4].str();

    static const std::string EpisodeTaskPrefix = "Episode";
    m_chapter_task = EpisodeTaskPrefix + chapter;
    Log.info("chapter task", m_chapter_task);
    if (!Task.get(m_chapter_task)) {
        Log.error("chapter task not exists", m_chapter_task);
        return false;
    }

    if (!difficulty.empty()) {
        std::string upper_difficulty = difficulty;
        upper_difficulty[0] = static_cast<char>(::toupper(upper_difficulty[0]));
        static const std::string DifficultyTaskPrefix = "ChapterDifficulty";
        m_difficulty_task = DifficultyTaskPrefix + upper_difficulty;
        Log.info("difficulty task", m_difficulty_task);
        if (!Task.get(m_difficulty_task)) {
            Log.error("difficulty task not exists", m_difficulty_task);
            return false;
        }
    }

    std::string upper_perfix = stage_prefix;
    ranges::transform(upper_perfix, upper_perfix.begin(),
                      [](char ch) -> char { return static_cast<char>(::toupper(ch)); });
    m_stage_code = upper_perfix + chapter + "-" + stage_index;
    Log.info("stage code", m_stage_code);

    return true;
}

bool asst::StageNavigationTask::_run()
{
    LogTraceFunction;

    if (m_is_directly) {
        return ProcessTask(*this, { m_directly_task }).run();
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

    if (!ProcessTask(*this, { m_chapter_task }).run()) {
        return false;
    }

    if (!m_difficulty_task.empty()) {
        return ProcessTask(*this, { m_difficulty_task }).run();
    }

    return true;
}

bool asst::StageNavigationTask::swipe_and_find_stage()
{
    LogTraceFunction;

    // TODO: 这里多实例会炸，不能这么写，得改回原来的循环
    Task.get<OcrTaskInfo>(m_stage_code + "@ClickStageName")->text = { m_stage_code };
    Task.get<OcrTaskInfo>(m_stage_code + "@ClickedCorrectStage")->text = { m_stage_code };
    return ProcessTask(*this, { m_stage_code + "@StageNavigationBegin" }).run();
}
