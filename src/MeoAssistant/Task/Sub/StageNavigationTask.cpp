#include "StageNavigationTask.h"

#include <regex>

#include "Controller.h"
#include "ImageAnalyzer/General/OcrImageAnalyzer.h"
#include "ProcessTask.h"
#include "TaskData.h"
#include "Utils/Logger.hpp"

void asst::StageNavigationTask::set_stage_name(std::string stage_name)
{
    m_stage_name = std::move(stage_name);
}

bool asst::StageNavigationTask::_run()
{
    LogTraceFunction;
    m_cur_retry = m_retry_times;

    // 已有关卡优先使用 tasks.json 中的逻辑
    if (Task.get(m_stage_name)) {
        return ProcessTask(*this, { m_stage_name }).run();
    }

    return chapter_wayfinding() && swipe_and_find_stage();
}

bool asst::StageNavigationTask::chapter_wayfinding()
{
    LogTraceFunction;

    static const std::regex EpisodeRegex(R"(^[A-Z]*(\d+)-\d+$)");
    std::smatch episode_sm;
    if (!std::regex_match(m_stage_name, episode_sm, EpisodeRegex)) {
        Log.error("Unknown stage", m_stage_name);
        return false;
    }

    static const std::string EpisodeTaskPrefix = "Episode";
    std::string episode_task_name = EpisodeTaskPrefix + episode_sm[1].str();

    Log.info("chapter name", episode_task_name);

    return Task.get(episode_task_name) && ProcessTask(*this, { episode_task_name }).run();
}

bool asst::StageNavigationTask::swipe_and_find_stage()
{
    LogTraceFunction;
    std::dynamic_pointer_cast<OcrTaskInfo>(Task.get(m_stage_name + "@ClickStageName"))->text = { m_stage_name };
    std::dynamic_pointer_cast<OcrTaskInfo>(Task.get(m_stage_name + "@ClickedCorrectStage"))->text = { m_stage_name };
    return ProcessTask(*this, { m_stage_name + "@StageNavigationBegin" }).run();
}
