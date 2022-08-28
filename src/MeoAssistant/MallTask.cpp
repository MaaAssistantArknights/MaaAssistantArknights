#include "MallTask.h"

#include "CreditShoppingTask.h"
#include "ProcessTask.h"

asst::MallTask::MallTask(const AsstCallback& callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
      m_mall_task_ptr(std::make_shared<ProcessTask>(callback, callback_arg, TaskType)),
      m_shopping_first_task_ptr(std::make_shared<CreditShoppingTask>(callback, callback_arg, TaskType)),
      m_shopping_task_ptr(std::make_shared<CreditShoppingTask>(callback, callback_arg, TaskType))
{
    m_mall_task_ptr->set_tasks({ "MallBegin" });
    m_shopping_first_task_ptr->set_enable(false).set_retry_times(1);
    m_shopping_task_ptr->set_enable(false).set_retry_times(1);

    m_subtasks.emplace_back(m_mall_task_ptr);
    m_subtasks.emplace_back(m_shopping_first_task_ptr);
    m_subtasks.emplace_back(m_shopping_task_ptr);
}

bool asst::MallTask::set_params(const json::value& params)
{
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
    }
    else {
        m_shopping_first_task_ptr->set_enable(false);
        m_shopping_task_ptr->set_enable(false);
    }
    return true;
}
