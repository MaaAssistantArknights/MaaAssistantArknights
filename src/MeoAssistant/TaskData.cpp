#include "TaskData.h"

#include <algorithm>
#include "AsstRanges.hpp"

#include <meojson/json.hpp>

#include "AsstTypes.h"
#include "GeneralConfiger.h"
#include "TemplResource.h"
#include "Logger.hpp"

const std::unordered_set<std::string>& asst::TaskData::get_templ_required() const noexcept
{
    return m_templ_required;
}

bool asst::TaskData::parse(const json::value& json)
{
    LogTraceFunction;

    auto to_lower = [](char c) -> char {
        return static_cast<char>(std::tolower(c));
    };
    for (const auto& [name, task_json] : json.as_object()) {
        std::string algorithm_str = task_json.get("algorithm", "matchtemplate");
        ranges::transform(algorithm_str, algorithm_str.begin(), to_lower);
        auto algorithm = AlgorithmType::Invalid;
        if (algorithm_str == "matchtemplate") {
            algorithm = AlgorithmType::MatchTemplate;
        }
        else if (algorithm_str == "justreturn") {
            algorithm = AlgorithmType::JustReturn;
        }
        else if (algorithm_str == "ocrdetect") {
            algorithm = AlgorithmType::OcrDetect;
        }
        else if (algorithm_str == "hash") {
            algorithm = AlgorithmType::Hash;
        }
        else {
            m_last_error = "algorithm error " + algorithm_str;
            return false;
        }

        std::shared_ptr<TaskInfo> task_info_ptr = nullptr;
        switch (algorithm) {
        case AlgorithmType::JustReturn:
            task_info_ptr = std::make_shared<TaskInfo>();
            break;
        case AlgorithmType::MatchTemplate: {
            auto match_task_info_ptr = std::make_shared<MatchTaskInfo>();
            match_task_info_ptr->templ_name = task_json.get("template", name + ".png");
            m_templ_required.emplace(match_task_info_ptr->templ_name);

            match_task_info_ptr->templ_threshold = task_json.get(
                "templThreshold", TemplThresholdDefault);
            match_task_info_ptr->special_threshold = task_json.get(
                "specialThreshold", 0);
            if (auto opt = task_json.find<json::array>("maskRange")) {
                auto& mask_range = *opt;
                match_task_info_ptr->mask_range = std::make_pair(static_cast<int>(mask_range[0]), static_cast<int>(mask_range[1]));
            }

            task_info_ptr = match_task_info_ptr;
        } break;
        case AlgorithmType::OcrDetect: {
            auto ocr_task_info_ptr = std::make_shared<OcrTaskInfo>();
            for (const json::value& text : task_json.at("text").as_array()) {
                ocr_task_info_ptr->text.emplace_back(text.as_string());
            }
            ocr_task_info_ptr->full_match = task_json.get("fullMatch", false);
            if (auto opt = task_json.find<json::array>("ocrReplace")) {
                for (const json::value& rep : opt.value()) {
                    ocr_task_info_ptr->replace_map.emplace(rep[0].as_string(), rep[1].as_string());
                }
            }
            task_info_ptr = ocr_task_info_ptr;
        } break;
        case AlgorithmType::Hash:
        {
            auto hash_task_info_ptr = std::make_shared<HashTaskInfo>();
            for (const json::value& hash : task_json.at("hash").as_array()) {
                hash_task_info_ptr->hashes.emplace_back(hash.as_string());
            }
            hash_task_info_ptr->dist_threshold = task_json.get("threshold", 0);

            if (auto opt = task_json.find<json::array>("maskRange")) {
                auto& mask_range = *opt;
                hash_task_info_ptr->mask_range = std::make_pair(static_cast<int>(mask_range[0]), static_cast<int>(mask_range[1]));
            }
            hash_task_info_ptr->bound = task_json.get("bound", true);

            task_info_ptr = hash_task_info_ptr;
        } break;
        }
        task_info_ptr->cache = task_json.get("cache", true);
        task_info_ptr->algorithm = algorithm;
        task_info_ptr->name = name;
        std::string action = task_json.get("action", "donothing");
        ranges::transform(action, action.begin(), to_lower);
        if (action == "clickself") {
            task_info_ptr->action = ProcessTaskAction::ClickSelf;
        }
        else if (action == "clickrand") {
            task_info_ptr->action = ProcessTaskAction::ClickRand;
        }
        else if (action == "donothing" || action.empty()) {
            task_info_ptr->action = ProcessTaskAction::DoNothing;
        }
        else if (action == "stop") {
            task_info_ptr->action = ProcessTaskAction::Stop;
        }
        else if (action == "clickrect") {
            task_info_ptr->action = ProcessTaskAction::ClickRect;
        }
        else if (action == "swipetotheleft") {
            task_info_ptr->action = ProcessTaskAction::SwipeToTheLeft;
        }
        else if (action == "swipetotheright") {
            task_info_ptr->action = ProcessTaskAction::SwipeToTheRight;
        }
        else if (action == "slowlyswipetotheleft") {
            task_info_ptr->action = ProcessTaskAction::SlowlySwipeToTheLeft;
        }
        else if (action == "slowlyswipetotheright") {
            task_info_ptr->action = ProcessTaskAction::SlowlySwipeToTheRight;
        }
        else {
            m_last_error = "Task: " + name + " error: " + action;
            return false;
        }

        task_info_ptr->max_times = task_json.get("maxTimes", INT_MAX);
        if (auto opt = task_json.find<json::array>("exceededNext")) {
            for (const json::value& exceed_next : opt.value()) {
                task_info_ptr->exceeded_next.emplace_back(exceed_next.as_string());
            }
        }
        if (auto opt = task_json.find<json::array>("onErrorNext")) {
            for (const json::value& on_error_next : opt.value()) {
                task_info_ptr->on_error_next.emplace_back(on_error_next.as_string());
            }
        }
        task_info_ptr->pre_delay = task_json.get("preDelay", 0);
        task_info_ptr->rear_delay = task_json.get("rearDelay", 0);
        if (auto opt = task_json.find<json::array>("reduceOtherTimes")) {
            for (const json::value& reduce : opt.value()) {
                task_info_ptr->reduce_other_times.emplace_back(reduce.as_string());
            }
        }
        if (auto opt = task_json.find<json::array>("roi")) {
            auto& roi_arr = *opt;
            int x = static_cast<int>(roi_arr[0]);
            int y = static_cast<int>(roi_arr[1]);
            int width = static_cast<int>(roi_arr[2]);
            int height = static_cast<int>(roi_arr[3]);
#ifdef ASST_DEBUG
            if (x + width > WindowWidthDefault || y + height > WindowHeightDefault) {
                m_last_error = name + " roi is out of bounds";
                return false;
            }
#endif
            task_info_ptr->roi = Rect(x, y, width, height);
        }
        else {
            task_info_ptr->roi = Rect();
        }

        if (auto opt = task_json.find<json::array>("sub")) {
            for (const json::value& sub : opt.value()) {
                task_info_ptr->sub.emplace_back(sub.as_string());
            }
        }
        task_info_ptr->sub_error_ignored = task_json.get("subErrorIgnored", false);

        if (auto opt = task_json.find<json::array>("next")) {
            for (const json::value& next : opt.value()) {
                task_info_ptr->next.emplace_back(next.as_string());
            }
        }

        if (auto opt = task_json.find<json::array>("rectMove")) {
            auto& move_arr = opt.value();
            task_info_ptr->rect_move = Rect(
                move_arr[0].as_integer(),
                move_arr[1].as_integer(),
                move_arr[2].as_integer(),
                move_arr[3].as_integer());
        }
        else {
            task_info_ptr->rect_move = Rect();
        }

        if (auto opt = task_json.find<json::array>("specificRect")) {
            auto& rect_arr = opt.value();
            task_info_ptr->specific_rect = Rect(
                rect_arr[0].as_integer(),
                rect_arr[1].as_integer(),
                rect_arr[2].as_integer(),
                rect_arr[3].as_integer());
        }
        else {
            task_info_ptr->specific_rect = Rect();
        }

        m_all_tasks_info[name] = task_info_ptr;
    }
#ifdef ASST_DEBUG
    for (const auto& [name, task] : m_all_tasks_info) {
        for (const auto& next : task->next) {
            if (m_all_tasks_info.find(next) == m_all_tasks_info.cend()) {
                m_last_error = name + "'s next " + next + " is null";
                return false;
            }
        }
    }
#endif
    return true;
}
