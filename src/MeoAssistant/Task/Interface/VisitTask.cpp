#include "VisitTask.h"

#include <utility>

#include "Task/Miscellaneous/CreditFightTask.h"
#include "Task/ProcessTask.h"

asst::VisitTask::VisitTask(const AsstCallback& callback, void* callback_arg)
    : InterfaceTask(callback, callback_arg, TaskType),
      m_visit_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType)),
      m_credit_fight_task_ptr(std::make_shared<CreditFightTask>(callback, callback_arg, TaskType))
{
    m_visit_task_ptr->set_tasks({ "VisitBegin" });
    m_subtasks.emplace_back(m_visit_task_ptr)->set_ignore_error(false);
    m_subtasks.emplace_back(m_credit_fight_task_ptr);
}

bool asst::VisitTask::set_params(const json::value& params)
{
    bool credit_fight = params.get("credit_fight", false);

    m_credit_fight_task_ptr->set_enable(credit_fight);

    return true;
}
