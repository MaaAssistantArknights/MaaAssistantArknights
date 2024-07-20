#pragma once

#include "AbstractConfigWithTempl.h"

#include <unordered_map>
#include <unordered_set>

#include "Common/AsstTypes.h"
#include "TaskData/TaskDataSymbol.h"

namespace asst
{
    class TaskData final : public SingletonHolder<TaskData>, public AbstractConfigWithTempl
    {
    private:
        static MatchTaskConstPtr _default_match_task_info();
        static OcrTaskConstPtr _default_ocr_task_info();
        static TaskConstPtr _default_task_info();

        // 从模板任务生成
        static inline const MatchTaskConstPtr default_match_task_info_ptr = _default_match_task_info();
        static inline const OcrTaskConstPtr default_ocr_task_info_ptr = _default_ocr_task_info();
        static inline const TaskConstPtr default_task_info_ptr = _default_task_info();

        static std::string append_prefix(std::string_view task_name, std::string_view task_prefix);
        static TaskList append_prefix(const TaskList& base_task_list, std::string_view task_prefix);

        template <ranges::forward_range ListType>
        requires(!std::same_as<ranges::range_value_t<ListType>, std::string> &&
                 requires { std::declval<ranges::range_value_t<ListType>>().as_string(); })
        static TaskList to_string_list(const ListType& other_string_list)
        {
            TaskList task_list = {};
            task_list.reserve(other_string_list.size());
            ranges::copy(other_string_list | views::transform(&ranges::range_value_t<ListType>::as_string),
                         std::back_inserter(task_list));
            return task_list;
        }

        static inline std::unordered_set<std::string> m_task_names {};
        static const std::string& task_name_view(std::string_view name) { return *m_task_names.emplace(name).first; }

        struct RawCompileResult
        {
            bool task_changed;
            TaskDataSymbol::Symbols symbols;
        };
        static ResultOrError<RawCompileResult> compile_raw_tasklist(
            const TaskList& raw_tasks, std::string_view self_name,
            std::function<TaskDerivedConstPtr(std::string_view)> get_raw, bool allow_duplicate);

    private:
        TaskPtr generate_task_info(std::string_view name);
        TaskPtr generate_match_task_info(std::string_view name, const json::value&, MatchTaskConstPtr default_ptr,
                                         TaskDerivedType derived_type);
        TaskPtr generate_ocr_task_info(std::string_view name, const json::value&, OcrTaskConstPtr default_ptr);
        decltype(auto) insert_or_assign_raw_task(std::string_view task_name, TaskDerivedPtr task_info_ptr)
        {
            return m_raw_all_tasks_info.insert_or_assign(task_name_view(task_name), task_info_ptr);
        }
        decltype(auto) insert_or_assign_task(std::string_view task_name, TaskPtr task_info_ptr)
        {
            return m_all_tasks_info.insert_or_assign(task_name_view(task_name), task_info_ptr);
        }
        struct CompileResult
        {
            bool task_changed;
            TaskList tasks;
        };
        ResultOrError<CompileResult> compile_tasklist(const TaskList& raw_tasks, std::string_view self_name,
                                                      bool allow_duplicate);
        bool generate_raw_task_info(std::string_view name, std::string_view prefix, std::string_view base_name,
                                    const json::value& task_json, TaskDerivedType type);
        bool generate_raw_task_and_base(std::string_view name, bool must_true, bool allow_implicit = true);
#ifdef ASST_DEBUG
        bool syntax_check(std::string_view task_name, const json::value& task_json);
#endif
        TaskDerivedConstPtr get_raw(std::string_view name);

    public:
        virtual ~TaskData() override = default;
        virtual const std::unordered_set<std::string>& get_templ_required() const noexcept override;
        void clear_tasks();
        void set_task_base(const std::string_view task_name, std::string base_task_name);
        bool lazy_parse(const json::value& json);

        TaskPtr get(std::string_view name);
        template <typename TargetTaskInfoType>
        requires(std::derived_from<TargetTaskInfoType, TaskInfo> &&
                 !std::same_as<TargetTaskInfoType, TaskInfo>) // Parameter must be a TaskInfo
        std::shared_ptr<TargetTaskInfoType> get(std::string_view name)
        {
            // TODO: should be const
            // any `Task.get(name)->x = y` could be transformed to
            // ```
            // json::object json = {};
            // json[name][x] = y;
            // Task.lazy_parse(json);
            // ```
            return std::dynamic_pointer_cast<TargetTaskInfoType>(get(name));
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

        std::unordered_set<std::string> m_templ_required;
        std::unordered_map<std::string_view, TaskStatus> m_task_status;
        std::unordered_map<std::string_view, json::object> m_json_all_tasks_info;  // 原始的 json 信息
        std::unordered_map<std::string_view, TaskDerivedPtr> m_raw_all_tasks_info; // 未展开虚任务的任务信息
        std::unordered_map<std::string_view, TaskPtr> m_all_tasks_info;            // 已展开虚任务的任务信息
    };

    inline static auto& Task = TaskData::get_instance();
} // namespace asst
