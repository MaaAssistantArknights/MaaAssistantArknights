#include "AwardTask.h"

#include <utility>

#include "Task/ProcessTask.h"

#include "Utils/Logger.hpp"

asst::AwardTask::AwardTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      award_task_ptr(std::make_shared<ProcessTask>(m_callback, m_inst, TaskType)),
      mail_task_ptr(std::make_shared<ProcessTask>(m_callback, m_inst, TaskType)),
      recruit_task_ptr(std::make_shared<ProcessTask>(m_callback, m_inst, TaskType))
{
    LogTraceFunction;

    award_task_ptr->set_tasks({ "AwardBegin" });
    mail_task_ptr->set_tasks({ "MailBegin" });
    recruit_task_ptr->set_tasks({ "RecruitingActivitiesBegin" });

    m_subtasks.emplace_back(award_task_ptr);
    m_subtasks.emplace_back(mail_task_ptr);
    m_subtasks.emplace_back(recruit_task_ptr);
}

bool asst::AwardTask::set_params(const json::value& params)
{
    LogTraceFunction;

    bool award = params.get("award", true);
    bool mail = params.get("mail", false);
    bool recruit = params.get("recruit", false);

    award_task_ptr->set_enable(award);
    mail_task_ptr->set_enable(mail);
    recruit_task_ptr->set_enable(recruit);

    return true;
}
