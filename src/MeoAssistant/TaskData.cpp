#include "TaskData.h"

#include <algorithm>

#include <meojson/json.hpp>

#include "AsstTypes.h"
#include "GeneralConfiger.h"
#include "TemplResource.h"
#include "Logger.hpp"

const std::shared_ptr<asst::TaskInfo> asst::TaskData::get(const std::string& name) const noexcept
{
    if (auto iter = m_all_tasks_info.find(name);
        iter != m_all_tasks_info.cend()) {
        return iter->second;
    }
    else {
        return nullptr;
    }
}

const std::unordered_set<std::string>& asst::TaskData::get_templ_required() const noexcept
{
    return m_templ_required;
}

std::shared_ptr<asst::TaskInfo> asst::TaskData::get(const std::string& name)
{
    return m_all_tasks_info[name];
}

bool asst::TaskData::parse(const json::value& json)
{
    LogTraceFunction;

    auto to_lower = [](char c) -> char {
        return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
    };
    for (const auto& [name, task_json] : json.as_object()) {
        std::string algorithm_str = task_json.get("algorithm", "matchtemplate");
        std::transform(algorithm_str.begin(), algorithm_str.end(), algorithm_str.begin(), to_lower);
        AlgorithmType algorithm = AlgorithmType::Invalid;
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
            if (task_json.contains("maskRange")) {
                match_task_info_ptr->mask_range = std::make_pair(
                    task_json.at("maskRange")[0].as_integer(),
                    task_json.at("maskRange")[1].as_integer());
            }

            task_info_ptr = match_task_info_ptr;
        } break;
        case AlgorithmType::OcrDetect: {
            auto ocr_task_info_ptr = std::make_shared<OcrTaskInfo>();
            for (const json::value& text : task_json.at("text").as_array()) {
                ocr_task_info_ptr->text.emplace_back(text.as_string());
            }
            ocr_task_info_ptr->full_match = task_json.get("fullMatch", false);
            if (task_json.contains("ocrReplace")) {
                for (const json::value& rep : task_json.at("ocrReplace").as_array()) {
                    ocr_task_info_ptr->replace_map.emplace(rep.as_array()[0].as_string(), rep.as_array()[1].as_string());
                }
            }
            task_info_ptr = ocr_task_info_ptr;
        } break;
        case AlgorithmType::Hash:
        {
            auto hash_task_info_ptr = std::make_shared<HashTaskInfo>();
            for (const json::value& hash : task_json.at("hash").as_array()) {
                hash_task_info_ptr->hashs.emplace_back(hash.as_string());
            }
            hash_task_info_ptr->dist_threshold = task_json.get("threshold", 0);
            if (task_json.contains("maskRange")) {
                hash_task_info_ptr->mask_range = std::make_pair(
                    task_json.at("maskRange")[0].as_integer(),
                    task_json.at("maskRange")[1].as_integer());
            }
            hash_task_info_ptr->bound = task_json.get("bound", true);

            task_info_ptr = hash_task_info_ptr;
        } break;
        }
        task_info_ptr->cache = task_json.get("cache", true);
        task_info_ptr->algorithm = algorithm;
        task_info_ptr->name = name;
        std::string action = task_json.get("action", "donothing");
        std::transform(action.begin(), action.end(), action.begin(), to_lower);
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
            const json::value& rect_json = task_json.at("specificRect");
            task_info_ptr->specific_rect = Rect(
                rect_json[0].as_integer(),
                rect_json[1].as_integer(),
                rect_json[2].as_integer(),
                rect_json[3].as_integer());
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
        if (task_json.contains("exceededNext")) {
            const json::array& excceed_next_arr = task_json.at("exceededNext").as_array();
            for (const json::value& excceed_next : excceed_next_arr) {
                task_info_ptr->exceeded_next.emplace_back(excceed_next.as_string());
            }
        }
        else {
            task_info_ptr->exceeded_next.emplace_back("Stop");
        }
        task_info_ptr->pre_delay = task_json.get("preDelay", 0);
        task_info_ptr->rear_delay = task_json.get("rearDelay", 0);
        if (task_json.contains("reduceOtherTimes")) {
            const json::array& reduce_arr = task_json.at("reduceOtherTimes").as_array();
            for (const json::value& reduce : reduce_arr) {
                task_info_ptr->reduce_other_times.emplace_back(reduce.as_string());
            }
        }
        if (task_json.contains("roi")) {
            const json::array& area_arr = task_json.at("roi").as_array();
            int x = area_arr[0].as_integer();
            int y = area_arr[1].as_integer();
            int width = area_arr[2].as_integer();
            int height = area_arr[3].as_integer();
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

        if (task_json.contains("next")) {
            for (const json::value& next : task_json.at("next").as_array()) {
                task_info_ptr->next.emplace_back(next.as_string());
            }
        }
        if (task_json.contains("rectMove")) {
            const json::array& move_arr = task_json.at("rectMove").as_array();
            task_info_ptr->rect_move = Rect(
                move_arr[0].as_integer(),
                move_arr[1].as_integer(),
                move_arr[2].as_integer(),
                move_arr[3].as_integer());
        }
        else {
            task_info_ptr->rect_move = Rect();
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
