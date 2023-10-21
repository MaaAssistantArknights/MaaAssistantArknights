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
        taskptr_t generate_task_info(std::string_view name, const json::value&, taskptr_t default_ptr,
                                     std::string_view task_prefix = "");

        taskptr_t generate_match_task_info(std::string_view name, const json::value&,
                                           std::shared_ptr<MatchTaskInfo> default_ptr);
        taskptr_t generate_ocr_task_info(std::string_view name, const json::value&,
                                         std::shared_ptr<OcrTaskInfo> default_ptr);
        taskptr_t generate_hash_task_info(std::string_view name, const json::value&,
                                          std::shared_ptr<HashTaskInfo> default_ptr);
        // TaskInfo 的基础成员
        bool append_base_task_info(taskptr_t task_info_ptr, std::string_view name, const json::value& task_json,
                                   taskptr_t default_ptr, std::string_view task_prefix = "");
        static std::string append_prefix(std::string_view task_name, std::string_view task_prefix)
        {
            if (task_prefix.ends_with('@')) [[unlikely]] {
                task_prefix.remove_suffix(1);
            }
            if (task_prefix.empty()) [[unlikely]] {
                return std::string(task_name);
            }
            if (task_name.empty()) {
                return std::string(task_prefix);
            }
            if (task_name.starts_with('#')) {
                return std::string(task_prefix) + std::string(task_name);
            }
            return std::string(task_prefix) + '@' + std::string(task_name);
        }
        static tasklist_t append_prefix(const tasklist_t& base_task_list, std::string_view task_prefix)
        {
            if (task_prefix.ends_with('@')) [[unlikely]] {
                task_prefix.remove_suffix(1);
            }
            if (task_prefix.empty()) {
                return base_task_list;
            }
            tasklist_t task_list = {};
            for (const std::string& base : base_task_list) {
                std::string_view base_view = base;
                size_t l = 0;
                bool has_same_prefix = false;
                // base 任务里已经存在相同前缀，则不加前缀
                // https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/2116#issuecomment-1270115238
                for (size_t r = base_view.find('@'); r != std::string_view::npos; r = base_view.find('@', l)) {
                    if (task_prefix == base_view.substr(l, r - l)) [[unlikely]] {
                        has_same_prefix = true;
                        break;
                    }
                    l = r + 1;
                }
                task_list.emplace_back(has_same_prefix ? base : append_prefix(base_view, task_prefix));
            }
            return task_list;
        };

        // 运行时动态生成任务
        static taskptr_t _generate_task_info(const taskptr_t& base_ptr)
        {
            if (!base_ptr) [[unlikely]] {
                return nullptr;
            }
            switch (base_ptr->algorithm) {
            case AlgorithmType::MatchTemplate:
                return std::make_shared<MatchTaskInfo>(*std::dynamic_pointer_cast<MatchTaskInfo>(base_ptr));
            case AlgorithmType::OcrDetect:
                return std::make_shared<OcrTaskInfo>(*std::dynamic_pointer_cast<OcrTaskInfo>(base_ptr));
            case AlgorithmType::Hash:
                return std::make_shared<HashTaskInfo>(*std::dynamic_pointer_cast<HashTaskInfo>(base_ptr));
            default:
                return std::make_shared<TaskInfo>(*base_ptr);
            }
        }
        static taskptr_t _generate_task_info(const taskptr_t& base_ptr, std::string_view task_prefix)
        {
            taskptr_t task_info_ptr = _generate_task_info(base_ptr);
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
        bool compile_tasklist(tasklist_t& new_tasks, const tasklist_t& raw_tasks, std::string_view name,
                              bool& task_changed, bool multi);
        bool generate_task_and_its_base(std::string_view name, bool must_true);
#ifdef ASST_DEBUG
        bool syntax_check(std::string_view task_name, const json::value& task_json);
#endif
        taskptr_t get_raw(std::string_view name);
        template <typename TargetTaskInfoType>
        requires(std::derived_from<TargetTaskInfoType, TaskInfo> &&
                 !std::same_as<TargetTaskInfoType, TaskInfo>) // Parameter must be a TaskInfo
        std::shared_ptr<TargetTaskInfoType> get_raw(std::string_view name)
        {
            return std::dynamic_pointer_cast<TargetTaskInfoType>(get_raw(name));
        }

    public:
        virtual ~TaskData() override = default;
        virtual const std::unordered_set<std::string>& get_templ_required() const noexcept override;
        void clear_tasks();
        void set_task_base(const std::string_view task_name, std::string base_task_name);
        bool lazy_parse(const json::value& json);
        virtual bool load(const std::filesystem::path& path) override;

        taskptr_t get(std::string_view name);
        template <typename TargetTaskInfoType>
        requires(std::derived_from<TargetTaskInfoType, TaskInfo> &&
                 !std::same_as<TargetTaskInfoType, TaskInfo>) // Parameter must be a TaskInfo
        std::shared_ptr<TargetTaskInfoType> get(std::string_view name)
        {
            return std::dynamic_pointer_cast<TargetTaskInfoType>(get(name));
        }
        std::optional<json::object> get_json(std::string_view name) const
        {
            if (m_json_all_tasks_info.find(name) != m_json_all_tasks_info.cend())
                return m_json_all_tasks_info.at(name);
            else
                return std::nullopt;
        }

    protected:
        enum TaskStatus
        {
            NotToBeGenerate = 0, // 已经显式生成 或 不是待显式生成 的任务
            ToBeGenerate,        // 待生成 的任务
            Generating,          // 正在生成 的任务
            NotExists,           // 不存在的任务
        };

        virtual bool parse(const json::value& json) override;

        std::unordered_set<std::string> m_task_names;
        std::unordered_set<std::string> m_templ_required;
        std::unordered_map<std::string_view, TaskStatus> m_task_status;
        std::unordered_map<std::string_view, json::object> m_json_all_tasks_info; // 原始的 json 信息
        std::unordered_map<std::string_view, taskptr_t> m_raw_all_tasks_info;     // 未展开虚任务的任务信息
        std::unordered_map<std::string_view, taskptr_t> m_all_tasks_info;         // 已展开虚任务的任务信息
    };

    inline static auto& Task = TaskData::get_instance();
} // namespace asst
