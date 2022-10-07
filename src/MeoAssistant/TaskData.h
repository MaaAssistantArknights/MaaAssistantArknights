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
        std::shared_ptr<TaskInfo> generate_task_info(const std::shared_ptr<TaskInfo>& base_ptr,
                                                     const std::string& task_prefix) const
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
            using tasklist_t = std::vector<std::string>;
            auto generate_tasks = [&](tasklist_t& task_list, const tasklist_t& base_task_list) {
                task_list = {};
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
                        task_list.emplace_back(task_prefix + "@" + base);
                    }
                }
            };

            generate_tasks(task_info_ptr->next, base_ptr->next);
            generate_tasks(task_info_ptr->exceeded_next, base_ptr->exceeded_next);
            generate_tasks(task_info_ptr->on_error_next, base_ptr->on_error_next);
            generate_tasks(task_info_ptr->reduce_other_times, base_ptr->reduce_other_times);

            return task_info_ptr;
        }

    public:
        virtual ~TaskData() override = default;
        virtual const std::unordered_set<std::string>& get_templ_required() const noexcept override;

        template <typename TargetTaskInfoType = TaskInfo>
        requires(std::derived_from<TargetTaskInfoType, TaskInfo> ||
                 std::same_as<TargetTaskInfoType, TaskInfo>) // Parameter must be a TaskInfo
        std::shared_ptr<TargetTaskInfoType> get(const std::string& name)
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
            if (at_pos == std::string::npos) [[unlikely]] {
                return nullptr;
            }

            // `@` 前面的字符长度
            size_t name_len = at_pos;
            auto base_task_iter = get(name.substr(name_len + 1));
            if (base_task_iter == nullptr) [[unlikely]] {
                return nullptr;
            }

            std::string derived_task_name = name.substr(0, name_len);
            auto task_info_ptr = generate_task_info(base_task_iter, derived_task_name);
            if (task_info_ptr == nullptr) [[unlikely]] {
                return nullptr;
            }

            // tasks 个数超过上限时不再 emplace，返回临时值
            constexpr size_t MAX_TASKS_SIZE = 65535;
            if (m_all_tasks_info.size() < MAX_TASKS_SIZE) {
                m_all_tasks_info.emplace(name, task_info_ptr);
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

    private:
        static AlgorithmType get_algorithm_type(std::string_view algorithm_str)
        {
            if (algorithm_str == "matchtemplate") {
                return AlgorithmType::MatchTemplate;
            }
            else if (algorithm_str == "justreturn") {
                return AlgorithmType::JustReturn;
            }
            else if (algorithm_str == "ocrdetect") {
                return AlgorithmType::OcrDetect;
            }
            else if (algorithm_str == "hash") {
                return AlgorithmType::Hash;
            }
            return AlgorithmType::Invalid;
        }

        static ProcessTaskAction get_action_type(std::string_view action_str)
        {
            if (action_str == "clickself") {
                return ProcessTaskAction::ClickSelf;
            }
            else if (action_str == "clickrand") {
                return ProcessTaskAction::ClickRand;
            }
            else if (action_str == "donothing" || action_str.empty()) {
                return ProcessTaskAction::DoNothing;
            }
            else if (action_str == "stop") {
                return ProcessTaskAction::Stop;
            }
            else if (action_str == "clickrect") {
                return ProcessTaskAction::ClickRect;
            }
            else if (action_str == "swipetotheleft") {
                return ProcessTaskAction::SwipeToTheLeft;
            }
            else if (action_str == "swipetotheright") {
                return ProcessTaskAction::SwipeToTheRight;
            }
            else if (action_str == "slowlyswipetotheleft") {
                return ProcessTaskAction::SlowlySwipeToTheLeft;
            }
            else if (action_str == "slowlyswipetotheright") {
                return ProcessTaskAction::SlowlySwipeToTheRight;
            }
            return ProcessTaskAction::Invalid;
        }

#ifdef ASST_DEBUG
        bool syntax_check(std::string_view task_name, const json::value& task_json);
#endif

    protected:
        std::unordered_map<std::string, std::shared_ptr<TaskInfo>> m_all_tasks_info;
        std::unordered_set<std::string> m_templ_required;
    };

    inline static auto& Task = TaskData::get_instance();
} // namespace asst
