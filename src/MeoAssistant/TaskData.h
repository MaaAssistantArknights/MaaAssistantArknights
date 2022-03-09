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

        static TaskData& get_instance()
        {
            static TaskData unique_instance;
            return unique_instance;
        }
        const std::unordered_set<std::string>& get_templ_required() const noexcept;

        const std::shared_ptr<TaskInfo> get(const std::string& name) const noexcept;
        std::shared_ptr<TaskInfo> get(const std::string& name);

    protected:
        TaskData() = default;

        virtual bool parse(const json::value& json);

        std::unordered_map<std::string, std::shared_ptr<TaskInfo>> m_all_tasks_info;
        std::unordered_set<std::string> m_templ_required;
    };

    static auto& Task = TaskData::get_instance();
}
