#include "TaskData.h"

#include <algorithm>
#include <meojson/json.hpp>

#include "Resource/GeneralConfiger.h"
#include "Resource/TemplResource.h"
#include "Utils/AsstRanges.hpp"
#include "Utils/AsstTypes.h"
#include "Utils/Logger.hpp"

const std::unordered_set<std::string>& asst::TaskData::get_templ_required() const noexcept
{
    return m_templ_required;
}

bool asst::TaskData::parse(const json::value& json)
{
    LogTraceFunction;

    auto to_lower = [](char c) -> char { return static_cast<char>(std::tolower(c)); };
#ifdef ASST_DEBUG
    {
        bool validity = true;
        for (const auto& [name, task_json] : json.as_object()) {
            validity &= syntax_check(name, task_json);
        }
        if (!validity) return false;
    }
#endif
    for (const auto& [name, task_json] : json.as_object()) {
        std::string algorithm_str = task_json.get("algorithm", "matchtemplate");
        ranges::transform(algorithm_str, algorithm_str.begin(), to_lower);
        auto algorithm = get_algorithm_type(algorithm_str);
        if (algorithm == AlgorithmType::Invalid) [[unlikely]] {
            Log.error("Unknown algorithm", algorithm_str);
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

            match_task_info_ptr->templ_threshold = task_json.get("templThreshold", TemplThresholdDefault);
            match_task_info_ptr->special_threshold = task_json.get("specialThreshold", 0);
            if (auto opt = task_json.find<json::array>("maskRange")) {
                auto& mask_range = *opt;
                match_task_info_ptr->mask_range =
                    std::make_pair(static_cast<int>(mask_range[0]), static_cast<int>(mask_range[1]));
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
        case AlgorithmType::Hash: {
            auto hash_task_info_ptr = std::make_shared<HashTaskInfo>();
            for (const json::value& hash : task_json.at("hash").as_array()) {
                hash_task_info_ptr->hashes.emplace_back(hash.as_string());
            }
            hash_task_info_ptr->dist_threshold = task_json.get("threshold", 0);

            if (auto opt = task_json.find<json::array>("maskRange")) {
                auto& mask_range = *opt;
                hash_task_info_ptr->mask_range =
                    std::make_pair(static_cast<int>(mask_range[0]), static_cast<int>(mask_range[1]));
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
        task_info_ptr->action = get_action_type(action);
        if (task_info_ptr->action == ProcessTaskAction::Invalid) [[unlikely]] {
            Log.error("Unknown action:", action, ", Task:", name);
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
                Log.error(name, "roi is out of bounds");
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
            task_info_ptr->rect_move = Rect(move_arr[0].as_integer(), move_arr[1].as_integer(),
                                            move_arr[2].as_integer(), move_arr[3].as_integer());
        }
        else {
            task_info_ptr->rect_move = Rect();
        }

        if (auto opt = task_json.find<json::array>("specificRect")) {
            auto& rect_arr = opt.value();
            task_info_ptr->specific_rect = Rect(rect_arr[0].as_integer(), rect_arr[1].as_integer(),
                                                rect_arr[2].as_integer(), rect_arr[3].as_integer());
        }
        else {
            task_info_ptr->specific_rect = Rect();
        }

        m_all_tasks_info[name] = task_info_ptr;
    }
#ifdef ASST_DEBUG
    for (const auto& [name, task] : m_all_tasks_info) {
        for (const auto& next : task->next) {
            if (get(next) == nullptr) {
                Log.error(name, "'s next", next, "is null");
                return false;
            }
        }
    }
#endif
    return true;
}

#ifdef ASST_DEBUG
// 为了解决类似 beddc7c828126c678391e0b4da288db6d2c2d58a 导致的问题，加载的时候做一个语法检查
// 主要是处理是否包含未知键值的问题
bool asst::TaskData::syntax_check(std::string_view task_name, const json::value& task_json)
{
    static const std::unordered_map<AlgorithmType, std::unordered_set<std::string>> allowed_key_under_algorithm = {
        { AlgorithmType::Invalid,
          {
              "algorithm",
              "template",
              "text",
              "action",
              "sub",
              "subErrorIgnored",
              "next",
              "maxTimes",
              "exceededNext",
              "onErrorNext",
              "preDelay",
              "rearDelay",
              "roi",
              "cache",
              "rectMove",
              "reduceOtherTimes",
              "templThreshold",
              "maskRange",
              "fullMatch",
              "ocrReplace",
              "hash",
              "specialThreshold",
              "threshold",
          } },
        { AlgorithmType::MatchTemplate,
          {
              "algorithm",
              "template",
              "action",
              "sub",
              "subErrorIgnored",
              "next",
              "maxTimes",
              "exceededNext",
              "onErrorNext",
              "preDelay",
              "rearDelay",
              "roi",
              "cache",
              "rectMove",
              "reduceOtherTimes",

              "templThreshold",
              "maskRange",
          } },
        { AlgorithmType::OcrDetect,
          {
              "algorithm",
              "text",
              "action",
              "sub",
              "subErrorIgnored",
              "next",
              "maxTimes",
              "exceededNext",
              "onErrorNext",
              "preDelay",
              "rearDelay",
              "roi",
              "cache",
              "rectMove",
              "reduceOtherTimes",

              "fullMatch",
              "ocrReplace",
          } },
        { AlgorithmType::JustReturn,
          {
              "algorithm",
              "action",
              "sub",
              "subErrorIgnored",
              "next",
              "maxTimes",
              "exceededNext",
              "onErrorNext",
              "preDelay",
              "rearDelay",
              "reduceOtherTimes",
          } },
        { AlgorithmType::Hash,
          {
              "algorithm",
              "action",
              "sub",
              "subErrorIgnored",
              "next",
              "maxTimes",
              "exceededNext",
              "onErrorNext",
              "preDelay",
              "rearDelay",
              "roi",
              "cache",
              "rectMove",
              "reduceOtherTimes",

              "hash",
              "maskRange",
              "specialThreshold",
              "threshold",
          } },
    };

    static const std::unordered_map<ProcessTaskAction, std::unordered_set<std::string>> allowed_key_under_action = {
        { ProcessTaskAction::ClickRect,
          {
              "specificRect",
          } },
    };

    auto is_doc = [&](std::string_view key) {
        return key.find("Doc") != std::string_view::npos || key.find("doc") != std::string_view::npos;
    };

    // 兜底策略，如果某个 key ("xxx") 不符合规范（可能是代码中使用到的参数，而不是任务流程）
    // 需要加一个注释 ("xxx_Doc") 就能过 syntax_check.
    auto has_doc = [&](const std::string& key) -> bool {
        return task_json.find(key + "_Doc") || task_json.find(key + "_doc");
    };

    bool validity = true;

    // 获取 algorithm
    std::string algorithm_str = task_json.get("algorithm", "matchtemplate");
    auto to_lower = [](char c) -> char { return static_cast<char>(std::tolower(c)); };
    ranges::transform(algorithm_str, algorithm_str.begin(), to_lower);
    auto algorithm = get_algorithm_type(algorithm_str);
    if (algorithm == AlgorithmType::Invalid) [[unlikely]] {
        Log.error(task_name, "has unknown algorithm:", algorithm_str);
        validity = false;
    }

    std::string action_str = task_json.get("action", "donothing");
    ranges::transform(action_str, action_str.begin(), to_lower);
    auto action = get_action_type(action_str);
    if (action == ProcessTaskAction::Invalid) [[unlikely]] {
        Log.error(task_name, "has unknown action:", algorithm_str);
        validity = false;
    }

    std::unordered_set<std::string> allowed_key {};
    if (allowed_key_under_algorithm.contains(algorithm)) {
        decltype(allowed_key) tmp = allowed_key_under_algorithm.at(algorithm);
        allowed_key.merge(tmp);
    }
    if (allowed_key_under_action.contains(action)) {
        decltype(allowed_key) tmp = allowed_key_under_action.at(action);
        allowed_key.merge(tmp);
    }

    // TODO: 之后也许还要对 key-value 联合检查，json 先留着
    for (const auto& [name, json] : task_json.as_object()) {
        if (!allowed_key.contains(name) && !is_doc(name) && !has_doc(name)) {
            Log.error(task_name, "has unknown key:", name);
            validity = false;
        }
    }
    return validity;
}
#endif
