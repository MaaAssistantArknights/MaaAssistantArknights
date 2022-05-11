#include "MallTask.h"

#include "ProcessTask.h"
#include "CreditShoppingTask.h"

asst::MallTask::MallTask(AsstCallback callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_mall_task_ptr(std::make_shared<ProcessTask>(callback, callback_arg, TaskType)),
    m_shopping_task_ptr(std::make_shared<CreditShoppingTask>(callback, callback_arg, TaskType))
{
    m_mall_task_ptr->set_tasks({ "MallBegin" });
    m_shopping_task_ptr->set_enable(false);

    m_subtasks.emplace_back(m_mall_task_ptr);
    m_subtasks.emplace_back(m_shopping_task_ptr);
}

bool asst::MallTask::set_params(const json::value& params)
{
    bool shopping = params.get("shopping", true);

    if (shopping) {
        if (params.contains("shopping_list") && params.at("shopping_list").is_array()) {
            std::vector<std::string> shopping_list;
            for (auto& name : params.at("shopping_list").as_array()) {
                shopping_list.emplace_back(name.as_string());
            }
            if (params.get("is_black_list", false) == true) {
                m_shopping_task_ptr->set_black_list(std::move(shopping_list));
            }
            else {
                m_shopping_task_ptr->set_white_list(std::move(shopping_list));
            }
        }
        else {
            return false;
        }
        m_shopping_task_ptr->set_enable(true);
    }
    else {
        m_shopping_task_ptr->set_enable(false);
    }
    return true;
}
