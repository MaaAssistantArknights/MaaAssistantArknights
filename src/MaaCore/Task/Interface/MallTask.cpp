#include "MallTask.h"

#include "Task/Miscellaneous/CreditFightTask.h"
#include "Task/Miscellaneous/CreditShoppingTask.h"
#include "Task/ProcessTask.h"

#include "Utils/Logger.hpp"

asst::MallTask::MallTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_visit_task_ptr(std::make_shared<ProcessTask>(m_callback, m_inst, TaskType)),
      m_credit_fight_task_ptr(std::make_shared<CreditFightTask>(callback, inst, TaskType)),
      m_mall_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType)),
      m_shopping_first_task_ptr(std::make_shared<CreditShoppingTask>(callback, inst, TaskType)),
      m_shopping_task_ptr(std::make_shared<CreditShoppingTask>(callback, inst, TaskType)),
      m_shopping_force_task_ptr(std::make_shared<CreditShoppingTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_visit_task_ptr->set_tasks({ "VisitBegin" });
    m_mall_task_ptr->set_tasks({ "MallBegin" });
    m_shopping_first_task_ptr->set_enable(false).set_retry_times(1);
    m_shopping_task_ptr->set_enable(false).set_retry_times(1);
    m_shopping_force_task_ptr->set_enable(false).set_retry_times(1);

    m_subtasks.emplace_back(m_visit_task_ptr)->set_ignore_error(true);
    m_subtasks.emplace_back(m_credit_fight_task_ptr)->set_ignore_error(true);
    m_subtasks.emplace_back(m_mall_task_ptr);
    m_subtasks.emplace_back(m_shopping_first_task_ptr)->set_ignore_error(true);
    m_subtasks.emplace_back(m_shopping_task_ptr)->set_ignore_error(true);
    m_subtasks.emplace_back(m_shopping_force_task_ptr)->set_ignore_error(true);
}

bool asst::MallTask::set_params(const json::value& params)
{
    LogTraceFunction;

    bool shopping = params.get("shopping", true);

    if (shopping) {
        if (auto buy_first_opt = params.find<json::array>("buy_first")) {
            std::vector<std::string> buy_first;
            for (auto& name : buy_first_opt.value()) {
                if (std::string name_str = name.as_string(); !name_str.empty()) {
                    buy_first.emplace_back(name_str);
                }
            }
            if (!buy_first.empty()) {
                m_shopping_first_task_ptr->set_white_list(std::move(buy_first));
                m_shopping_first_task_ptr->set_enable(true);
            }
            else {
                m_shopping_first_task_ptr->set_enable(false);
            }
        }
        else {
            m_shopping_first_task_ptr->set_enable(false);
        }

        if (auto blacklist_opt = params.find<json::array>("blacklist")) {
            std::vector<std::string> shopping_list;
            for (auto& name : blacklist_opt.value()) {
                if (std::string name_str = name.as_string(); !name_str.empty()) {
                    shopping_list.emplace_back(name.as_string());
                }
            }
            m_shopping_task_ptr->set_black_list(std::move(shopping_list));
        }

        m_shopping_task_ptr->set_enable(true);

        m_shopping_first_task_ptr->set_force_shopping_if_credit_full(false);
        m_shopping_task_ptr->set_force_shopping_if_credit_full(false);
        bool force_shopping_if_credit_full = params.get("force_shopping_if_credit_full", false);

        if (force_shopping_if_credit_full) {
            m_shopping_force_task_ptr->set_enable(true);
            m_shopping_force_task_ptr->set_force_shopping_if_credit_full(true);
        }
        else {
            m_shopping_force_task_ptr->set_enable(false);
            m_shopping_force_task_ptr->set_force_shopping_if_credit_full(false);
        }
    }
    else {
        m_shopping_first_task_ptr->set_enable(false);
        m_shopping_task_ptr->set_enable(false);
        m_shopping_force_task_ptr->set_enable(false);
    }

    if (!m_running) {
        bool credit_fight = params.get("credit_fight", false);
        m_credit_fight_task_ptr->set_enable(credit_fight);
    }

    return true;
}
