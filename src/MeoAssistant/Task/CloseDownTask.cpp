#include "CloseDownTask.h"

#include <utility>

#include "Sub/ProcessTask.h"

asst::CloseDownTask::CloseDownTask(AsstCallback callback, void* callback_arg)
    : PackageTask(std::move(callback), callback_arg, TaskType),
      m_stop_game_task_ptr(std::make_shared<StopGameTaskPlugin>(m_callback, m_callback_arg, TaskType))
{
    m_subtasks.emplace_back(m_stop_game_task_ptr)->set_ignore_error(false);
}
