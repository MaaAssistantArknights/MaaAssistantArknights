#include "TaskData.h"

#include <algorithm>
#include <meojson/json.hpp>

#include "Resource/GeneralConfiger.h"
#include "Resource/TemplResource.h"
#include "Utils/AsstRanges.hpp"
#include "Utils/AsstTypes.h"
#include "Utils/AsstUtils.hpp"
#include "Utils/Logger.hpp"

const std::unordered_set<std::string>& asst::TaskData::get_templ_required() const noexcept
{
    return m_templ_required;
}

bool asst::TaskData::parse(const json::value& json)
{
    LogTraceFunction;

#ifdef ASST_DEBUG
    // 语法检查
    {
        bool validity = true;
        for (const auto& [name, task_json] : json.as_object()) {
            validity &= syntax_check(name, task_json);
        }
        if (!validity) return false;
    }
#endif
    // <任务名, 模板任务标志位置, 任务参数ref>
    std::vector<std::tuple<std::string, size_t, std::reference_wrapper<const json::value>>> template_task_info {};

    for (const auto& [name, task_json] : json.as_object()) {
        // 模板任务特化需要等模板任务生成后才能生成
        if (size_t p = name.find('@'); p != std::string::npos) {
            template_task_info.emplace_back(name, p, task_json);
            continue;
        }

        std::shared_ptr<TaskInfo> task_info_ptr = generate_task_info(name, task_json, nullptr);
        if (task_info_ptr == nullptr) {
            return false;
        }
        m_all_tasks_info[name] = task_info_ptr;
    }

    // 短任务在前，因为模板任务特化一定比模板任务长
    ranges::sort(template_task_info,
                 [](const auto& l, const auto& r) { return std::get<0>(l).length() < std::get<0>(r).length(); });

    for (const auto& [name, pos, task_json] : template_task_info) {
        // 可能某个名字里带@的任务不是模板任务特化，只是普通的基类任务，这里最后一个参数可能为 nullptr
        auto task_info_ptr = generate_task_info(name, task_json, get(name.substr(pos + 1)), name.substr(0, pos));
        if (task_info_ptr == nullptr) {
            return false;
        }
        m_all_tasks_info[name] = task_info_ptr;
    }

#ifdef ASST_DEBUG
    bool some_next_null = false;
    for (const auto& [name, task] : m_all_tasks_info) {
        for (const auto& next : task->next) {
            if (get(next) == nullptr) {
                Log.error(name, "'s next", next, "is null");
                some_next_null = true;
            }
        }
    }
    if (some_next_null) {
        return false;
    }
#endif
    return true;
}

std::shared_ptr<asst::TaskInfo> asst::TaskData::generate_task_info(const std::string& name,
                                                                   const json::value& task_json,
                                                                   std::shared_ptr<TaskInfo> default_ptr,
                                                                   const std::string& task_prefix)
{
    if (default_ptr == nullptr) {
        default_ptr = default_task_info_ptr;
    }

    // 获取 algorithm 并按照 algorithm 生成 TaskInfo
    auto algorithm = default_ptr->algorithm;
    std::shared_ptr<TaskInfo> default_derived_ptr = default_ptr;
    if (auto opt = task_json.find<std::string>("algorithm")) {
        std::string algorithm_str = opt.value();
        utils::tolowers(algorithm_str);
        algorithm = get_algorithm_type(algorithm_str);
        if (default_ptr->algorithm != algorithm) {
            // 相同 algorithm 时才继承派生类成员
            default_derived_ptr = nullptr;
        }
    }
    std::shared_ptr<TaskInfo> task_info_ptr = nullptr;
    switch (algorithm) {
    case AlgorithmType::MatchTemplate:
        task_info_ptr =
            generate_match_task_info(name, task_json, std::dynamic_pointer_cast<MatchTaskInfo>(default_derived_ptr));
        break;
    case AlgorithmType::OcrDetect:
        task_info_ptr =
            generate_ocr_task_info(name, task_json, std::dynamic_pointer_cast<OcrTaskInfo>(default_derived_ptr));
        break;
    case AlgorithmType::Hash:
        task_info_ptr =
            generate_hash_task_info(name, task_json, std::dynamic_pointer_cast<HashTaskInfo>(default_derived_ptr));
        break;
    case AlgorithmType::JustReturn:
        task_info_ptr = std::make_shared<TaskInfo>();
        break;
    default:
        Log.error("Unknown algorithm in task", name);
        return nullptr;
    }

    // 不管什么algorithm，都有基础成员（next, roi, 等等）
    if (!append_base_task_info(task_info_ptr, name, task_json, default_ptr, task_prefix)) {
        return nullptr;
    }
    task_info_ptr->algorithm = algorithm;
    task_info_ptr->name = name;
    return task_info_ptr;
}

std::shared_ptr<asst::TaskInfo> asst::TaskData::generate_match_task_info(const std::string& name,
                                                                         const json::value& task_json,
                                                                         std::shared_ptr<MatchTaskInfo> default_ptr)
{
    if (default_ptr == nullptr) {
        default_ptr = default_match_task_info_ptr;
    }
    auto match_task_info_ptr = std::make_shared<MatchTaskInfo>();
    // template 留空时不从模板任务继承
    match_task_info_ptr->templ_name = task_json.get("template", name + ".png");
    m_templ_required.emplace(match_task_info_ptr->templ_name);

    // 其余若留空则继承模板任务
    match_task_info_ptr->templ_threshold = task_json.get("templThreshold", default_ptr->templ_threshold);
    match_task_info_ptr->special_threshold = task_json.get("specialThreshold", default_ptr->special_threshold);
    if (auto opt = task_json.find<json::array>("maskRange")) {
        auto& mask_range = *opt;
        match_task_info_ptr->mask_range =
            std::make_pair(static_cast<int>(mask_range[0]), static_cast<int>(mask_range[1]));
    }
    else {
        match_task_info_ptr->mask_range = default_ptr->mask_range;
    }
    return match_task_info_ptr;
}

std::shared_ptr<asst::TaskInfo> asst::TaskData::generate_ocr_task_info([[maybe_unused]] const std::string& name,
                                                                       const json::value& task_json,
                                                                       std::shared_ptr<OcrTaskInfo> default_ptr)
{
    if (default_ptr == nullptr) {
        default_ptr = default_ocr_task_info_ptr;
    }
    auto ocr_task_info_ptr = std::make_shared<OcrTaskInfo>();

    if (auto opt = task_json.find<json::array>("text")) {
        for (const json::value& text : opt.value()) {
            ocr_task_info_ptr->text.emplace_back(text.as_string());
        }
    }
    else {
#ifdef ASST_DEBUG
        if (default_ptr->text.empty()) {
            Log.warn("Ocr task", name, "has implicit empty text.");
        }
#endif
        ocr_task_info_ptr->text = default_ptr->text;
    }

    ocr_task_info_ptr->full_match = task_json.get("fullMatch", default_ptr->full_match);
    if (auto opt = task_json.find<json::array>("ocrReplace")) {
        for (const json::value& rep : opt.value()) {
            ocr_task_info_ptr->replace_map.emplace(rep[0].as_string(), rep[1].as_string());
        }
    }
    else {
        ocr_task_info_ptr->replace_map = default_ptr->replace_map;
    }
    return ocr_task_info_ptr;
}

std::shared_ptr<asst::TaskInfo> asst::TaskData::generate_hash_task_info([[maybe_unused]] const std::string& name,
                                                                        const json::value& task_json,
                                                                        std::shared_ptr<HashTaskInfo> default_ptr)
{
    if (default_ptr == nullptr) {
        default_ptr = default_hash_task_info_ptr;
    }
    auto hash_task_info_ptr = std::make_shared<HashTaskInfo>();
    if (auto opt = task_json.find<json::array>("hash")) {
        for (const json::value& hash : opt.value()) {
            hash_task_info_ptr->hashes.emplace_back(hash.as_string());
        }
    }
    else {
#ifdef ASST_DEBUG
        if (default_ptr->hashes.empty()) {
            Log.warn("Hash task", name, "has implicit empty hashes.");
        }
#endif
        hash_task_info_ptr->hashes = default_ptr->hashes;
    }

    hash_task_info_ptr->dist_threshold = task_json.get("threshold", default_ptr->dist_threshold);

    if (auto opt = task_json.find<json::array>("maskRange")) {
        auto& mask_range = *opt;
        hash_task_info_ptr->mask_range =
            std::make_pair(static_cast<int>(mask_range[0]), static_cast<int>(mask_range[1]));
    }
    else {
        hash_task_info_ptr->mask_range = default_ptr->mask_range;
    }
    hash_task_info_ptr->bound = task_json.get("bound", default_ptr->bound);

    return hash_task_info_ptr;
}

bool asst::TaskData::append_base_task_info(std::shared_ptr<TaskInfo> task_info_ptr, const std::string& name,
                                           const json::value& task_json, std::shared_ptr<TaskInfo> default_ptr,
                                           std::string task_prefix)
{
    if (default_ptr == nullptr) {
        default_ptr = default_task_info_ptr;
    }
    if (auto opt = task_json.find<std::string>("action")) {
        std::string action = opt.value();
        utils::tolowers(action);
        task_info_ptr->action = get_action_type(action);
        if (task_info_ptr->action == ProcessTaskAction::Invalid) [[unlikely]] {
            Log.error("Unknown action:", action, ", Task:", name);
            return false;
        }
    }
    else {
        task_info_ptr->action = default_ptr->action;
    }

    task_info_ptr->cache = task_json.get("cache", default_ptr->cache);
    task_info_ptr->max_times = task_json.get("maxTimes", default_ptr->max_times);
    if (auto opt = task_json.find<json::array>("exceededNext")) {
        for (const json::value& exceed_next : opt.value()) {
            task_info_ptr->exceeded_next.emplace_back(exceed_next.as_string());
        }
    }
    else {
        task_info_ptr->exceeded_next = append_prefix(default_ptr->exceeded_next, task_prefix);
    }
    if (auto opt = task_json.find<json::array>("onErrorNext")) {
        for (const json::value& on_error_next : opt.value()) {
            task_info_ptr->on_error_next.emplace_back(on_error_next.as_string());
        }
    }
    else {
        task_info_ptr->on_error_next = append_prefix(default_ptr->on_error_next, task_prefix);
    }
    task_info_ptr->pre_delay = task_json.get("preDelay", default_ptr->pre_delay);
    task_info_ptr->rear_delay = task_json.get("rearDelay", default_ptr->rear_delay);
    if (auto opt = task_json.find<json::array>("reduceOtherTimes")) {
        for (const json::value& reduce : opt.value()) {
            task_info_ptr->reduce_other_times.emplace_back(reduce.as_string());
        }
    }
    else {
        task_info_ptr->reduce_other_times = append_prefix(default_ptr->reduce_other_times, task_prefix);
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
        task_info_ptr->roi = default_ptr->roi;
    }

    if (auto opt = task_json.find<json::array>("sub")) {
        for (const json::value& sub : opt.value()) {
            task_info_ptr->sub.emplace_back(sub.as_string());
        }
    }
    else {
        task_info_ptr->sub = append_prefix(default_ptr->sub, task_prefix);
    }
    task_info_ptr->sub_error_ignored = task_json.get("subErrorIgnored", default_ptr->sub_error_ignored);

    if (auto opt = task_json.find<json::array>("next")) {
        for (const json::value& next : opt.value()) {
            task_info_ptr->next.emplace_back(next.as_string());
        }
    }
    else {
        task_info_ptr->next = append_prefix(default_ptr->next, task_prefix);
    }

    if (auto opt = task_json.find<json::array>("rectMove")) {
        auto& move_arr = opt.value();
        task_info_ptr->rect_move = Rect(move_arr[0].as_integer(), move_arr[1].as_integer(), move_arr[2].as_integer(),
                                        move_arr[3].as_integer());
    }
    else {
        task_info_ptr->rect_move = default_ptr->rect_move;
    }

    if (auto opt = task_json.find<json::array>("specificRect")) {
        auto& rect_arr = opt.value();
        task_info_ptr->specific_rect = Rect(rect_arr[0].as_integer(), rect_arr[1].as_integer(),
                                            rect_arr[2].as_integer(), rect_arr[3].as_integer());
    }
    else {
        task_info_ptr->specific_rect = default_ptr->specific_rect;
    }
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
