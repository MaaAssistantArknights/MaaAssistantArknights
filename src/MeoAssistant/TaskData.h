#pragma once

#include "AbstractConfiger.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace asst
{
    struct TaskInfo;

    class TaskData : public AbstractConfiger
    {
    public:
        TaskData(const TaskData&) = delete;
        TaskData(TaskData&&) = delete;

        virtual ~TaskData() override = default;

        static TaskData& get_instance() noexcept
        {
            static TaskData unique_instance;
            return unique_instance;
        }
        const std::unordered_set<std::string>& get_templ_required() const noexcept;

        template<typename TargetTaskInfoType>
        requires std::derived_from<TargetTaskInfoType, TaskInfo>
             && (!std::same_as<TargetTaskInfoType, TaskInfo>) // Parameter must be a TaskInfo and not same as TaskInfo
        std::shared_ptr<TargetTaskInfoType> get(const std::string& name)
        {
            auto it = m_all_tasks_info.find(name);
            if (it == m_all_tasks_info.cend()) {
                return nullptr;
            }

            return std::dynamic_pointer_cast<TargetTaskInfoType>(it->second);
        }

        template<typename TargetTaskInfoType = TaskInfo>
        requires std::same_as<TargetTaskInfoType, TaskInfo> // Parameter must be a TaskInfo
        std::shared_ptr<TargetTaskInfoType> get(const std::string& name)
        {
            auto it = m_all_tasks_info.find(name);
            if (it == m_all_tasks_info.cend()) {
                return nullptr;
            }

            return it->second;
        }

    protected:
        TaskData() = default;

        virtual bool parse(const json::value& json) override;

        std::unordered_map<std::string, std::shared_ptr<TaskInfo>> m_all_tasks_info;
        std::unordered_set<std::string> m_templ_required;
    };

    static auto& Task = TaskData::get_instance();
}
