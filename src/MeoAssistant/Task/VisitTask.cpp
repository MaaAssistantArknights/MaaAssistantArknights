#include "VisitTask.h"

#include <utility>

#include "Sub/ProcessTask.h"
#include "CreditFightTask.h"

asst::VisitTask::VisitTask(AsstCallback callback, void* callback_arg)
    : PackageTask(std::move(callback), callback_arg, TaskType),
      m_visit_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType)),
      m_credit_fight_task_ptr(std::make_shared<CreditFightTask>(m_callback, m_callback_arg))
{
    m_visit_task_ptr->set_tasks({ "VisitBegin" });
    m_subtasks.emplace_back(m_visit_task_ptr)->set_ignore_error(false);

    m_subtasks.emplace_back(m_credit_fight_task_ptr)->set_ignore_error(false);
}
