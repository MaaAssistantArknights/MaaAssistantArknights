#include "MaterialSynthesisTaskPlugin.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <unordered_set>

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/Infrast/InfrastProcessingTask.h"
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
    if (need_exit()) {
        return false;
    }
    report_status("MaterialSynthesisStart");

    if (!detect_task("MiniGame@MaterialSynthesis@Workshop")) {
        if (need_exit()) {
            return false;
        }
        Log.error("MaterialSynthesis | start from the material synthesis page");
        save_img(utils::path("debug") / utils::path("material_synthesis"), false);
        report_result(Result::NavigationFailed);
        return false;
    }

    // 使用独立任务实例，避免材料合成缓存和页面操作进入常规基建任务对象。
    InfrastProcessingTask processing_task(m_callback, m_inst, m_task_chain);
    processing_task.set_task_id(m_task_id);
    std::unordered_set<std::string> material_stack;
    int operation_budget = MaxMaterialOperations;
    bool operator_selection_initialized = false;
    const Result result =
        synthesize_material(0, material_stack, operation_budget, processing_task, operator_selection_initialized);
    Log.info("MaterialSynthesis | finished", result_name(result), "remaining operations", operation_budget);
    if (result != Result::Cancelled && !need_exit()) {
        if (result != Result::Completed) {
            save_img(utils::path("debug") / utils::path("material_synthesis"), false);
        }
        report_result(result);
    }
    return result == Result::Completed;
}

asst::MaterialSynthesisTaskPlugin::Result asst::MaterialSynthesisTaskPlugin::synthesize_material(
    int depth,
    std::unordered_set<std::string>& material_stack,
    int& operation_budget,
    InfrastProcessingTask& processing_task,
    bool& operator_selection_initialized)
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
    const auto material_level = ItemData.get_item_level(material_id);
    if (!material_level) {
        Log.warn("MaterialSynthesis | material level is not in item config", material_id);
        return Result::Unsupported;
    }
    if (!material_stack.emplace(material_id).second) {
        Log.warn("MaterialSynthesis | recipe cycle detected", material_id);
        return Result::InsufficientResources;
    }

    // 根配方由用户手动打开；递归进入的材料必然有父配方，只在完成时识别并点击返回。
    const bool has_parent = depth > 0;
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
        report_status(
            "MaterialSynthesisMaterial",
            json::object {
                { "material", *material_name },
                { "material_id", material_id },
                { "level", *material_level },
                { "depth", depth },
                { "count", *count },
            });

        // 三个下级材料都需要独立检查并补足，再加工当前配方。
        for (int ingredient = 1; ingredient <= 3; ++ingredient) {
            const std::string prefix = "MiniGame@MaterialSynthesis@Ingredient" + std::to_string(ingredient);
            // Available 任务在识别成功后直接点击对应的下级材料。
            if (run_task(prefix + "Available", 0)) {
                report_status(
                    "MaterialSynthesisIngredient",
                    json::object {
                        { "material", *material_name },
                        { "material_id", material_id },
                        { "depth", depth },
                        { "ingredient", ingredient },
                    });
                result = synthesize_material(
                    depth + 1,
                    material_stack,
                    operation_budget,
                    processing_task,
                    operator_selection_initialized);
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
                report_status(
                    "MaterialSynthesisIngredientUnavailable",
                    json::object {
                        { "material", *material_name },
                        { "material_id", material_id },
                        { "depth", depth },
                        { "ingredient", ingredient },
                    });
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

        // 无人进驻任务识别后直接点击空槽；已有干员时仍由独立任务点击相同位置。
        const bool operator_missing = run_task("MiniGame@MaterialSynthesis@NoOperator", 0);
        const bool mood_insufficient = !operator_missing && detect_task("MiniGame@MaterialSynthesis@LowMood");
        bool operator_changed = false;
        if (!operator_selection_initialized || operator_missing || mood_insufficient) {
            report_status(
                "MaterialSynthesisOperator",
                json::object {
                    { "material", *material_name },
                    { "material_id", material_id },
                    { "reason",
                      operator_missing    ? "missing"
                      : mood_insufficient ? "low_mood"
                                          : "initial" },
                });
            result = select_processing_operator(
                material_id,
                *material_level,
                operator_missing,
                operator_changed,
                processing_task);
            if (result != Result::Completed) {
                break;
            }
            operator_selection_initialized = true;
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
            result = Result::OperatorUnavailable;
            break;
        }

        report_status(
            "MaterialSynthesisCraft",
            json::object {
                { "material", *material_name },
                { "material_id", material_id },
                { "count", selected_count },
            });
        if (!run_task("MiniGame@MaterialSynthesis@Start") || !detect_task("MiniGame@MaterialSynthesis@Workshop")) {
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
    if (has_parent) {
        Log.info("MaterialSynthesis | return to parent recipe", depth);
        report_status(
            "MaterialSynthesisReturn",
            json::object {
                { "material", *first_material_name },
                { "material_id", material_id },
                { "depth", depth },
            });
        if (!run_task("MiniGame@MaterialSynthesis@HasParent") || !detect_task("MiniGame@MaterialSynthesis@Workshop")) {
            return Result::NavigationFailed;
        }
    }
    return Result::Completed;
}

asst::MaterialSynthesisTaskPlugin::Result asst::MaterialSynthesisTaskPlugin::select_processing_operator(
    const std::string& material_id,
    int material_level,
    bool operator_missing,
    bool& operator_changed,
    InfrastProcessingTask& processing_task)
{
    operator_changed = false;
    if (!operator_missing && !run_task("MiniGame@MaterialSynthesis@OpenOperatorList")) {
        return Result::NavigationFailed;
    }
    if (processing_task.select_operator(material_id, material_level, operator_changed)) {
        return Result::Completed;
    }
    if (need_exit()) {
        return Result::Cancelled;
    }
    if (!return_to_workshop()) {
        return Result::NavigationFailed;
    }
    return operator_missing ? Result::OperatorUnavailable : Result::Completed;
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

void asst::MaterialSynthesisTaskPlugin::report_status(std::string what, json::value details)
{
    auto info = basic_info_with_what(std::move(what));
    info["details"] = std::move(details);
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::MaterialSynthesisTaskPlugin::report_result(Result result)
{
    if (result == Result::Completed) {
        report_status("MaterialSynthesisCompleted");
        return;
    }
    report_status(
        "MaterialSynthesisFailed",
        json::object {
            { "result", std::string(result_name(result)) },
        });
}

std::string_view asst::MaterialSynthesisTaskPlugin::result_name(Result result)
{
    switch (result) {
    case Result::Completed:
        return "completed";
    case Result::InsufficientResources:
        return "insufficient_resources";
    case Result::OperatorUnavailable:
        return "operator_unavailable";
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
