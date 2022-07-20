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

        virtual ~TaskData() = default;

        static TaskData& get_instance() noexcept
        {
            static TaskData unique_instance;
            return unique_instance;
        }
        const std::unordered_set<std::string>& get_templ_required() const noexcept;

        template<typename TargetTaskInfoType = TaskInfo>
        std::shared_ptr<TargetTaskInfoType> get(const std::string& name)
        {
            auto it = m_all_tasks_info.find(name);
            if (it == m_all_tasks_info.cend()) {
                return nullptr;
            }

            if constexpr (std::is_same_v<TaskInfo, TargetTaskInfoType>) {
                return it->second;
            }
            else if constexpr (std::is_base_of_v<TaskInfo, TargetTaskInfoType>) {
                return std::dynamic_pointer_cast<TargetTaskInfoType>(it->second);
            }
            else {
                static_assert(!sizeof(TargetTaskInfoType), "Parameter must be a TaskInfo");
            }
        }

    protected:
        TaskData() = default;

        virtual bool parse(const json::value& json);

        std::unordered_map<std::string, std::shared_ptr<TaskInfo>> m_all_tasks_info;
        std::unordered_set<std::string> m_templ_required;
    };

    static auto& Task = TaskData::get_instance();
}
