#include "AwardTask.h"

#include <utility>

#include "Task/ProcessTask.h"

#include "Utils/Logger.hpp"

asst::AwardTask::AwardTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    award_task_ptr(std::make_shared<ProcessTask>(m_callback, m_inst, TaskType)), 
    mail_task_ptr(std::make_shared<ProcessTask>(m_callback, m_inst, TaskType))
{
    LogTraceFunction;

    award_task_ptr->set_tasks({ "AwardBegin" });
    mail_task_ptr->set_tasks({ "MailBegin" });

    m_subtasks.emplace_back(award_task_ptr);
    m_subtasks.emplace_back(mail_task_ptr);
}

bool asst::AwardTask::set_params(const json::value& params)
{
    LogTraceFunction;

    bool mail = params.get("mail", true);
    mail_task_ptr->set_enable(mail);

    return true;
}
