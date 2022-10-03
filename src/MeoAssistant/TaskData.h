#pragma once

#include "Resource/AbstractConfigerWithTempl.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "Utils/AsstTypes.h"

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
            task_info_ptr->reduce_other_times = {};
            for (const std::string& reduce_other_times : base_ptr->reduce_other_times) {
                task_info_ptr->reduce_other_times.emplace_back(task_prefix + reduce_other_times);
            }
            task_info_ptr->on_error_next = {};
            for (const std::string& on_error_next : base_ptr->on_error_next) {
                task_info_ptr->on_error_next.emplace_back(task_prefix + on_error_next);
            }
            task_info_ptr->exceeded_next = {};
            for (const std::string& exceeded_next : base_ptr->exceeded_next) {
                task_info_ptr->exceeded_next.emplace_back(task_prefix + exceeded_next);
            }
            task_info_ptr->next = {};
            for (const std::string& next : base_ptr->next) {
                task_info_ptr->next.emplace_back(task_prefix + next);
            }
            return task_info_ptr;
        }

    public:
        virtual ~TaskData() override = default;
        virtual const std::unordered_set<std::string>& get_templ_required() const noexcept override;

        template <typename TargetTaskInfoType = TaskInfo>
        requires (std::derived_from<TargetTaskInfoType, TaskInfo> ||
                  std::same_as<TargetTaskInfoType, TaskInfo>) // Parameter must be a TaskInfo
        std::shared_ptr<TargetTaskInfoType> get(const std::string& name)
        {
            auto it = m_all_tasks_info.find(name);
            if (it == m_all_tasks_info.cend()) {
                if (size_t name_split_pos = name.find('@'); name_split_pos == std::string::npos) [[unlikely]] {
                    return nullptr;
                }
                else {
                    size_t name_len = name_split_pos + 1;
                    std::string base_task_name = name.substr(name_len);
                    std::string derived_task_name = name.substr(0, name_len);
                    if (auto base_task_iter = get(base_task_name); base_task_iter != nullptr) {
                        if (auto task_info_ptr = generate_task_info(base_task_iter, derived_task_name);
                            task_info_ptr != nullptr) {
                            m_all_tasks_info.emplace(name, task_info_ptr);
                            if constexpr (std::same_as<TargetTaskInfoType, TaskInfo>) {
                                return task_info_ptr;
                            }
                            else {
                                return std::dynamic_pointer_cast<TargetTaskInfoType>(task_info_ptr);
                            }
                        }
                    }
                }
                return nullptr;
            }

            if constexpr (std::same_as<TargetTaskInfoType, TaskInfo>) {
                return it->second;
            }
            else {
                return std::dynamic_pointer_cast<TargetTaskInfoType>(it->second);
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
