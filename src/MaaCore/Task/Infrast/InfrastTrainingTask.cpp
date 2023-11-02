#include "InfrastTrainingTask.h"

#include "Task/ProcessTask.h"

bool asst::InfrastTrainingTask::_run()
{
    m_all_available_opers.clear();

    set_product("SkillLevel");

    swipe_to_the_right_of_main_ui();

    if (!enter_facility()) {
        return false;
    }
    enter_facility();
    
    // 训练未完成或没有训练，退出
    if (!training_completed()) {
        return true;
    }

    return true;
}

bool asst::InfrastTrainingTask::training_completed()
{
    ProcessTask task(*this, { "InfrastTrainingCompleted" });
    return task.run();
}
