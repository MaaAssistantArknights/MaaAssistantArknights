#include "DebugTask.h"

//#include "RoguelikeSkillSelectionTaskPlugin.h"

#include "StageDropsTaskPlugin.h"

asst::DebugTask::DebugTask(AsstCallback callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType)
{
    //auto task_ptr = std::make_shared<RoguelikeSkillSelectionTaskPlugin>(callback, callback_arg, TaskType);
    auto task_ptr = std::make_shared<StageDropsTaskPlugin>(callback, callback_arg, TaskType);
    m_subtasks.emplace_back(task_ptr);
}
