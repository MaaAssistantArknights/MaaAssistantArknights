#pragma once

#include "AbstractConfiger.h"

#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace asst {
    struct TaskInfo;

    class TaskData : public AbstractConfiger
    {
    public:
        virtual ~TaskData() = default;

        bool set_param(const std::string& type, const std::string& param, const std::string& value);

        const std::shared_ptr<TaskInfo> task_ptr(const std::string& name) const noexcept;
        const std::unordered_set<std::string>& get_templ_required() const noexcept;
        std::shared_ptr<TaskInfo> task_ptr(std::string name);
        void clear_exec_times();

    protected:
        virtual bool parse(const json::value& json);

        std::unordered_map<std::string, std::shared_ptr<TaskInfo>> m_all_tasks_info;
        std::unordered_set<std::string> m_templ_required;
    };
}