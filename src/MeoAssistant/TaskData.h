#pragma once

#include "AbstractConfiger.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace asst
{
    struct TaskInfo;

    class StaticTaskData : public AbstractConfiger
    {
    public:
        StaticTaskData(const StaticTaskData&) = delete;
        StaticTaskData(StaticTaskData&&) = delete;

        virtual ~StaticTaskData() = default;

        static StaticTaskData& get_instance()
        {
            static StaticTaskData unique_instance;
            return unique_instance;
        }
        const std::unordered_set<std::string>& get_templ_required() const noexcept;

        const std::shared_ptr<TaskInfo> get(const std::string& name) const noexcept;
        std::shared_ptr<TaskInfo> get(const std::string& name);

    protected:
        StaticTaskData() = default;

        virtual bool parse(const json::value& json);

        std::unordered_map<std::string, std::shared_ptr<TaskInfo>> m_all_tasks_info;
        std::unordered_set<std::string> m_templ_required;
    };

    class TaskData
    {
    public:
        TaskData() = default;
        ~TaskData() noexcept = default;

        const std::shared_ptr<TaskInfo> get(const std::string& name) const noexcept;
        std::shared_ptr<TaskInfo> get(const std::string& name);

        bool set_ocr_text(const std::string& key, std::vector<std::string> text);

    private:
        std::unordered_map<std::string, std::shared_ptr<TaskInfo>> m_specal_tasks_info;
    };
}
