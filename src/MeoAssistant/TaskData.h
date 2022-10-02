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
    public:
        virtual ~TaskData() override = default;
        virtual const std::unordered_set<std::string>& get_templ_required() const noexcept override;

        template <typename TargetTaskInfoType>
        requires std::derived_from<TargetTaskInfoType, TaskInfo> &&
                 (!std::same_as<TargetTaskInfoType, TaskInfo>) // Parameter must be a TaskInfo and not same as TaskInfo
        std::shared_ptr<TargetTaskInfoType> get(const std::string& name)
        {
            auto it = m_all_tasks_info.find(name);
            if (it == m_all_tasks_info.cend()) {
                return nullptr;
            }

            return std::dynamic_pointer_cast<TargetTaskInfoType>(it->second);
        }

        template <typename TargetTaskInfoType = TaskInfo>
        requires std::same_as<TargetTaskInfoType, TaskInfo> // Parameter must be a TaskInfo
        std::shared_ptr<TargetTaskInfoType> get(const std::string& name)
        {
            auto it = m_all_tasks_info.find(name);
            if (it == m_all_tasks_info.cend()) {
                return nullptr;
            }

            return it->second;
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
}
