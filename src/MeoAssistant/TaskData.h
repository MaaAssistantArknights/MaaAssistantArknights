#pragma once

#include "Resource/AbstractConfigerWithTempl.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "Utils/AsstTypes.h"
#ifdef ASST_DEBUG
#include "Utils/Logger.hpp"
#endif

namespace asst
{
    struct TaskInfo;

    class TaskData final : public SingletonHolder<TaskData>, public AbstractConfigerWithTempl
    {
    private:
        std::shared_ptr<MatchTaskInfo> _default_match_task_info();
        std::shared_ptr<OcrTaskInfo> _default_ocr_task_info();
        std::shared_ptr<HashTaskInfo> _default_hash_task_info();
        std::shared_ptr<TaskInfo> _default_task_info();

        // 从模板任务生成
        const std::shared_ptr<MatchTaskInfo> default_match_task_info_ptr = _default_match_task_info();
        const std::shared_ptr<OcrTaskInfo> default_ocr_task_info_ptr = _default_ocr_task_info();
        const std::shared_ptr<HashTaskInfo> default_hash_task_info_ptr = _default_hash_task_info();
        const std::shared_ptr<TaskInfo> default_task_info_ptr = _default_task_info();

        // 这是下面几个函数的封装
        std::shared_ptr<TaskInfo> generate_task_info(const std::string& name, const json::value&,
                                                     std::shared_ptr<TaskInfo> default_ptr,
                                                     std::string_view task_prefix = "");

        std::shared_ptr<TaskInfo> generate_match_task_info(const std::string& name, const json::value&,
                                                           std::shared_ptr<MatchTaskInfo> default_ptr);
        std::shared_ptr<TaskInfo> generate_ocr_task_info(const std::string& name, const json::value&,
                                                         std::shared_ptr<OcrTaskInfo> default_ptr);
        std::shared_ptr<TaskInfo> generate_hash_task_info(const std::string& name, const json::value&,
                                                          std::shared_ptr<HashTaskInfo> default_ptr);
        // TaskInfo 的基础成员
        bool append_base_task_info(std::shared_ptr<TaskInfo> task_info_ptr, const std::string& name,
                                   const json::value& task_json, std::shared_ptr<TaskInfo> default_ptr,
                                   std::string_view task_prefix = "");

        static std::vector<std::string> append_prefix(const std::vector<std::string>& base_task_list,
                                                      std::string_view task_prefix)
        {
            if (task_prefix.empty()) {
                return base_task_list;
            }
            std::vector<std::string> task_list = {};
            for (const std::string& base : base_task_list) {
                std::string_view base_view = base;
                size_t l = 0;
                bool has_same_prefix = false;
                // base 任务里已经存在相同前缀，则不加前缀
                // https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/2116#issuecomment-1270115238
                for (size_t r = base_view.find('@'); r != std::string_view::npos; r = base_view.find('@', l)) {
                    if (task_prefix == base_view.substr(l, r - l)) {
                        has_same_prefix = true;
                        break;
                    }
                    l = r + 1;
                }
                if (has_same_prefix) {
                    task_list.emplace_back(base);
                }
                else {
                    task_list.emplace_back(std::string(task_prefix) + "@" + base);
                }
            }
            return task_list;
        };
        // 运行时动态生成任务
        static std::shared_ptr<TaskInfo> _generate_task_info(const std::shared_ptr<TaskInfo>& base_ptr,
                                                             std::string_view task_prefix = "")
        {
            std::shared_ptr<TaskInfo> task_info_ptr;
            switch (base_ptr->algorithm) {
            case AlgorithmType::MatchTemplate:
                task_info_ptr = std::make_shared<MatchTaskInfo>(*std::dynamic_pointer_cast<MatchTaskInfo>(base_ptr));
                break;
            case AlgorithmType::OcrDetect:
                task_info_ptr = std::make_shared<OcrTaskInfo>(*std::dynamic_pointer_cast<OcrTaskInfo>(base_ptr));
                break;
            case AlgorithmType::Hash:
                task_info_ptr = std::make_shared<HashTaskInfo>(*std::dynamic_pointer_cast<HashTaskInfo>(base_ptr));
                break;
            default:
                task_info_ptr = std::make_shared<TaskInfo>(*base_ptr);
                break;
            }

            if (!task_prefix.empty()) {
                task_info_ptr->name = std::string(task_prefix) + "@" + base_ptr->name;
                task_info_ptr->sub = append_prefix(base_ptr->sub, task_prefix);
                task_info_ptr->next = append_prefix(base_ptr->next, task_prefix);
                task_info_ptr->exceeded_next = append_prefix(base_ptr->exceeded_next, task_prefix);
                task_info_ptr->on_error_next = append_prefix(base_ptr->on_error_next, task_prefix);
                task_info_ptr->reduce_other_times = append_prefix(base_ptr->reduce_other_times, task_prefix);
            }

            return task_info_ptr;
        }

        std::string_view task_name_view(std::string_view task_name) { return *m_task_names.emplace(task_name).first; }
        void insert_or_assign_raw_task(std::string_view task_name, std::shared_ptr<TaskInfo> task_info_ptr)
        {
            m_raw_all_tasks_info.insert_or_assign(task_name_view(task_name), task_info_ptr);
        }
        void insert_or_assign_task(std::string_view task_name, std::shared_ptr<TaskInfo> task_info_ptr)
        {
            m_all_tasks_info.insert_or_assign(task_name_view(task_name), task_info_ptr);
        }

    public:
        virtual ~TaskData() override = default;
        virtual const std::unordered_set<std::string>& get_templ_required() const noexcept override;

        template <typename TargetTaskInfoType = TaskInfo>
        requires(std::derived_from<TargetTaskInfoType, TaskInfo> ||
                 std::same_as<TargetTaskInfoType, TaskInfo>) // Parameter must be a TaskInfo
        std::shared_ptr<TargetTaskInfoType> get(std::string_view name)
        {
            // 普通 task 或已经生成过的 `@` 型 task
            if (auto it = m_all_tasks_info.find(name); it != m_all_tasks_info.cend()) [[likely]] {
                if constexpr (std::same_as<TargetTaskInfoType, TaskInfo>) {
                    return it->second;
                }
                else {
                    return std::dynamic_pointer_cast<TargetTaskInfoType>(it->second);
                }
            }

            size_t at_pos = name.find('@');
            if (at_pos == std::string_view::npos) [[unlikely]] {
                return nullptr;
            }

            // `@` 前面的字符长度
            size_t name_len = at_pos;
            auto base_task_iter = get(name.substr(name_len + 1));
            if (base_task_iter == nullptr) [[unlikely]] {
                return nullptr;
            }

            std::string_view derived_task_name = name.substr(0, name_len);
            auto task_info_ptr = _generate_task_info(base_task_iter, derived_task_name);
            if (task_info_ptr == nullptr) [[unlikely]] {
                return nullptr;
            }

            // tasks 个数超过上限时不再 emplace，返回临时值
            constexpr size_t MAX_TASKS_SIZE = 65535;
            if (m_all_tasks_info.size() < MAX_TASKS_SIZE) [[likely]] {
                insert_or_assign_task(name, task_info_ptr);
            }
#ifdef ASST_DEBUG
            else {
                Log.debug("Task count has exceeded the upper limit:", MAX_TASKS_SIZE, "current task:", name);
            }
#endif // ASST_DEBUG

            if constexpr (std::same_as<TargetTaskInfoType, TaskInfo>) {
                return task_info_ptr;
            }
            else {
                return std::dynamic_pointer_cast<TargetTaskInfoType>(task_info_ptr);
            }
        }

    protected:
        virtual bool parse(const json::value& json) override;

        template <typename TargetTaskInfoType = TaskInfo>
        requires(std::derived_from<TargetTaskInfoType, TaskInfo> ||
                 std::same_as<TargetTaskInfoType, TaskInfo>) // Parameter must be a TaskInfo
        std::shared_ptr<TargetTaskInfoType> get_raw(std::string_view name) const
        {
            // 普通 task 或已经生成过的 `@` 型 task
            if (auto it = m_raw_all_tasks_info.find(name); it != m_raw_all_tasks_info.cend()) [[likely]] {
                if constexpr (std::same_as<TargetTaskInfoType, TaskInfo>) {
                    return it->second;
                }
                else {
                    return std::dynamic_pointer_cast<TargetTaskInfoType>(it->second);
                }
            }

            size_t at_pos = name.find('@');
            if (at_pos == std::string_view::npos) [[unlikely]] {
                return nullptr;
            }

            // `@` 前面的字符长度
            size_t name_len = at_pos;
            auto base_task_iter = get_raw(name.substr(name_len + 1));
            if (base_task_iter == nullptr) [[unlikely]] {
                return nullptr;
            }

            std::string_view derived_task_name = name.substr(0, name_len);
            auto task_info_ptr = _generate_task_info(base_task_iter, derived_task_name);
            if (task_info_ptr == nullptr) [[unlikely]] {
                return nullptr;
            }

            if constexpr (std::same_as<TargetTaskInfoType, TaskInfo>) {
                return task_info_ptr;
            }
            else {
                return std::dynamic_pointer_cast<TargetTaskInfoType>(task_info_ptr);
            }
        }

    private:
#ifdef ASST_DEBUG
        bool syntax_check(const std::string& task_name, const json::value& task_json);
#endif

    protected:
        std::unordered_set<std::string> m_task_names;
        std::unordered_map<std::string_view, std::shared_ptr<TaskInfo>> m_raw_all_tasks_info;
        std::unordered_map<std::string_view, std::shared_ptr<TaskInfo>> m_all_tasks_info;
        std::unordered_set<std::string> m_templ_required;
    };

    inline static auto& Task = TaskData::get_instance();
} // namespace asst
