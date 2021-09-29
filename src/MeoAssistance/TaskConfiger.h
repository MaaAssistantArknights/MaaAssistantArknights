#pragma once

#include "AbstractConfiger.h"

#include <unordered_map>

namespace asst {
    class TaskInfo;

    class TaskConfiger : public AbstractConfiger
    {
    public:
        virtual ~TaskConfiger() = default;

        static TaskConfiger& get_instance() {
            static TaskConfiger unique_instance;
            return unique_instance;
        }

        bool set_param(const std::string& type, const std::string& param, const std::string& value);

        std::unordered_map<std::string, std::shared_ptr<TaskInfo>> m_all_tasks_info;
    protected:
        virtual bool parse(const json::value& json);
    };
}