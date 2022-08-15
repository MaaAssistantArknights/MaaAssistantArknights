#include "StageNavigationTask.h"

#include <regex>

#include "TaskData.h"
#include "Controller.h"
#include "OcrImageAnalyzer.h"
#include "ProcessTask.h"
#include "Logger.hpp"

void asst::StageNavigationTask::set_stage_name(std::string stage_name)
{
    m_stage_name = std::move(stage_name);
}

bool asst::StageNavigationTask::_run()
{
    LogTraceFunction;

    // 已有关卡优先使用 tasks.json 中的逻辑
    if (Task.get(m_stage_name)) {
        return ProcessTask(*this, { m_stage_name }).run();
    }

    return chapter_wayfinding()
        && swipe_and_find_stage();
}

bool asst::StageNavigationTask::chapter_wayfinding()
{
    LogTraceFunction;

    static const std::regex EpisodeRegex(R"(^[A-Z]*(\d+)-\d+$)");
    std::smatch episode_sm;
    if (!std::regex_match(m_stage_name, episode_sm, EpisodeRegex)) {
        Log.error("Unkown stage", m_stage_name);
        return false;
    }

    static const std::string EpisodeTaskPrefix = "Episode";
    std::string episode_task_name = EpisodeTaskPrefix + episode_sm[1].str();

    Log.info("capter name", episode_task_name);

    return Task.get(episode_task_name)
        && ProcessTask(*this, { episode_task_name }).run();
}

bool asst::StageNavigationTask::swipe_and_find_stage()
{
    LogTraceFunction;

    ProcessTask(*this, { "SwipeToTheRight" }).run();

    auto task_ptr = Task.get("EpisodeStageNameOcr");
    for (int i = 0; i < task_ptr->max_times; ++i) {
        OcrImageAnalyzer analyzer(m_ctrler->get_image());
        analyzer.set_task_info(task_ptr);
        analyzer.set_required({ m_stage_name });
        if (analyzer.analyze()) {
            m_ctrler->click(analyzer.get_result().front().rect);
            return true;
        }
        ProcessTask(*this, { "SlowlySwipeToTheLeft" }).run();
    }
    return false;
}
