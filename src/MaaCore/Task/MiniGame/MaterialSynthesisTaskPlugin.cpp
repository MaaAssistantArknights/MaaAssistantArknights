#include "MaterialSynthesisTaskPlugin.h"

#include "MaterialSynthesisOperatorTask.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <unordered_set>

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/RegionOCRer.h"

namespace
{
constexpr int MaxMaterialDepth = 8;
constexpr int MaxMaterialOperations = 64;
constexpr int MaxMaterialBatch = 24;

std::string trim_text(std::string text)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.erase(text.begin());
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.pop_back();
    }
    return text;
}
}

bool asst::MaterialSynthesisTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& task = details.get("details", "task", "");
    return task == "MiniGame@MaterialSynthesis@Begin";
}

bool asst::MaterialSynthesisTaskPlugin::_run()
{
    LogTraceFunction;

    if (!detect_task("MiniGame@MaterialSynthesis@Workshop")) {
        Log.error("MaterialSynthesis | start from the material synthesis page");
        return false;
    }

    MaterialSynthesisOperatorTask operator_task(m_callback, m_inst, m_task_chain);
    operator_task.set_task_id(m_task_id);
    std::unordered_set<std::string> material_stack;
    int operation_budget = MaxMaterialOperations;
    const Result result = synthesize_material(0, material_stack, operation_budget, operator_task);
    Log.info("MaterialSynthesis | finished", result_name(result), "remaining operations", operation_budget);
    return result == Result::Completed;
}

asst::MaterialSynthesisTaskPlugin::Result asst::MaterialSynthesisTaskPlugin::synthesize_material(
    int depth,
    std::unordered_set<std::string>& material_stack,
    int& operation_budget,
    MaterialSynthesisOperatorTask& operator_task)
{
    if (need_exit()) {
        return Result::Cancelled;
    }
    if (depth >= MaxMaterialDepth || operation_budget <= 0) {
        Log.warn("MaterialSynthesis | recursion limit reached", depth, operation_budget);
        return Result::InsufficientResources;
    }
    if (!detect_task("MiniGame@MaterialSynthesis@Workshop")) {
        return Result::NavigationFailed;
    }

    const auto first_material_name = read_text("MiniGame@MaterialSynthesis@MaterialName");
    if (!first_material_name) {
        return Result::Unsupported;
    }
    const std::string material_id = find_item_id(*first_material_name);
    if (material_id.empty()) {
        Log.warn("MaterialSynthesis | material is not in item config", *first_material_name);
        return Result::Unsupported;
    }
    if (!material_stack.emplace(material_id).second) {
        Log.warn("MaterialSynthesis | recipe cycle detected", material_id);
        return Result::InsufficientResources;
    }

    // 根配方由用户手动打开，即使页面识别异常也不能从根配方返回。
    const bool has_parent = depth > 0 && detect_task("MiniGame@MaterialSynthesis@HasParent");
    Result result = Result::Completed;
    while (!need_exit()) {
        if (detect_task("MiniGame@MaterialSynthesis@Satisfied")) {
            break;
        }
        if (operation_budget <= 0) {
            result = Result::InsufficientResources;
            break;
        }
        --operation_budget;
        if (!detect_task("MiniGame@MaterialSynthesis@Workshop")) {
            result = Result::NavigationFailed;
            break;
        }

        const auto material_name = read_text("MiniGame@MaterialSynthesis@MaterialName");
        const auto count = read_number("MiniGame@MaterialSynthesis@RequiredCount");
        if (!material_name || !count || *count <= 0 || *material_name != *first_material_name) {
            result = Result::Unsupported;
            break;
        }

        // 三个下级材料都需要独立检查并补足，再加工当前配方。
        for (int ingredient = 1; ingredient <= 3; ++ingredient) {
            const std::string prefix = "MiniGame@MaterialSynthesis@Ingredient" + std::to_string(ingredient);
            if (detect_task(prefix + "Available")) {
                if (!run_task(prefix + "Open")) {
                    result = Result::NavigationFailed;
                    break;
                }
                result = synthesize_material(depth + 1, material_stack, operation_budget, operator_task);
                if (result != Result::Completed) {
                    break;
                }
                const auto parent_material_name = read_text("MiniGame@MaterialSynthesis@MaterialName");
                if (!parent_material_name || *parent_material_name != *first_material_name) {
                    result = Result::NavigationFailed;
                    break;
                }
            }
            else if (detect_task(prefix + "Unavailable")) {
                Log.warn("MaterialSynthesis | ingredient unavailable", ingredient, material_id);
                result = Result::InsufficientResources;
                break;
            }
        }
        if (result != Result::Completed) {
            break;
        }

        const int batch_count = std::min(*count, MaxMaterialBatch);
        int selected_count = 1;
        for (; selected_count < batch_count; ++selected_count) {
            if (need_exit()) {
                result = Result::Cancelled;
                break;
            }
            if (!run_task("MiniGame@MaterialSynthesis@Increase", 0)) {
                result = Result::NavigationFailed;
                break;
            }
        }
        if (result != Result::Completed) {
            break;
        }

        const bool operator_missing = detect_task("MiniGame@MaterialSynthesis@NoOperator");
        const bool mood_insufficient = detect_task("MiniGame@MaterialSynthesis@LowMood");
        bool operator_changed = false;
        if (operator_missing || mood_insufficient) {
            result = select_processing_operator(
                material_id,
                item_level(material_id),
                operator_missing,
                operator_changed,
                operator_task);
            if (result != Result::Completed) {
                break;
            }
        }

        // 更换加工站干员后数量会重置为 1，需要为新干员重新逐步加量并及时避开心情不足。
        if (operator_changed) {
            selected_count = 1;
            while (selected_count < batch_count && !need_exit()) {
                if (!run_task("MiniGame@MaterialSynthesis@Increase", 0)) {
                    result = Result::NavigationFailed;
                    break;
                }
                ++selected_count;
                if (detect_task("MiniGame@MaterialSynthesis@LowMood")) {
                    if (!run_task("MiniGame@MaterialSynthesis@Decrease", 0)) {
                        result = Result::NavigationFailed;
                    }
                    else {
                        --selected_count;
                    }
                    break;
                }
            }
        }
        if (result != Result::Completed) {
            break;
        }

        // 没有可替换的满心情干员时，保留当前干员并将单批数量降到其心情可承受的范围。
        while (selected_count > 1 && detect_task("MiniGame@MaterialSynthesis@LowMood") && !need_exit()) {
            if (!run_task("MiniGame@MaterialSynthesis@Decrease", 0)) {
                result = Result::NavigationFailed;
                break;
            }
            --selected_count;
        }
        if (result != Result::Completed) {
            break;
        }
        if (need_exit()) {
            result = Result::Cancelled;
            break;
        }
        if (detect_task("MiniGame@MaterialSynthesis@LowMood")) {
            result = Result::InsufficientResources;
            break;
        }

        if (!run_task("MiniGame@MaterialSynthesis@Start") || !run_task("MiniGame@MaterialSynthesis@Obtain", 5) ||
            !detect_task("MiniGame@MaterialSynthesis@Workshop")) {
            result = Result::NavigationFailed;
            break;
        }
        if (detect_task("MiniGame@MaterialSynthesis@Satisfied")) {
            break;
        }

        // 单批小于总缺口时继续当前配方，并共享总操作预算。
        Log.info("MaterialSynthesis | continue current recipe", material_id, operation_budget);
    }

    material_stack.erase(material_id);
    if (need_exit()) {
        return Result::Cancelled;
    }
    if (result != Result::Completed) {
        return result;
    }
    if (has_parent && (!run_task("MiniGame@MaterialSynthesis@ReturnToParent") ||
                       !detect_task("MiniGame@MaterialSynthesis@Workshop"))) {
        return Result::NavigationFailed;
    }
    return Result::Completed;
}

asst::MaterialSynthesisTaskPlugin::Result asst::MaterialSynthesisTaskPlugin::select_processing_operator(
    const std::string& material_id,
    int material_level,
    bool operator_missing,
    bool& operator_changed,
    MaterialSynthesisOperatorTask& operator_task)
{
    operator_changed = false;
    if (!run_task("MiniGame@MaterialSynthesis@OpenOperatorList")) {
        return Result::NavigationFailed;
    }
    if (operator_task.select_operator(material_id, material_level)) {
        operator_changed = true;
        return Result::Completed;
    }
    if (need_exit()) {
        return Result::Cancelled;
    }
    if (!return_to_workshop()) {
        return Result::NavigationFailed;
    }
    return operator_missing ? Result::Unsupported : Result::Completed;
}

bool asst::MaterialSynthesisTaskPlugin::run_task(const std::string& task_name, int retry_times)
{
    ProcessTask task(*this, { task_name });
    task.set_retry_times(retry_times);
    return task.run();
}

bool asst::MaterialSynthesisTaskPlugin::detect_task(const std::string& task_name)
{
    return run_task(task_name, 0);
}

bool asst::MaterialSynthesisTaskPlugin::return_to_workshop()
{
    for (int attempt = 0; attempt < 3 && !need_exit(); ++attempt) {
        if (detect_task("MiniGame@MaterialSynthesis@Workshop")) {
            return true;
        }
        if (!run_task("MiniGame@MaterialSynthesis@ReturnFromOperatorList", 0)) {
            return false;
        }
    }
    return detect_task("MiniGame@MaterialSynthesis@Workshop");
}

std::optional<int> asst::MaterialSynthesisTaskPlugin::read_number(const std::string& task_name)
{
    RegionOCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info(task_name);
    analyzer.set_use_raw(true);
    if (!analyzer.analyze()) {
        return std::nullopt;
    }

    std::string digits;
    for (const unsigned char character : analyzer.get_result().text) {
        if (std::isdigit(character)) {
            digits.push_back(static_cast<char>(character));
        }
        else if (!digits.empty()) {
            break;
        }
    }
    if (digits.empty()) {
        return std::nullopt;
    }

    int value = 0;
    const auto [ptr, error] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (error != std::errc { } || ptr != digits.data() + digits.size()) {
        return std::nullopt;
    }
    return value;
}

std::optional<std::string> asst::MaterialSynthesisTaskPlugin::read_text(const std::string& task_name)
{
    RegionOCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info(task_name);
    analyzer.set_use_raw(true);
    if (!analyzer.analyze()) {
        return std::nullopt;
    }

    auto text = trim_text(analyzer.get_result().text);
    if (text.empty()) {
        return std::nullopt;
    }
    return text;
}

std::string asst::MaterialSynthesisTaskPlugin::find_item_id(const std::string& name) const
{
    for (const auto& id : ItemData.get_all_item_id()) {
        if (ItemData.get_item_name(id) == name) {
            return id;
        }
    }
    return { };
}

int asst::MaterialSynthesisTaskPlugin::item_level(const std::string& item_id)
{
    // 加工心情消耗并不等于物品 ID 的末位，芯片和部分材料链尤其如此。
    static const std::unordered_set<std::string_view> Level0 {
        "30011",
        "30021",
        "30041",
        "30061",
    };
    static const std::unordered_set<std::string_view> Level1 {
        "30012", "30022", "30031", "30042", "30051", "30062", "3301",
    };
    static const std::unordered_set<std::string_view> Level2 {
        "30013", "30023", "30032", "30043", "30052", "30063", "30093", "30103", "31013", "31023",
        "31033", "31043", "31053", "31063", "31073", "31083", "31093", "31103", "31113", "3211",
        "3221",  "3231",  "3241",  "3251",  "3261",  "3271",  "3281",  "3302",
    };
    static const std::unordered_set<std::string_view> Level4 {
        "30024", "30034", "30115", "30125", "30135", "30145", "30155", "30165",
        "3213",  "3223",  "3233",  "3243",  "3253",  "3263",  "3273",  "3283",
    };

    if (Level0.contains(item_id)) {
        return 0;
    }
    if (Level1.contains(item_id)) {
        return 1;
    }
    if (Level2.contains(item_id)) {
        return 2;
    }
    if (Level4.contains(item_id)) {
        return 4;
    }
    return 3;
}

std::string_view asst::MaterialSynthesisTaskPlugin::result_name(Result result)
{
    switch (result) {
    case Result::Completed:
        return "completed";
    case Result::InsufficientResources:
        return "insufficient_resources";
    case Result::Unsupported:
        return "unsupported";
    case Result::NavigationFailed:
        return "navigation_failed";
    case Result::Cancelled:
        return "cancelled";
    default:
        return "unknown";
    }
}
