#pragma once

#include "AbstractConfigWithTempl.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "Common/AsstTypes.h"

namespace asst
{
    struct TaskInfo;

    class TaskData final : public SingletonHolder<TaskData>, public AbstractConfigWithTempl
    {
    private:
        using tasklist_t = std::vector<std::string>;
        using tasklistptr_t = std::shared_ptr<tasklist_t>;
        using taskptr_t = std::shared_ptr<TaskInfo>;

        std::shared_ptr<MatchTaskInfo> _default_match_task_info();
        std::shared_ptr<OcrTaskInfo> _default_ocr_task_info();
        std::shared_ptr<HashTaskInfo> _default_hash_task_info();
        taskptr_t _default_task_info();

        // 从模板任务生成
        const std::shared_ptr<MatchTaskInfo> default_match_task_info_ptr = _default_match_task_info();
        const std::shared_ptr<OcrTaskInfo> default_ocr_task_info_ptr = _default_ocr_task_info();
        const std::shared_ptr<HashTaskInfo> default_hash_task_info_ptr = _default_hash_task_info();
        const taskptr_t default_task_info_ptr = _default_task_info();

        // 这是下面几个函数的封装
        taskptr_t generate_task_info(const std::string& name, const json::value&, taskptr_t default_ptr,
                                     std::string_view task_prefix = "");

        taskptr_t generate_match_task_info(const std::string& name, const json::value&,
                                           std::shared_ptr<MatchTaskInfo> default_ptr);
        taskptr_t generate_ocr_task_info(const std::string& name, const json::value&,
                                         std::shared_ptr<OcrTaskInfo> default_ptr);
        taskptr_t generate_hash_task_info(const std::string& name, const json::value&,
                                          std::shared_ptr<HashTaskInfo> default_ptr);
        // TaskInfo 的基础成员
        bool append_base_task_info(taskptr_t task_info_ptr, const std::string& name, const json::value& task_json,
                                   taskptr_t default_ptr, std::string_view task_prefix = "");
        static std::string append_prefix(const std::string& task_name, std::string_view task_prefix)
        {
            if (task_prefix.ends_with('@')) [[unlikely]] {
                task_prefix.remove_suffix(1);
            }
            if (task_prefix.empty()) [[unlikely]] {
                return task_name;
            }
            if (task_name.empty()) {
                return std::string(task_prefix);
            }
            if (task_name.starts_with('#')) {
                return std::string(task_prefix) + task_name;
            }
            return std::string(task_prefix) + '@' + task_name;
        }
        static tasklist_t append_prefix(const tasklist_t& base_task_list, std::string_view task_prefix_view)
        {
            if (task_prefix_view.ends_with('@')) [[unlikely]] {
                task_prefix_view.remove_suffix(1);
            }
            if (task_prefix_view.empty()) {
                return base_task_list;
            }
            std::string task_prefix(task_prefix_view);
            tasklist_t task_list = {};
            for (const std::string& base : base_task_list) {
                std::string_view base_view = base;
                size_t l = 0;
                bool has_same_prefix = false;
                // base 任务里已经存在相同前缀，则不加前缀
                // https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/2116#issuecomment-1270115238
                for (size_t r = base_view.find('@'); r != std::string_view::npos; r = base_view.find('@', l)) {
                    if (task_prefix_view == base_view.substr(l, r - l)) [[unlikely]] {
                        has_same_prefix = true;
                        break;
                    }
                    l = r + 1;
                }
                if (has_same_prefix) [[unlikely]] {
                    task_list.emplace_back(base);
                    continue;
                }
                task_list.emplace_back(append_prefix(base, task_prefix));
            }
            return task_list;
        };
        // 运行时动态生成任务
        static taskptr_t _generate_task_info(const taskptr_t& base_ptr, std::string_view task_prefix = "")
        {
            taskptr_t task_info_ptr;
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
                task_info_ptr->name = append_prefix(base_ptr->name, task_prefix);
                task_info_ptr->sub = append_prefix(base_ptr->sub, task_prefix);
                task_info_ptr->next = append_prefix(base_ptr->next, task_prefix);
                task_info_ptr->exceeded_next = append_prefix(base_ptr->exceeded_next, task_prefix);
                task_info_ptr->on_error_next = append_prefix(base_ptr->on_error_next, task_prefix);
                task_info_ptr->reduce_other_times = append_prefix(base_ptr->reduce_other_times, task_prefix);
            }

            return task_info_ptr;
        }
        template <ranges::forward_range ListType>
        requires(!std::same_as<ranges::range_value_t<ListType>, std::string> &&
                 requires { std::declval<ranges::range_value_t<ListType>>().as_string(); })
        static tasklist_t to_string_list(const ListType& other_string_list)
        {
            tasklist_t task_list = {};
            task_list.reserve(other_string_list.size());
            ranges::copy(other_string_list | views::transform(&ranges::range_value_t<ListType>::as_string),
                         std::back_inserter(task_list));
            return task_list;
        }

        std::string_view task_name_view(std::string_view task_name) { return *m_task_names.emplace(task_name).first; }
        decltype(auto) insert_or_assign_raw_task(std::string_view task_name, taskptr_t task_info_ptr)
        {
            return m_raw_all_tasks_info.insert_or_assign(task_name_view(task_name), task_info_ptr);
        }
        decltype(auto) insert_or_assign_task(std::string_view task_name, taskptr_t task_info_ptr)
        {
            return m_all_tasks_info.insert_or_assign(task_name_view(task_name), task_info_ptr);
        }
        bool explain_tasks(tasklist_t& new_tasks, const tasklist_t& raw_tasks, std::string_view name,
                           bool& task_changed, bool multi);
        std::optional<taskptr_t> expand_task(std::string_view name, taskptr_t old_task);
#ifdef ASST_DEBUG
        bool syntax_check(const std::string& task_name, const json::value& task_json);
#endif
        std::shared_ptr<TaskInfo> get_raw(std::string_view name) const;
        template <typename TargetTaskInfoType>
        requires(std::derived_from<TargetTaskInfoType, TaskInfo> &&
                 !std::same_as<TargetTaskInfoType, TaskInfo>) // Parameter must be a TaskInfo
        std::shared_ptr<TargetTaskInfoType> get_raw(std::string_view name) const
        {
            return std::dynamic_pointer_cast<TargetTaskInfoType>(get_raw(name));
        }

    public:
        virtual ~TaskData() override = default;
        virtual const std::unordered_set<std::string>& get_templ_required() const noexcept override;

        std::shared_ptr<TaskInfo> get(std::string_view name);
        template <typename TargetTaskInfoType>
        requires(std::derived_from<TargetTaskInfoType, TaskInfo> &&
                 !std::same_as<TargetTaskInfoType, TaskInfo>) // Parameter must be a TaskInfo
        std::shared_ptr<TargetTaskInfoType> get(std::string_view name)
        {
            return std::dynamic_pointer_cast<TargetTaskInfoType>(get(name));
        }

    protected:
        virtual bool parse(const json::value& json) override;

        std::unordered_set<std::string> m_task_names;
        std::unordered_map<std::string_view, taskptr_t> m_raw_all_tasks_info;
        std::unordered_map<std::string_view, taskptr_t> m_all_tasks_info;
        std::unordered_set<std::string> m_templ_required;
    };

    inline static auto& Task = TaskData::get_instance();
} // namespace asst
