#include "DebugTask.h"

//#include "RoguelikeSkillSelectionTaskPlugin.h"

#include "StageDropsImageAnalyzer.h"

asst::DebugTask::DebugTask(const AsstCallback& callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType)
{
    ////auto task_ptr = std::make_shared<RoguelikeSkillSelectionTaskPlugin>(callback, callback_arg, TaskType);
    //auto task_ptr = std::make_shared<StageDropsTaskPlugin>(callback, callback_arg, TaskType);
    //m_subtasks.emplace_back(task_ptr);
}

bool asst::DebugTask::run()
{
    cv::Mat image = cv::imread("err/1.png");
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(1280, 720), 0, 0, cv::INTER_AREA);
    StageDropsImageAnalyzer analyzer(resized);
    return analyzer.analyze();
}
