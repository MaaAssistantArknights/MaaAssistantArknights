#include "InfrastMaterialCraftTask.h"

#include <chrono>
#include <thread>

#include <algorithm>
#include <charconv>
#include <ranges>

#include <meojson/json.hpp>

#include "Config/Miscellaneous/MaterialRecipeConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/WorkingDir.hpp"
#include "Vision/Infrast/InfrastMaterialCraftImageAnalyzer.h"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

using namespace asst;
using namespace asst::utils::path_literals;

namespace
{
constexpr std::string_view WorkshopQualityDropdownArrowDownTemplate = "MaterialCraft-QualityDropdownArrowDown";
constexpr std::string_view WorkshopQualityDropdownArrowUpTemplate = "MaterialCraft-QualityDropdownArrowUp";
constexpr std::string_view WorkshopQualityOptionAllTemplate = "MaterialCraft-QualityOptionAll";
constexpr std::string_view WorkshopQualityOptionNormalTemplate = "MaterialCraft-QualityOptionNormal";
constexpr std::string_view WorkshopQualityOptionRareTemplate = "MaterialCraft-QualityOptionRare";
constexpr std::string_view WorkshopQualityOptionExcellentTemplate = "MaterialCraft-QualityOptionExcellent";
constexpr std::string_view WorkshopQualityOptionSuperiorTemplate = "MaterialCraft-QualityOptionSuperior";
constexpr std::array<std::string_view, 5> WorkshopQualityOptionTemplates = {
    WorkshopQualityOptionAllTemplate,       WorkshopQualityOptionNormalTemplate,   WorkshopQualityOptionRareTemplate,
    WorkshopQualityOptionExcellentTemplate, WorkshopQualityOptionSuperiorTemplate,
};

Point center_of(const Rect& rect)
{
    return { rect.x + rect.width / 2, rect.y + rect.height / 2 };
}

std::string_view quality_option_template_by_gold_cost(int gold_cost)
{
    if (gold_cost <= 100) {
        return WorkshopQualityOptionNormalTemplate;
    }
    if (gold_cost <= 200) {
        return WorkshopQualityOptionRareTemplate;
    }
    if (gold_cost <= 300) {
        return WorkshopQualityOptionExcellentTemplate;
    }
    return WorkshopQualityOptionSuperiorTemplate;
}

bool is_elite_category_rect(const cv::Mat& image, const Rect& rect)
{
    const int center_y = center_of(rect).y;
    return center_y > image.rows * 20 / 100 && center_y < image.rows * 40 / 100;
}
}

InfrastMaterialCraftTask::InfrastMaterialCraftTask(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain) :
    InfrastAbstractTask(callback, inst, task_chain)
{
}

bool InfrastMaterialCraftTask::set_params(const json::value& params)
{
    try {
        auto request = parse_material_craft_request(params);
        if (request.targets.empty()) {
            return false;
        }
        m_request = std::move(request);
        m_plan = {};
        return true;
    }
    catch (const std::exception& e) {
        Log.error(__FUNCTION__, e.what());
        return false;
    }
}

bool InfrastMaterialCraftTask::build_plan()
{
    MaterialCraftPlanner planner(MaterialRecipes.formulas());
    m_plan = planner.build(m_request, [this] { return need_exit(); });
    if (need_exit()) {
        return false;
    }
    auto info = basic_info_with_what("MaterialCraftPlan");
    info["details"] = material_craft_plan_json(m_plan);
    callback(AsstMsg::SubTaskExtraInfo, info);
    if (!m_plan.valid || !m_plan.missing.empty()) {
        info["what"] = "MaterialCraftPlanFailed";
        callback(AsstMsg::SubTaskExtraInfo, info);
    }
    // Keep the existing ability to try crafting against an out-of-date depot snapshot.
    return m_plan.valid && !m_plan.operations.empty();
}

bool InfrastMaterialCraftTask::_run()
{
    if (need_exit()) {
        return false;
    }

    LogTraceFunction;

    m_next_operation_id = 0;
    if (!build_plan()) {
        return false;
    }

    if (!ensure_processing_room()) {
        return false;
    }

    if (!ensure_craft_page()) {
        return false;
    }

    for (const CraftOperation& operation : m_plan.operations) {
        if (need_exit()) {
            return false;
        }
        if (!execute_operation(operation)) {
            return false;
        }
    }

    return true;
}

bool InfrastMaterialCraftTask::craft_sleep(unsigned milliseconds) const
{
    // Check cancellation between short waits instead of waiting for the full UI animation.
    constexpr unsigned PollInterval = 50;
    while (milliseconds > 0) {
        const unsigned interval = std::min(milliseconds, PollInterval);
        if (need_exit()) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        milliseconds -= interval;
    }
    return !need_exit();
}

bool InfrastMaterialCraftTask::ensure_processing_room()
{
    if (need_exit()) {
        return false;
    }

    auto ready = [&](const cv::Mat& image) {
        return is_craft_page(image) || is_formula_selector(image) || is_processing_room(image);
    };

    cv::Mat image = ctrler()->get_image();
    if (ready(image)) {
        return true;
    }

    // Quick switching from mastery confirmation leads to the training room,
    // not the overview. Leave these pages before the usual base navigation.
    if (!ProcessTask(*this, { "MaterialCraft-LeaveTraining" }).run() || need_exit()) {
        return false;
    }

    for (int attempt = 0; attempt != 2; ++attempt) {
        // Every navigation attempt starts from the base overview, including retries
        // after entering a room whose page could not be confirmed.
        if (need_exit() || !ProcessTask(*this, { "MaterialCraft@InfrastBegin" }).run() || need_exit()) {
            return false;
        }
        image = ctrler()->get_image();
        if (ready(image)) {
            return true;
        }

        // Processing has its own normal/mini facility templates. Try the current
        // view first, then use the same overview swipes as the office task.
        if (!enter_facility()) {
            if (need_exit()) {
                return false;
            }
            swipe_to_the_left_of_main_ui();
            if (need_exit()) {
                return false;
            }
            if (!enter_facility()) {
                if (need_exit()) {
                    return false;
                }
                swipe_to_right_of_main_ui();
                if (need_exit()) {
                    return false;
                }
                if (!enter_facility()) {
                    Log.warn(__FUNCTION__, "| failed to enter processing room", attempt);
                    save_img(
                        utils::path("debug") / utils::path("material_craft") / utils::path("enter_processing_room"));
                    continue;
                }
            }
        }

        // Do not click the room tab until the destination is actually recognized.
        for (int poll = 0; poll != 3; ++poll) {
            if (need_exit()) {
                return false;
            }
            if (ready(ctrler()->get_image())) {
                return true;
            }
            if (!craft_sleep(Task.get("MaterialCraft-RetryDelay")->post_delay)) {
                return false;
            }
        }
    }

    save_img(utils::path("debug") / utils::path("material_craft") / utils::path("ensure_processing_room"));
    return false;
}

bool InfrastMaterialCraftTask::ensure_craft_page()
{
    if (need_exit()) {
        return false;
    }

    for (int i = 0; i != 3; ++i) {
        if (need_exit()) {
            return false;
        }
        cv::Mat image = ctrler()->get_image();
        if (is_craft_page(image)) {
            return true;
        }

        auto entrance = match_workshop_template(image, "MaterialCraft-CraftEntrance");
        if (entrance) {
            if (need_exit() || !ProcessTask(*this, { "MaterialCraft-EnterCraftPage" }).run()) {
                return false;
            }
            continue;
        }

        if (is_formula_selector(image)) {
            return true;
        }

        save_img(utils::path("debug") / utils::path("material_craft") / utils::path("ensure_craft_page"));
        if (!craft_sleep(Task.get("MaterialCraft-RetryDelay")->post_delay)) {
            return false;
        }
    }

    if (need_exit()) {
        return false;
    }
    const auto image = ctrler()->get_image();
    return is_craft_page(image) || is_formula_selector(image);
}

bool InfrastMaterialCraftTask::is_processing_room(const cv::Mat& image) const
{
    if (need_exit()) {
        return false;
    }

    return match_workshop_template(image, "MaterialCraft-CraftEntrance").has_value();
}

bool InfrastMaterialCraftTask::is_craft_page(const cv::Mat& image) const
{
    if (need_exit()) {
        return false;
    }

    const bool has_slot = match_workshop_template(image, "MaterialCraft-FormulaSlotSelected").has_value() ||
                          match_workshop_template(image, "MaterialCraft-FormulaSlotEmpty").has_value();
    const bool has_stepper = match_workshop_template(image, "MaterialCraft-PlusButton").has_value() ||
                             match_workshop_template(image, "MaterialCraft-MinusButton").has_value();
    if (has_slot && has_stepper) {
        return true;
    }

    return false;
}

bool InfrastMaterialCraftTask::is_formula_selector(const cv::Mat& image) const
{
    if (need_exit()) {
        return false;
    }

    auto category = match_workshop_template(image, "MaterialCraft-EliteCategory");
    if (category && is_elite_category_rect(image, *category)) {
        return true;
    }

    if (match_workshop_template(image, std::string(WorkshopQualityDropdownArrowDownTemplate)) ||
        match_workshop_template(image, std::string(WorkshopQualityDropdownArrowUpTemplate))) {
        return true;
    }
    return false;
}

bool InfrastMaterialCraftTask::is_obtain_items_page(const cv::Mat& image) const
{
    if (need_exit()) {
        return false;
    }

    Matcher matcher(image);
    matcher.set_task_info("MaterialCraft-ObtainItemsIcon");
    return matcher.analyze().has_value();
}

bool InfrastMaterialCraftTask::execute_operation(const CraftOperation& operation)
{
    int remaining = operation.batches;
    while (remaining > 0 && !need_exit()) {
        if (!open_formula_selector() || !select_formula(operation.formula)) {
            return false;
        }
        const auto batches = set_craft_count(remaining);
        if (!batches || *batches <= 0 || *batches > remaining) {
            return false;
        }

        CraftOperation actual { operation.formula, *batches };
        RegionOCRer byproduct(ctrler()->get_image());
        byproduct.set_task_info("MaterialCraft-ByproductRate");
        const auto rate = byproduct.analyze();
        // Recipe deltas cannot account for random byproducts. Do not present them as a full inventory.
        m_inventory_complete = rate && rate->text == "0%";
        const int operation_id = m_next_operation_id++;
        callback_operation("MaterialCraftOperationStarted", actual, operation_id);
        if (!click_start_button() || !click_complete_tick(actual, operation_id)) {
            return false;
        }
        remaining -= *batches;
    }
    return remaining == 0;
}

void InfrastMaterialCraftTask::callback_operation(
    const std::string& what,
    const CraftOperation& operation,
    int operation_id)
{
    auto info = basic_info_with_what(what);
    auto& details = info["details"];
    details["operation_id"] = operation_id;
    details["item_id"] = operation.formula.item_id;
    details["batches"] = operation.batches;
    if (what == "MaterialCraftOperationCompleted") {
        details["inventory_complete"] = m_inventory_complete;
        std::unordered_map<std::string, long long> deltas;
        deltas[operation.formula.item_id] += static_cast<long long>(operation.formula.count) * operation.batches;
        for (const auto& cost : operation.formula.costs) {
            deltas[cost.item_id] -= static_cast<long long>(cost.count) * operation.batches;
        }
        // Only adjust currency when the supplied depot snapshot contains it.
        if (m_request.inventory.contains("4001")) {
            deltas["4001"] -= static_cast<long long>(operation.formula.gold_cost) * operation.batches;
        }
        json::array changes;
        for (const auto& [id, count] : deltas) {
            changes.emplace_back(json::object { { "item_id", id }, { "count", count } });
        }
        details["inventory_changes"] = std::move(changes);
    }
    callback(AsstMsg::SubTaskExtraInfo, info);
}

bool InfrastMaterialCraftTask::open_formula_selector()
{
    if (need_exit()) {
        return false;
    }

    for (int i = 0; i != 5; ++i) {
        if (need_exit()) {
            return false;
        }
        cv::Mat image = ctrler()->get_image();
        if (is_obtain_items_page(image)) {
            auto complete_tick = match_workshop_template(image, "MaterialCraft-CompleteTick");
            if (complete_tick) {
                if (need_exit() || !ctrler()->click(center_of(*complete_tick))) {
                    return false;
                }
                if (!craft_sleep(Task.get("MaterialCraft-RewardDelay")->post_delay)) {
                    return false;
                }
                continue;
            }
        }

        if (is_formula_selector(image)) {
            return true;
        }

        if (!is_craft_page(image)) {
            save_img(utils::path("debug") / utils::path("material_craft") / utils::path("open_formula_selector"));
            if (!craft_sleep(Task.get("MaterialCraft-AnimationDelay")->post_delay)) {
                return false;
            }
            continue;
        }

        auto slot = match_workshop_template(image, "MaterialCraft-FormulaSlotSelected-Click");
        if (!slot) {
            slot = match_workshop_template(image, "MaterialCraft-FormulaSlotEmpty-Click");
        }
        if (slot) {
            if (need_exit() || !ctrler()->click(center_of(*slot))) {
                return false;
            }
            if (!craft_sleep(Task.get("MaterialCraft-AnimationDelay")->post_delay)) {
                return false;
            }
            image = ctrler()->get_image();
            if (is_formula_selector(image)) {
                return true;
            }
            Log.warn(__FUNCTION__, "| formula selector did not open after slot click");
            continue;
        }

        auto entrance = match_workshop_template(image, "MaterialCraft-CraftEntrance");
        if (entrance) {
            if (need_exit() || !ctrler()->click(*entrance)) {
                return false;
            }
            if (!craft_sleep(Task.get("MaterialCraft-AnimationDelay")->post_delay)) {
                return false;
            }
            continue;
        }

        save_img(utils::path("debug") / utils::path("material_craft") / utils::path("open_formula_selector"));
        if (!craft_sleep(Task.get("MaterialCraft-AnimationDelay")->post_delay)) {
            return false;
        }
    }

    return false;
}

bool InfrastMaterialCraftTask::select_formula(const Formula& formula)
{
    if (need_exit() || !prepare_formula_selector(formula)) {
        return false;
    }
    const int page_limit = max_formula_pages(formula);
    auto result = scan_formula_pages(formula, page_limit);
    if (result == FormulaScanResult::NotFound && rewind_formula_list_to_top()) {
        result = scan_formula_pages(formula, page_limit);
    }
    if (result == FormulaScanResult::Selected) {
        return true;
    }
    save_img(utils::path("debug") / utils::path("material_craft") / utils::path("select_formula_failed"));
    return false;
}

bool InfrastMaterialCraftTask::prepare_formula_selector(const Formula& formula)
{
    if (need_exit()) {
        return false;
    }

    if (!click_elite_category()) {
        save_img(utils::path("debug") / utils::path("material_craft") / utils::path("select_formula_category_failed"));
        return false;
    }
    if (!select_quality_filter(formula)) {
        save_img(utils::path("debug") / utils::path("material_craft") / utils::path("select_formula_quality_failed"));
        return false;
    }

    return true;
}

bool InfrastMaterialCraftTask::click_elite_category()
{
    if (need_exit()) {
        return false;
    }

    cv::Mat image = ctrler()->get_image();
    auto category = match_workshop_template(image, "MaterialCraft-EliteCategory");
    if (category && is_elite_category_rect(image, *category)) {
        if (need_exit() || !ctrler()->click(center_of(*category))) {
            return false;
        }
        if (!craft_sleep(Task.get("MaterialCraft-AnimationDelay")->post_delay)) {
            return false;
        }
        return true;
    }

    if (category) {
        Log.warn(__FUNCTION__, "| ignore non-elite category template match", *category);
    }
    if (is_formula_selector(image)) {
        Log.info(__FUNCTION__, "| elite category is likely already selected");
        return true;
    }

    Log.warn(__FUNCTION__, "| elite category template not found");
    return false;
}

bool InfrastMaterialCraftTask::select_quality_filter(const Formula& formula)
{
    if (need_exit()) {
        return false;
    }

    const std::string quality_template(quality_option_template_by_gold_cost(formula.gold_cost));

    for (int attempt = 0; attempt != 3; ++attempt) {
        if (need_exit()) {
            return false;
        }
        if (!open_quality_menu()) {
            Log.warn(__FUNCTION__, "| quality menu not opened", quality_template);
            if (!craft_sleep(Task.get("MaterialCraft-RetryDelay")->post_delay)) {
                return false;
            }
            continue;
        }

        cv::Mat image = ctrler()->get_image();
        auto option = match_workshop_template(image, quality_template);
        if (!option) {
            Log.warn(__FUNCTION__, "| quality option template not found", quality_template);
            if (!craft_sleep(Task.get("MaterialCraft-RetryDelay")->post_delay)) {
                return false;
            }
            continue;
        }

        if (need_exit() || !ctrler()->click(center_of(*option))) {
            return false;
        }
        if (!craft_sleep(Task.get("MaterialCraft-AnimationDelay")->post_delay)) {
            return false;
        }

        if (close_quality_menu()) {
            return true;
        }

        Log.warn(__FUNCTION__, "| quality menu not closed", quality_template);
        if (!craft_sleep(Task.get("MaterialCraft-RetryDelay")->post_delay)) {
            return false;
        }
    }

    return false;
}

bool InfrastMaterialCraftTask::open_quality_menu()
{
    if (need_exit()) {
        return false;
    }

    for (int i = 0; i != 3; ++i) {
        if (need_exit()) {
            return false;
        }
        cv::Mat image = ctrler()->get_image();
        if (is_quality_menu_open(image)) {
            return true;
        }

        auto arrow = match_workshop_template(image, std::string(WorkshopQualityDropdownArrowDownTemplate));
        if (!arrow) {
            if (!craft_sleep(Task.get("MaterialCraft-RetryDelay")->post_delay)) {
                return false;
            }
            continue;
        }

        if (need_exit() || !ctrler()->click(center_of(*arrow))) {
            return false;
        }
        if (!craft_sleep(Task.get("MaterialCraft-AnimationDelay")->post_delay)) {
            return false;
        }
    }

    return is_quality_menu_open(ctrler()->get_image());
}

bool InfrastMaterialCraftTask::close_quality_menu()
{
    if (need_exit()) {
        return false;
    }

    for (int i = 0; i != 3; ++i) {
        if (need_exit()) {
            return false;
        }
        cv::Mat image = ctrler()->get_image();
        if (!is_quality_menu_open(image)) {
            return true;
        }

        auto arrow = match_workshop_template(image, std::string(WorkshopQualityDropdownArrowUpTemplate));
        if (!arrow) {
            if (!craft_sleep(Task.get("MaterialCraft-RetryDelay")->post_delay)) {
                return false;
            }
            continue;
        }

        if (need_exit() || !ctrler()->click(center_of(*arrow))) {
            return false;
        }
        if (!craft_sleep(Task.get("MaterialCraft-AnimationDelay")->post_delay)) {
            return false;
        }
    }

    return !is_quality_menu_open(ctrler()->get_image());
}

bool InfrastMaterialCraftTask::is_quality_menu_open(const cv::Mat& image) const
{
    // Menu text alone can also match the craft page. Require the expanded dropdown header.
    if (need_exit() || !match_workshop_template(image, std::string(WorkshopQualityDropdownArrowUpTemplate))) {
        return false;
    }

    return std::ranges::any_of(WorkshopQualityOptionTemplates, [&](std::string_view option_template) {
        return match_workshop_template(image, std::string(option_template)).has_value();
    });
}

int InfrastMaterialCraftTask::max_formula_pages(const Formula& formula) const
{
    return Task.get(std::string(quality_option_template_by_gold_cost(formula.gold_cost)))->max_times;
}

InfrastMaterialCraftTask::FormulaScanResult
    InfrastMaterialCraftTask::scan_formula_pages(const Formula& formula, int page_limit)
{
    if (need_exit()) {
        return FormulaScanResult::Cancelled;
    }

    for (int page = 0; page != page_limit; ++page) {
        if (need_exit()) {
            return FormulaScanResult::Cancelled;
        }
        const FormulaScanResult result = scan_and_click_formula(formula);
        if (need_exit()) {
            return FormulaScanResult::Cancelled;
        }
        if (result == FormulaScanResult::Selected) {
            return FormulaScanResult::Selected;
        }
        if (result == FormulaScanResult::VerificationFailed || result == FormulaScanResult::Cancelled) {
            return result;
        }

        if (page + 1 == page_limit) {
            break;
        }

        if (!swipe_formula_list(true)) {
            return need_exit() ? FormulaScanResult::Cancelled : FormulaScanResult::VerificationFailed;
        }
    }

    return FormulaScanResult::NotFound;
}

InfrastMaterialCraftTask::FormulaScanResult InfrastMaterialCraftTask::scan_and_click_formula(const Formula& formula)
{
    if (need_exit()) {
        return FormulaScanResult::Cancelled;
    }

    cv::Mat image = ctrler()->get_image();
    auto scan_matches = [&](const std::vector<double>& scales) {
        InfrastMaterialCraftImageAnalyzer analyzer(image, Task.get("MaterialCraft-FormulaProduct")->roi);
        analyzer.set_item_id(formula.item_id);
        analyzer.set_cancel_check([this] { return need_exit(); });
        analyzer.set_task_info("MaterialCraft-FormulaProduct");
        analyzer.set_scales(scales);
        if (!analyzer.analyze()) {
            return std::vector<InfrastMaterialCraftImageAnalyzer::FormulaMatch>();
        }
        return analyzer.get_result();
    };

    const auto task = Task.get("MaterialCraft-FormulaProduct");
    std::vector<double> scales;
    for (const auto scale : task->special_params) {
        scales.push_back(scale / 100.0);
    }
    if (scales.empty()) {
        return FormulaScanResult::VerificationFailed;
    }
    std::vector<InfrastMaterialCraftImageAnalyzer::FormulaMatch> matches = scan_matches({ scales.front() });
    if (need_exit()) {
        return FormulaScanResult::Cancelled;
    }
    if (matches.empty()) {
        matches = scan_matches(std::vector<double>(scales.begin() + 1, scales.end()));
    }
    if (need_exit()) {
        return FormulaScanResult::Cancelled;
    }
    if (matches.empty()) {
        return FormulaScanResult::NotFound;
    }

    for (const auto& formula_match : matches) {
        if (need_exit()) {
            return FormulaScanResult::Cancelled;
        }
        for (int click_count = 0; click_count != 3; ++click_count) {
            if (need_exit()) {
                return FormulaScanResult::Cancelled;
            }
            if (need_exit() || !ctrler()->click(formula_match.click_rect)) {
                return need_exit() ? FormulaScanResult::Cancelled : FormulaScanResult::VerificationFailed;
            }
            if (!craft_sleep(Task.get("MaterialCraft-AnimationDelay")->post_delay)) {
                return FormulaScanResult::Cancelled;
            }

            image = ctrler()->get_image();
            if (is_craft_page(image)) {
                if (selected_formula_matches(formula)) {
                    return FormulaScanResult::Selected;
                }

                Log.warn(__FUNCTION__, "| selected formula verification failed", formula.formula_id, formula.item_id);
                save_img(utils::path("debug") / utils::path("material_craft") / utils::path("select_formula_mismatch"));
                return need_exit() ? FormulaScanResult::Cancelled : FormulaScanResult::VerificationFailed;
            }
            if (!is_formula_selector(image)) {
                if (!craft_sleep(Task.get("MaterialCraft-RetryDelay")->post_delay)) {
                    return FormulaScanResult::Cancelled;
                }
                image = ctrler()->get_image();
                if (is_craft_page(image)) {
                    if (selected_formula_matches(formula)) {
                        return FormulaScanResult::Selected;
                    }

                    Log.warn(
                        __FUNCTION__,
                        "| selected formula verification failed",
                        formula.formula_id,
                        formula.item_id);
                    save_img(
                        utils::path("debug") / utils::path("material_craft") / utils::path("select_formula_mismatch"));
                    return need_exit() ? FormulaScanResult::Cancelled : FormulaScanResult::VerificationFailed;
                }
            }
        }
    }

    return FormulaScanResult::NotFound;
}

bool InfrastMaterialCraftTask::selected_formula_matches(const Formula& formula) const
{
    if (need_exit()) {
        return false;
    }

    for (int i = 0; i != 3; ++i) {
        if (need_exit()) {
            return false;
        }
        const cv::Mat image = ctrler()->get_image();

        MaterialImageAnalyzer analyzer(image);
        analyzer.set_task_info("MaterialCraft-SelectedProduct");
        analyzer.set_item_id(formula.item_id);
        analyzer.set_cancel_check([this] { return need_exit(); });
        if (analyzer.analyze()) {
            const auto& match = analyzer.get_result().front();
            Log.trace(
                __FUNCTION__,
                "| verified selected formula",
                formula.formula_id,
                formula.item_id,
                "score",
                match.score,
                "rect",
                match.rect);
            return true;
        }

        if (!craft_sleep(Task.get("MaterialCraft-VerifyDelay")->post_delay)) {
            return false;
        }
    }

    return false;
}

bool InfrastMaterialCraftTask::rewind_formula_list(int swipe_times)
{
    if (need_exit()) {
        return false;
    }

    bool swiped = false;
    for (int i = 0; i != swipe_times; ++i) {
        if (need_exit()) {
            return false;
        }
        if (!swipe_formula_list(false)) {
            return false;
        }
        swiped = true;
    }
    return swiped;
}

bool InfrastMaterialCraftTask::rewind_formula_list_to_top()
{
    if (need_exit()) {
        return false;
    }

    return rewind_formula_list(Task.get("MaterialCraft-FormulaListRewind")->max_times);
}

bool InfrastMaterialCraftTask::swipe_formula_list(bool forward)
{
    if (need_exit()) {
        return false;
    }

    const auto task = Task.get(forward ? "MaterialCraft-FormulaListSwipe" : "MaterialCraft-FormulaListRewind");
    if (!craft_sleep(task->pre_delay)) {
        return false;
    }

    // Use the same swipe parameter convention as ProcessTask, retaining input failure
    // propagation and the crafting task's short cancellation polling interval.
    const auto& params = task->special_params;
    const bool swiped = ctrler()->swipe(
        task->specific_rect,
        task->rect_move,
        params.size() > 0 ? params[0] : 0,
        params.size() > 1 ? params[1] != 0 : false,
        params.size() > 2 ? params[2] : 1,
        params.size() > 3 ? params[3] : 1,
        false,
        task->high_resolution_swipe_fix);
    return swiped && craft_sleep(task->post_delay);
}

std::optional<int> InfrastMaterialCraftTask::read_craft_count() const
{
    if (need_exit()) {
        return std::nullopt;
    }
    RegionOCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info("MaterialCraft-Quantity");
    const auto result = analyzer.analyze();
    if (!result) {
        return std::nullopt;
    }
    int count = 0;
    const auto& text = result->text;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), count);
    if (error != std::errc() || end != text.data() + text.size() || count < 0) {
        return std::nullopt;
    }
    return count;
}

std::optional<int> InfrastMaterialCraftTask::set_craft_count(int batches)
{
    if (need_exit() || batches <= 0) {
        return std::nullopt;
    }
    auto current = read_craft_count();
    if (!current || *current <= 0 || *current > batches) {
        save_img(utils::path("debug") / utils::path("material_craft") / utils::path("craft_count_failed"));
        return std::nullopt;
    }
    if (*current == batches) {
        return current;
    }
    const auto image = ctrler()->get_image();
    const auto button = match_workshop_template(image, "MaterialCraft-PlusButton-Click");
    if (!button) {
        return std::nullopt;
    }
    const auto task = Task.get("MaterialCraft-IncreaseQuantity");
    const Rect click_rect = button->move(task->rect_move);
    while (*current < batches) {
        // Read back in small groups so a game-side cap cannot cause unbounded clicking.
        const int clicks = std::min(10, batches - *current);
        for (int i = 0; i < clicks; ++i) {
            if (!craft_sleep(task->pre_delay) || !ctrler()->click(click_rect) || !craft_sleep(task->post_delay)) {
                return std::nullopt;
            }
        }
        if (!craft_sleep(Task.get("MaterialCraft-Quantity")->post_delay)) {
            return std::nullopt;
        }
        auto observed = read_craft_count();
        if (!observed || *observed < *current || *observed > batches) {
            return std::nullopt;
        }
        if (*observed == *current) {
            return current;
        }
        current = observed;
    }
    return current;
}

bool InfrastMaterialCraftTask::click_start_button()
{
    if (need_exit()) {
        return false;
    }

    for (int i = 0; i != 5; ++i) {
        if (need_exit()) {
            return false;
        }
        cv::Mat image = ctrler()->get_image();
        auto start = match_workshop_template(image, "MaterialCraft-StartButton");
        if (start) {
            if (need_exit() || !ctrler()->click(*start)) {
                return false;
            }
            if (!craft_sleep(Task.get("MaterialCraft-AnimationDelay")->post_delay)) {
                return false;
            }
            return true;
        }
        if (!craft_sleep(Task.get("MaterialCraft-RetryDelay")->post_delay)) {
            return false;
        }
    }
    save_img(utils::path("debug") / utils::path("material_craft") / utils::path("start_button_failed"));
    return false;
}

bool InfrastMaterialCraftTask::click_complete_tick(const CraftOperation& operation, int operation_id)
{
    if (need_exit()) {
        return false;
    }

    bool has_obtained_items = false;
    for (int i = 0; i != 20; ++i) {
        if (need_exit()) {
            return false;
        }
        cv::Mat image = ctrler()->get_image();
        if (has_obtained_items && (is_craft_page(image) || is_formula_selector(image))) {
            return true;
        }

        if (!is_obtain_items_page(image)) {
            if (!craft_sleep(Task.get("MaterialCraft-RetryDelay")->post_delay)) {
                return false;
            }
            continue;
        }

        if (!has_obtained_items) {
            // Commit a confirmed result even if cancellation interrupts dismissing the reward page.
            callback_operation("MaterialCraftOperationCompleted", operation, operation_id);
            has_obtained_items = true;
        }
        auto tick = match_workshop_template(image, "MaterialCraft-CompleteTick");
        if (tick) {
            if (need_exit() || !ctrler()->click(center_of(*tick))) {
                return false;
            }
            if (!craft_sleep(Task.get("MaterialCraft-RewardDelay")->post_delay)) {
                return false;
            }
            continue;
        }
        if (!craft_sleep(Task.get("MaterialCraft-RetryDelay")->post_delay)) {
            return false;
        }
    }
    save_img(utils::path("debug") / utils::path("material_craft") / utils::path("complete_tick_failed"));
    return false;
}

std::optional<Rect>
    InfrastMaterialCraftTask::match_workshop_template(const cv::Mat& image, const std::string& task_name) const
{
    if (need_exit()) {
        return std::nullopt;
    }
    Matcher matcher(image);
    const auto task = Task.get<MatchTaskInfo>(task_name);
    matcher.set_task_info(task);
    // Resource task parameters do not include mask_close. Preserve the workshop
    // templates' previous mask preprocessing when loading their parameters from TaskData.
    matcher.set_mask_ranges(task->mask_ranges, false, true);
    const auto result = matcher.analyze();
    return result && !need_exit() ? std::optional(result->rect) : std::nullopt;
}
