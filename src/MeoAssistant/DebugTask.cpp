#include "DebugTask.h"

//#include "RoguelikeSkillSelectionTaskPlugin.h"

#include "BattleProcessTask.h"

asst::DebugTask::DebugTask(AsstCallback callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType)
{
    //auto task_ptr = std::make_shared<RoguelikeSkillSelectionTaskPlugin>(callback, callback_arg, TaskType);
    auto task_ptr = std::make_shared<BattleProcessTask>(callback, callback_arg, TaskType);
    task_ptr->set_stage_name("千层蛋糕");
    m_subtasks.emplace_back(task_ptr);
}
