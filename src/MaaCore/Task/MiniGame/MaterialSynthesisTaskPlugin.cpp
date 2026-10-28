#include "MaterialSynthesisTaskPlugin.h"

#include <algorithm>
#include <limits>
#include <unordered_set>

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/Infrast/InfrastProcessingTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/MaterialSynthesisImageAnalyzer.h"

namespace
{
constexpr int MaxMaterialDepth = 8;
constexpr int MaxMaterialOperations = 64;
constexpr int MaxMaterialBatch = 24;
constexpr int MaxMoodProbeIncreaseClicks = 11;
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
    m_material_shortfall.reset();
    m_material_shortfall_context.clear();
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

    const auto first_material_id = recognize_material();
    if (!first_material_id) {
        return Result::Unsupported;
    }
    const std::string& material_id = *first_material_id;
    const std::string& first_material_name = ItemData.get_item_name(material_id);
    const auto material_rarity = ItemData.get_item_rarity(material_id);
    if (!material_rarity) {
        Log.warn("MaterialSynthesis | material rarity is not in item config", material_id);
        return Result::Unsupported;
    }
    if (!material_stack.emplace(material_id).second) {
        Log.warn("MaterialSynthesis | recipe cycle detected", material_id);
        return Result::InsufficientResources;
    }

    // 根配方由用户手动打开；递归进入的材料必然有父配方，只在完成时识别并点击返回。
    const bool has_parent = depth > 0;
    Result result = Result::Completed;
    cv::Mat count_image;
    while (!need_exit()) {
        if (detect_task("MiniGame@MaterialSynthesis@Satisfied")) {
            break;
        }
        count_image.release();
        if (operation_budget <= 0) {
            result = Result::InsufficientResources;
            break;
        }
        --operation_budget;
        if (!detect_task("MiniGame@MaterialSynthesis@Workshop")) {
            result = Result::NavigationFailed;
            break;
        }

        const auto current_material_id = recognize_material();
        count_image = ctrler()->get_image();
        const auto count =
            MaterialSynthesisImageAnalyzer::read_number(count_image, "MiniGame@MaterialSynthesis@RequiredCount", false);
        if (!current_material_id || *current_material_id != material_id || !count || *count <= 0) {
            result = Result::Unsupported;
            break;
        }
        report_status(
            "MaterialSynthesisMaterial",
            json::object {
                { "material", first_material_name },
                { "material_id", material_id },
                { "rarity", *material_rarity },
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
                        { "material", first_material_name },
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
                const auto parent_material_id = recognize_material();
                if (!parent_material_id || *parent_material_id != material_id) {
                    result = Result::NavigationFailed;
                    break;
                }
            }
            else if (detect_task(prefix + "Unavailable")) {
                Log.warn("MaterialSynthesis | ingredient unavailable", ingredient, material_id);
                report_status(
                    "MaterialSynthesisIngredientUnavailable",
                    json::object {
                        { "material", first_material_name },
                        { "material_id", material_id },
                        { "depth", depth },
                        { "ingredient", ingredient },
                    });
                result = Result::InsufficientResources;
                if (m_collect_material_shortfall) {
                    const auto& formula = ItemData.get_item_formula(material_id);
                    std::vector<std::string> ingredient_ids;
                    for (const auto& [id, quantity] : formula) {
                        ingredient_ids.emplace_back(id);
                    }
                    const auto image = ctrler()->get_image();
                    const auto missing =
                        MaterialSynthesisImageAnalyzer::observe_missing_material(image, prefix, ingredient_ids);
                    const auto needed =
                        MaterialSynthesisImageAnalyzer::read_number(image, "MiniGame@MaterialSynthesis@RequiredCount");
                    if (missing && needed && *needed > 0) {
                        const auto per_craft = formula.find(missing->item_id);
                        if (per_craft != formula.end() && per_craft->second > 0 &&
                            missing->required == per_craft->second &&
                            *needed <= std::numeric_limits<int>::max() / per_craft->second) {
                            const int shortfall = *needed * per_craft->second - missing->owned;
                            if (shortfall > 0) {
                                m_material_shortfall = { missing->item_id, shortfall };
                                m_material_shortfall_context = material_id + ":" + std::to_string(ingredient);
                            }
                        }
                    }
                }
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
                    { "material", first_material_name },
                    { "material_id", material_id },
                    { "reason",
                      operator_missing    ? "missing"
                      : mood_insufficient ? "low_mood"
                                          : "initial" },
                });
            result = select_processing_operator(
                material_id,
                *material_rarity,
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
            int increase_clicks_without_low_mood = 0;
            while (selected_count < batch_count && !need_exit()) {
                if (!run_task("MiniGame@MaterialSynthesis@Increase", 10)) {
                    result = Result::NavigationFailed;
                    break;
                }
                ++selected_count;
                if (detect_task("MiniGame@MaterialSynthesis@LowMood")) {
                    if (!run_task("MiniGame@MaterialSynthesis@Decrease", 2)) {
                        result = Result::NavigationFailed;
                    }
                    else {
                        --selected_count;
                    }
                    break;
                }
                if (need_exit()) {
                    result = Result::Cancelled;
                    break;
                }

                ++increase_clicks_without_low_mood;
                if (increase_clicks_without_low_mood >= MaxMoodProbeIncreaseClicks && selected_count < batch_count) {
                    Log.info("MaterialSynthesis | mood probe increase limit reached", selected_count, batch_count);
                    break;
                }
            }
        }
        if (result != Result::Completed) {
            break;
        }

        // 没有可替换的满心情干员时，保留当前干员并将单批数量降到其心情可承受的范围。
        while (selected_count > 1 && detect_task("MiniGame@MaterialSynthesis@LowMood") && !need_exit()) {
            if (!run_task("MiniGame@MaterialSynthesis@Decrease", 2)) {
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
                { "material", first_material_name },
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
        // 可合成的蓝材料缺少低级原料时，优先上报蓝材料本身的缺口，供培养任务去绿票商店兑换。
        if (result == Result::InsufficientResources && m_collect_material_shortfall && !count_image.empty() &&
            ItemData.get_item_rarity(material_id) == 2) {
            // 合成沿用原有数量解析；兑换缺口必须从同一帧重新严格确认。
            const auto missing_count =
                MaterialSynthesisImageAnalyzer::read_number(count_image, "MiniGame@MaterialSynthesis@RequiredCount");
            if (missing_count && *missing_count > 0) {
                m_material_shortfall = { material_id, *missing_count };
                m_material_shortfall_context = material_id;
            }
        }
        return result;
    }
    if (has_parent) {
        Log.info("MaterialSynthesis | return to parent recipe", depth);
        report_status(
            "MaterialSynthesisReturn",
            json::object {
                { "material", first_material_name },
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
    int material_rarity,
    bool operator_missing,
    bool& operator_changed,
    InfrastProcessingTask& processing_task)
{
    operator_changed = false;
    if (!operator_missing && !run_task("MiniGame@MaterialSynthesis@OpenOperatorList")) {
        return Result::NavigationFailed;
    }
    if (processing_task.select_operator(material_id, material_rarity, operator_changed)) {
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
    return ProcessTask(*this, { task_name }).set_retry_times(retry_times).run();
}

bool asst::MaterialSynthesisTaskPlugin::detect_task(const std::string& task_name)
{
    return run_task(task_name, 5);
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

std::optional<std::string> asst::MaterialSynthesisTaskPlugin::recognize_material()
{
    MaterialSynthesisImageAnalyzer analyzer(ctrler()->get_image());
    if (!analyzer.analyze()) {
        return std::nullopt;
    }
    return analyzer.get_result().templ_name;
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
