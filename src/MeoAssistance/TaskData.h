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

        bool set_param(const std::string& type, const std::string& param, const std::string& value);

        const std::shared_ptr<TaskInfo> get(const std::string& name) const noexcept;
        const std::unordered_set<std::string>& get_templ_required() const noexcept;
        std::shared_ptr<TaskInfo> get(std::string name);
        void clear_exec_times();

    protected:
        TaskData() = default;

        virtual bool parse(const json::value& json);

        std::unordered_map<std::string, std::shared_ptr<TaskInfo>> m_all_tasks_info;
        std::unordered_set<std::string> m_templ_required;
    };

    static auto& task = TaskData::get_instance();
}
