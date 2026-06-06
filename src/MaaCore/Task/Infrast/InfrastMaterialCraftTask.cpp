#include "InfrastMaterialCraftTask.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <ranges>

#include <meojson/json.hpp>

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Utils/WorkingDir.hpp"
#include "Vision/Infrast/InfrastMaterialCraftImageAnalyzer.h"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

using namespace asst;
using namespace asst::utils::path_literals;

namespace
{
constexpr int WorkshopApCostPerMood = 360000;
constexpr int FormulaListScanSwipePercent = 20;
constexpr int FormulaListRewindSwipePercent = 70;
constexpr int FormulaListScanSwipeDuration = 500;
constexpr int FormulaListRewindSwipeDuration = 360;
constexpr int FormulaListScanSwipeSettleDelay = 1200;
constexpr double FormulaListProductThreshold = 0.68;
constexpr double SelectedFormulaProductThreshold = 0.44;
constexpr int FormulaSelectMaxAttempts = 3;
constexpr std::string_view WorkshopCategoryBuilding = "基建材料";
constexpr std::string_view WorkshopCategoryElite = "精英材料";
constexpr std::string_view WorkshopCategorySkill = "技巧概要";
constexpr std::string_view WorkshopCategoryChip = "芯片";
constexpr std::string_view WorkshopQualityDropdownArrowDownTemplate = "WorkshopQualityDropdownArrowDown.png";
constexpr std::string_view WorkshopQualityDropdownArrowUpTemplate = "WorkshopQualityDropdownArrowUp.png";
constexpr std::string_view WorkshopQualityOptionAllTemplate = "WorkshopQualityOptionAll.png";
constexpr std::string_view WorkshopQualityOptionNormalTemplate = "WorkshopQualityOptionNormal.png";
constexpr std::string_view WorkshopQualityOptionRareTemplate = "WorkshopQualityOptionRare.png";
constexpr std::string_view WorkshopQualityOptionExcellentTemplate = "WorkshopQualityOptionExcellent.png";
constexpr std::string_view WorkshopQualityOptionSuperiorTemplate = "WorkshopQualityOptionSuperior.png";
constexpr std::array<std::string_view, 5> WorkshopQualityOptionTemplates = {
    WorkshopQualityOptionAllTemplate,       WorkshopQualityOptionNormalTemplate,   WorkshopQualityOptionRareTemplate,
    WorkshopQualityOptionExcellentTemplate, WorkshopQualityOptionSuperiorTemplate,
};
constexpr std::array<std::string_view, 6> BuildingFormulaItemIds = {
    "3131", "3132", "3133", "3113", "3114", "3401",
};
constexpr std::array<std::string_view, 16> ChipFormulaItemIds = {
    "3211", "3212", "3221", "3222", "3231", "3232", "3241", "3242",
    "3251", "3252", "3261", "3262", "3271", "3272", "3281", "3282",
};

Point center_of(const Rect& rect)
{
    return { rect.x + rect.width / 2, rect.y + rect.height / 2 };
}

std::string item_display_name(const std::string& item_id)
{
    const std::string& name = ItemData.get_item_name(item_id);
    return name.empty() ? item_id : name;
}

int formula_sort_key(const std::string& formula_id)
{
    try {
        return std::stoi(formula_id);
    }
    catch (...) {
        return INT_MAX;
    }
}

template <size_t N>
bool contains_item_id(const std::array<std::string_view, N>& item_ids, const std::string& item_id)
{
    return std::ranges::find(item_ids, item_id) != item_ids.end();
}

std::string_view formula_category(const std::string& item_id)
{
    if (contains_item_id(BuildingFormulaItemIds, item_id)) {
        return WorkshopCategoryBuilding;
    }
    if (item_id == "3302" || item_id == "3303") {
        return WorkshopCategorySkill;
    }
    if (contains_item_id(ChipFormulaItemIds, item_id)) {
        return WorkshopCategoryChip;
    }
    return WorkshopCategoryElite;
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

Rect quality_dropdown_roi(const cv::Mat& image)
{
    const int left = std::min(image.cols * 88 / 100, image.cols - 1);
    return Rect(left, 0, image.cols - left, std::min(image.rows, 90));
}

Rect quality_menu_roi(const cv::Mat& image)
{
    const int left = std::min(image.cols * 86 / 100, image.cols - 1);
    const int top = std::min(55, image.rows - 1);
    return Rect(left, top, image.cols - left, image.rows - top);
}

Rect complete_tick_roi(const cv::Mat& image)
{
    return Rect(image.cols * 35 / 100, image.rows * 70 / 100, image.cols * 30 / 100, image.rows * 28 / 100);
}

Rect selected_formula_product_roi(const cv::Mat& image)
{
    return Rect(image.cols * 40 / 100, image.rows * 48 / 100, image.cols * 20 / 100, image.rows * 31 / 100);
}

Rect craft_count_roi(const cv::Mat& image)
{
    return Rect(image.cols * 74 / 100, image.rows * 38 / 100, image.cols * 24 / 100, image.rows * 28 / 100);
}

Rect plus_button_roi(const cv::Mat& image)
{
    return Rect(image.cols * 72 / 100, image.rows * 34 / 100, image.cols * 20 / 100, image.rows * 13 / 100);
}

Point safe_plus_click_point(const Rect& rect)
{
    return { rect.x + rect.width * 42 / 100, rect.y + rect.height / 2 };
}

std::optional<int> parse_count_text(std::string_view text)
{
    std::string digits;
    for (unsigned char ch : text) {
        if (std::isdigit(ch)) {
            digits += static_cast<char>(ch);
        }
    }

    if (digits.empty()) {
        return std::nullopt;
    }

    try {
        return std::stoi(digits);
    }
    catch (...) {
        return std::nullopt;
    }
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

int InfrastMaterialCraftTask::CraftState::get_inventory(const std::string& item_id) const
{
    if (auto iter = inventory.find(item_id); iter != inventory.cend()) {
        return iter->second;
    }
    return 0;
}

void InfrastMaterialCraftTask::CraftState::add_missing(const std::string& item_id, int count)
{
    if (count <= 0) {
        return;
    }
    missing[item_id] += count;
}

int InfrastMaterialCraftTask::CraftState::missing_total() const
{
    int total = 0;
    for (const auto& [_, count] : missing) {
        total += count;
    }
    return total;
}

bool InfrastMaterialCraftTask::set_params(const json::value& params)
{
    m_targets.clear();
    m_inventory.clear();
    m_plan.clear();
    m_plan_gold_cost = 0;
    m_plan_ap_cost = 0;

    return parse_targets(params) && parse_inventory(params);
}

bool InfrastMaterialCraftTask::parse_targets(const json::value& params)
{
    auto append_target = [&](std::string item_id, int count) -> bool {
        if (item_id.empty() || count <= 0) {
            return false;
        }
        m_targets.emplace_back(CraftTarget { std::move(item_id), count });
        return true;
    };

    if (auto targets_opt = params.find<json::array>("items")) {
        for (const auto& target : targets_opt.value()) {
            if (!target.is_object()) {
                return false;
            }
            std::string item_id = target.get("itemId", target.get("id", std::string()));
            int count = target.get("count", 0);
            if (!append_target(std::move(item_id), count)) {
                return false;
            }
        }
    }
    else if (auto targets_map_opt = params.find<json::object>("items")) {
        for (const auto& [item_id, count_json] : targets_map_opt.value()) {
            if (!append_target(item_id, static_cast<int>(count_json.as_integer()))) {
                return false;
            }
        }
    }
    else if (auto legacy_targets_opt = params.find<json::array>("targets")) {
        for (const auto& target : legacy_targets_opt.value()) {
            if (!target.is_object()) {
                return false;
            }
            std::string item_id = target.get("itemId", target.get("id", std::string()));
            int count = target.get("count", 0);
            if (!append_target(std::move(item_id), count)) {
                return false;
            }
        }
    }
    else {
        std::string item_id = params.get("itemId", params.get("id", std::string()));
        int count = params.get("count", 0);
        if (!append_target(std::move(item_id), count)) {
            Log.error(__FUNCTION__, "| empty material craft targets");
            return false;
        }
    }

    return !m_targets.empty();
}

bool InfrastMaterialCraftTask::parse_inventory(const json::value& params)
{
    auto inventory_opt = params.find<json::object>("inventory");
    if (!inventory_opt) {
        inventory_opt = params.find<json::object>("depot");
    }
    if (!inventory_opt) {
        Log.error(__FUNCTION__, "| inventory is required");
        return false;
    }

    for (const auto& [item_id, count_json] : inventory_opt.value()) {
        int count = 0;
        if (count_json.is_number()) {
            count = static_cast<int>(count_json.as_integer());
        }
        else if (count_json.is_string()) {
            try {
                count = std::stoi(count_json.as_string());
            }
            catch (...) {
                count = 0;
            }
        }
        if (!item_id.empty() && count > 0) {
            m_inventory[item_id] = count;
        }
    }

    return !m_inventory.empty();
}

bool InfrastMaterialCraftTask::load_formulas()
{
    if (!m_formulas_by_id.empty()) {
        return true;
    }

    const auto path = ResDir.get() / "material_recipes.json"_p;
    auto recipes_json_opt = json::open(path, true, true);
    if (!recipes_json_opt) {
        Log.error(__FUNCTION__, "| failed to open", path);
        return false;
    }

    m_formulas_by_id.clear();
    m_formulas_by_item.clear();

    for (const auto& [formula_id, formula_json] : recipes_json_opt.value().as_object()) {
        std::string item_id = formula_json.get("itemId", std::string());
        if (item_id.empty()) {
            continue;
        }
        if (formula_category(item_id) != WorkshopCategoryElite) {
            continue;
        }

        Formula formula;
        formula.formula_id = formula_json.get("formulaId", formula_id);
        formula.sort_id = formula_json.get("sortId", 0);
        formula.rarity = formula_json.get("rarity", 0);
        formula.item_id = std::move(item_id);
        formula.category = formula_category(formula.item_id);
        formula.count = std::max(1, formula_json.get("count", 1));
        formula.gold_cost = formula_json.get("goldCost", 0);
        formula.ap_cost = formula_json.get("apCost", 0);

        if (auto costs_opt = formula_json.find<json::array>("costs")) {
            for (const auto& cost_json : costs_opt.value()) {
                FormulaCost cost;
                cost.item_id = cost_json.get("id", std::string());
                cost.count = cost_json.get("count", 0);
                if (!cost.item_id.empty() && cost.count > 0) {
                    formula.costs.emplace_back(std::move(cost));
                }
            }
        }

        if (!formula.costs.empty()) {
            m_formulas_by_id.emplace(formula.formula_id, std::move(formula));
        }
    }

    for (auto& [_, formula] : m_formulas_by_id) {
        m_formulas_by_item[formula.item_id].emplace_back(&formula);
    }

    for (auto& [_, formulas] : m_formulas_by_item) {
        std::ranges::sort(formulas, [](const Formula* lhs, const Formula* rhs) {
            if (lhs->sort_id != rhs->sort_id) {
                return lhs->sort_id < rhs->sort_id;
            }
            return formula_sort_key(lhs->formula_id) < formula_sort_key(rhs->formula_id);
        });
    }

    return !m_formulas_by_id.empty();
}

bool InfrastMaterialCraftTask::build_plan()
{
    if (!load_formulas()) {
        return false;
    }

    CraftState state;
    state.inventory = m_inventory;

    std::unordered_set<std::string> stack;
    for (const auto& target : m_targets) {
        if (!craft_material(target.item_id, target.count, state, stack)) {
            callback_plan_failed(state);
            return false;
        }
    }

    m_plan = std::move(state.operations);
    m_plan_gold_cost = state.gold_cost;
    m_plan_ap_cost = state.ap_cost;
    callback_plan();
    if (!state.missing.empty()) {
        Log.warn(__FUNCTION__, "| material may be insufficient, will still try crafting");
        callback_plan_failed(state);
    }

    return !m_plan.empty();
}

bool InfrastMaterialCraftTask::craft_material(
    const std::string& item_id,
    int count,
    CraftState& state,
    std::unordered_set<std::string>& stack)
{
    if (count <= 0) {
        return true;
    }
    if (stack.contains(item_id)) {
        state.add_missing(item_id, count);
        return false;
    }
    if (!m_formulas_by_item.contains(item_id)) {
        state.add_missing(item_id, count);
        return false;
    }

    const auto& formulas = m_formulas_by_item.at(item_id);
    CraftState best_failure;
    bool has_failure = false;

    stack.emplace(item_id);
    for (const Formula* formula : formulas) {
        CraftState candidate = state;
        if (apply_formula(*formula, count, candidate, stack)) {
            state = std::move(candidate);
            stack.erase(item_id);
            return true;
        }

        if (!has_failure || candidate.missing_total() < best_failure.missing_total()) {
            best_failure = std::move(candidate);
            has_failure = true;
        }
    }
    stack.erase(item_id);

    if (has_failure) {
        state.missing = std::move(best_failure.missing);
    }
    else {
        state.add_missing(item_id, count);
    }
    return false;
}

bool InfrastMaterialCraftTask::apply_formula(
    const Formula& formula,
    int count,
    CraftState& state,
    std::unordered_set<std::string>& stack)
{
    const int batches = (count + formula.count - 1) / formula.count;
    for (const FormulaCost& cost : formula.costs) {
        consume_material(cost.item_id, cost.count * batches, state, stack);
    }

    state.inventory[formula.item_id] += formula.count * batches;
    state.gold_cost += static_cast<long long>(formula.gold_cost) * batches;
    state.ap_cost += static_cast<long long>(formula.ap_cost) * batches;
    append_operation(state, formula, batches);
    return true;
}

bool InfrastMaterialCraftTask::consume_material(
    const std::string& item_id,
    int count,
    CraftState& state,
    std::unordered_set<std::string>& stack)
{
    int available = state.get_inventory(item_id);
    const int used = std::min(available, count);
    if (used > 0) {
        state.inventory[item_id] = available - used;
    }

    const int shortage = count - used;
    if (shortage <= 0) {
        return true;
    }

    craft_material(item_id, shortage, state, stack);

    available = state.get_inventory(item_id);
    const int crafted_used = std::min(available, shortage);
    if (crafted_used > 0) {
        state.inventory[item_id] = available - crafted_used;
    }
    return true;
}

void InfrastMaterialCraftTask::append_operation(CraftState& state, const Formula& formula, int batches) const
{
    if (batches <= 0) {
        return;
    }
    if (!state.operations.empty() && state.operations.back().formula == &formula) {
        state.operations.back().batches += batches;
        return;
    }
    state.operations.emplace_back(CraftOperation { &formula, batches });
}

void InfrastMaterialCraftTask::callback_plan()
{
    json::value info = basic_info_with_what("MaterialCraftPlan");
    auto& details = info["details"];

    json::array operations;
    for (const CraftOperation& op : m_plan) {
        operations.emplace_back(
            json::object {
                { "formula_id", op.formula->formula_id },
                { "item_id", op.formula->item_id },
                { "item_name", item_display_name(op.formula->item_id) },
                { "batches", op.batches },
                { "count", op.formula->count * op.batches },
            });
    }

    details["operations"] = std::move(operations);
    details["gold_cost"] = m_plan_gold_cost;
    details["ap_cost"] = m_plan_ap_cost;
    details["mood_cost"] = m_plan_ap_cost / WorkshopApCostPerMood;
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void InfrastMaterialCraftTask::callback_plan_failed(const CraftState& state)
{
    json::value info = basic_info_with_what("MaterialCraftPlanFailed");
    auto& details = info["details"];

    json::array missing;
    for (const auto& [item_id, count] : state.missing) {
        missing.emplace_back(
            json::object {
                { "item_id", item_id },
                { "item_name", item_display_name(item_id) },
                { "count", count },
            });
    }
    details["missing"] = std::move(missing);
    callback(AsstMsg::SubTaskExtraInfo, info);
}

bool InfrastMaterialCraftTask::_run()
{
    LogTraceFunction;

    if (!build_plan()) {
        return false;
    }

    if (!ensure_craft_page()) {
        return false;
    }

    for (const CraftOperation& operation : m_plan) {
        if (need_exit()) {
            return false;
        }
        if (!execute_operation(operation)) {
            return false;
        }
    }

    return true;
}

bool InfrastMaterialCraftTask::ensure_craft_page()
{
    for (int i = 0; i != 3; ++i) {
        cv::Mat image = ctrler()->get_image();
        if (is_craft_page(image)) {
            return true;
        }

        auto entrance = match_workshop_template(image, "WorkshopCraftEntrance.png", 0.76);
        if (entrance) {
            ctrler()->click(*entrance);
            sleep(500);
            continue;
        }

        if (is_formula_selector(image)) {
            return true;
        }

        save_img(utils::path("debug") / utils::path("material_craft") / utils::path("ensure_craft_page"));
        sleep(250);
    }

    return false;
}

bool InfrastMaterialCraftTask::is_craft_page(const cv::Mat& image) const
{
    const bool has_slot = match_workshop_template(image, "WorkshopFormulaSlotSelected.png", 0.70).has_value() ||
                          match_workshop_template(image, "WorkshopFormulaSlotEmpty.png", 0.76).has_value();
    const bool has_stepper = match_workshop_template(image, "WorkshopPlusButton.png", 0.80).has_value() ||
                             match_workshop_template(image, "WorkshopMinusButton.png", 0.80).has_value();
    if (has_slot && has_stepper) {
        return true;
    }

    return false;
}

bool InfrastMaterialCraftTask::is_formula_selector(const cv::Mat& image) const
{
    const Rect category_roi(0, 70, std::min(image.cols, 260), std::max(1, image.rows - 70));
    auto category = match_workshop_template(image, "WorkshopEliteCategory.png", 0.60, category_roi, false);
    if (category && is_elite_category_rect(image, *category)) {
        return true;
    }

    const Rect dropdown_roi = quality_dropdown_roi(image);
    if (match_workshop_template(image, std::string(WorkshopQualityDropdownArrowDownTemplate), 0.70, dropdown_roi) ||
        match_workshop_template(image, std::string(WorkshopQualityDropdownArrowUpTemplate), 0.70, dropdown_roi) ||
        is_quality_menu_open(image)) {
        return true;
    }
    return false;

#if 0
    const Rect top_roi(image.cols * 60 / 100, 0, image.cols * 40 / 100, std::min(image.rows, 90));
    bool has_quality = false;
    bool has_sequence = false;
    for (const TextRect& text : find_all_text(image, top_roi)) {
        has_quality = has_quality || text.text == "品质";
        has_sequence = has_sequence || text.text == "序列";
    }
    return has_quality && has_sequence;
#endif
}

bool InfrastMaterialCraftTask::is_obtain_items_page(const cv::Mat& image) const
{
    Matcher matcher(image);
    matcher.set_task_info("MaterialCraft-ObtainItemsIcon");
    return matcher.analyze().has_value();
}

bool InfrastMaterialCraftTask::execute_operation(const CraftOperation& operation)
{
    const Formula& formula = *operation.formula;
    Log.info(
        __FUNCTION__,
        "| formula",
        formula.formula_id,
        item_display_name(formula.item_id),
        "batches",
        operation.batches);

    if (!open_formula_selector()) {
        return false;
    }
    if (!select_formula(formula)) {
        return false;
    }
    if (!set_craft_count(operation.batches)) {
        return false;
    }
    if (!click_start_button()) {
        return false;
    }
    return click_complete_tick();
}

bool InfrastMaterialCraftTask::open_formula_selector()
{
    for (int i = 0; i != 5; ++i) {
        cv::Mat image = ctrler()->get_image();
        if (is_obtain_items_page(image)) {
            auto complete_tick =
                match_workshop_template(image, "WorkshopCompleteTick.png", 0.68, complete_tick_roi(image));
            if (complete_tick) {
                ctrler()->click(center_of(*complete_tick));
                sleep(1200);
                continue;
            }
        }

        if (is_formula_selector(image)) {
            return true;
        }

        if (!is_craft_page(image)) {
            save_img(utils::path("debug") / utils::path("material_craft") / utils::path("open_formula_selector"));
            sleep(500);
            continue;
        }

        auto slot = match_workshop_template(image, "WorkshopFormulaSlotSelected.png", 0.74);
        if (!slot) {
            slot = match_workshop_template(image, "WorkshopFormulaSlotEmpty.png", 0.80);
        }
        if (slot) {
            ctrler()->click(center_of(*slot));
            sleep(500);
            image = ctrler()->get_image();
            if (is_formula_selector(image)) {
                return true;
            }
            Log.warn(__FUNCTION__, "| formula selector did not open after slot click");
            continue;
        }

        auto entrance = match_workshop_template(image, "WorkshopCraftEntrance.png", 0.76);
        if (entrance) {
            ctrler()->click(*entrance);
            sleep(500);
            continue;
        }

        save_img(utils::path("debug") / utils::path("material_craft") / utils::path("open_formula_selector"));
        sleep(500);
    }

    return false;
}

bool InfrastMaterialCraftTask::select_formula(const Formula& formula)
{
    const int page_limit = max_formula_pages(formula);
    bool force_rewind = false;

    for (int attempt = 0; attempt != FormulaSelectMaxAttempts; ++attempt) {
        if (!prepare_formula_selector(formula)) {
            return false;
        }
        if (force_rewind && rewind_formula_list_to_top()) {
            sleep(500);
        }
        force_rewind = false;

        FormulaScanResult result = scan_formula_pages(formula, page_limit);
        if (result == FormulaScanResult::Selected) {
            return true;
        }
        if (result == FormulaScanResult::NeedReselect) {
            Log.warn(__FUNCTION__, "| selected formula mismatch, reselect with quality filter", formula.formula_id);
            force_rewind = true;
            continue;
        }
        if (result == FormulaScanResult::VerificationFailed) {
            return false;
        }

        Log.warn(__FUNCTION__, "| formula not found, rewind list to top and retry");
        if (rewind_formula_list_to_top()) {
            sleep(500);
            result = scan_formula_pages(formula, page_limit);
            if (result == FormulaScanResult::Selected) {
                return true;
            }
            if (result == FormulaScanResult::NeedReselect) {
                Log.warn(
                    __FUNCTION__,
                    "| selected formula mismatch after rewind, reselect with quality filter",
                    formula.formula_id);
                force_rewind = true;
                continue;
            }
            if (result == FormulaScanResult::VerificationFailed) {
                return false;
            }
        }

        break;
    }

    save_img(utils::path("debug") / utils::path("material_craft") / utils::path("select_formula_failed"));
    return false;
}

bool InfrastMaterialCraftTask::prepare_formula_selector(const Formula& formula)
{
    m_formula_list_roi = Rect();
    if (!click_elite_category()) {
        save_img(utils::path("debug") / utils::path("material_craft") / utils::path("select_formula_category_failed"));
        return false;
    }
    if (!select_quality_filter(formula)) {
        save_img(utils::path("debug") / utils::path("material_craft") / utils::path("select_formula_quality_failed"));
        return false;
    }

    cv::Mat image = ctrler()->get_image();
    m_formula_list_roi = formula_list_roi(image);
    return true;
}

bool InfrastMaterialCraftTask::click_elite_category()
{
    cv::Mat image = ctrler()->get_image();
    const Rect category_roi(0, 70, std::min(image.cols, 260), std::max(1, image.rows - 70));
    auto category = match_workshop_template(image, "WorkshopEliteCategory.png", 0.60, category_roi, false);
    if (category && is_elite_category_rect(image, *category)) {
        ctrler()->click(center_of(*category));
        sleep(500);
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
    const std::string quality_template(quality_option_template_by_gold_cost(formula.gold_cost));

    for (int attempt = 0; attempt != 3; ++attempt) {
        if (!open_quality_menu()) {
            Log.warn(__FUNCTION__, "| quality menu not opened", quality_template);
            sleep(250);
            continue;
        }

        cv::Mat image = ctrler()->get_image();
        auto option = match_workshop_template(image, quality_template, 0.68, quality_menu_roi(image));
        if (!option) {
            Log.warn(__FUNCTION__, "| quality option template not found", quality_template);
            sleep(250);
            continue;
        }

        ctrler()->click(center_of(*option));
        sleep(500);

        if (close_quality_menu()) {
            return true;
        }

        Log.warn(__FUNCTION__, "| quality menu not closed", quality_template);
        sleep(250);
    }

    return false;
}

bool InfrastMaterialCraftTask::open_quality_menu()
{
    for (int i = 0; i != 3; ++i) {
        cv::Mat image = ctrler()->get_image();
        if (is_quality_menu_open(image)) {
            return true;
        }

        auto arrow = match_workshop_template(
            image,
            std::string(WorkshopQualityDropdownArrowDownTemplate),
            0.70,
            quality_dropdown_roi(image));
        if (!arrow) {
            sleep(250);
            continue;
        }

        ctrler()->click(center_of(*arrow));
        sleep(500);
    }

    return is_quality_menu_open(ctrler()->get_image());
}

bool InfrastMaterialCraftTask::close_quality_menu()
{
    for (int i = 0; i != 3; ++i) {
        cv::Mat image = ctrler()->get_image();
        if (!is_quality_menu_open(image)) {
            return true;
        }

        auto arrow = match_workshop_template(
            image,
            std::string(WorkshopQualityDropdownArrowUpTemplate),
            0.70,
            quality_dropdown_roi(image));
        if (!arrow) {
            sleep(250);
            continue;
        }

        ctrler()->click(center_of(*arrow));
        sleep(500);
    }

    return !is_quality_menu_open(ctrler()->get_image());
}

bool InfrastMaterialCraftTask::is_quality_menu_open(const cv::Mat& image) const
{
    const Rect menu_roi = quality_menu_roi(image);
    return std::ranges::any_of(WorkshopQualityOptionTemplates, [&](std::string_view option_template) {
        return match_workshop_template(image, std::string(option_template), 0.68, menu_roi).has_value();
    });
}

int InfrastMaterialCraftTask::max_formula_pages(const Formula& formula) const
{
    if (formula.gold_cost == 300) {
        return 16;
    }
    return 1;
}

InfrastMaterialCraftTask::FormulaScanResult
    InfrastMaterialCraftTask::scan_formula_pages(const Formula& formula, int page_limit)
{
    for (int page = 0; page != page_limit; ++page) {
        const FormulaScanResult result = scan_and_click_formula(formula);
        if (result == FormulaScanResult::Selected) {
            sleep(400);
            return FormulaScanResult::Selected;
        }
        if (result == FormulaScanResult::NeedReselect) {
            return FormulaScanResult::NeedReselect;
        }
        if (result == FormulaScanResult::VerificationFailed) {
            return FormulaScanResult::VerificationFailed;
        }

        if (page + 1 == page_limit) {
            break;
        }

        cv::Mat image = ctrler()->get_image();
        if (!swipe_formula_list(image, true, FormulaListScanSwipePercent, FormulaListScanSwipeDuration)) {
            break;
        }
        sleep(FormulaListScanSwipeSettleDelay);
    }

    return FormulaScanResult::NotFound;
}

InfrastMaterialCraftTask::FormulaScanResult InfrastMaterialCraftTask::scan_and_click_formula(const Formula& formula)
{
    cv::Mat image = ctrler()->get_image();
    auto scan_matches = [&](const std::vector<double>& scales) {
        InfrastMaterialCraftImageAnalyzer analyzer(image, formula_list_roi(image));
        analyzer.set_item_id(formula.item_id);
        analyzer.set_threshold(FormulaListProductThreshold);
        analyzer.set_scales(scales);
        if (!analyzer.analyze()) {
            return std::vector<InfrastMaterialCraftImageAnalyzer::FormulaMatch>();
        }
        return analyzer.get_result();
    };

    std::vector<InfrastMaterialCraftImageAnalyzer::FormulaMatch> matches = scan_matches({ 1.30 });
    if (matches.empty()) {
        matches = scan_matches({ 1.20, 1.40 });
    }
    if (matches.empty()) {
        return FormulaScanResult::NotFound;
    }

    for (const auto& formula_match : matches) {
        for (int click_count = 0; click_count != 3; ++click_count) {
            ctrler()->click(center_of(formula_match.click_rect));
            sleep(500);

            image = ctrler()->get_image();
            if (is_craft_page(image)) {
                if (selected_formula_matches(formula)) {
                    return FormulaScanResult::Selected;
                }

                Log.warn(__FUNCTION__, "| selected formula verification failed", formula.formula_id, formula.item_id);
                save_img(utils::path("debug") / utils::path("material_craft") / utils::path("select_formula_mismatch"));
                return FormulaScanResult::VerificationFailed;
            }
            if (!is_formula_selector(image)) {
                sleep(250);
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
                    return FormulaScanResult::VerificationFailed;
                }
            }
        }
    }

    return FormulaScanResult::NotFound;
}

bool InfrastMaterialCraftTask::selected_formula_matches(const Formula& formula) const
{
    for (int i = 0; i != 3; ++i) {
        const cv::Mat image = ctrler()->get_image();

        InfrastMaterialCraftImageAnalyzer analyzer(image, clamp_rect(selected_formula_product_roi(image), image));
        analyzer.set_item_id(formula.item_id);
        analyzer.set_threshold(SelectedFormulaProductThreshold);
        analyzer.set_filter_product_columns(false);
        analyzer.set_scales(
            { 0.80, 0.85, 0.95, 1.0, 1.05, 1.10, 1.15, 1.20, 1.25, 1.30, 1.35, 1.40, 1.45, 1.50, 1.60 });
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
                match.product_rect);
            return true;
        }

        sleep(200);
    }

    return false;
}

bool InfrastMaterialCraftTask::rewind_formula_list(int swipe_times)
{
    bool swiped = false;
    for (int i = 0; i != swipe_times; ++i) {
        cv::Mat image = ctrler()->get_image();
        if (!swipe_formula_list(image, false, FormulaListRewindSwipePercent, FormulaListRewindSwipeDuration)) {
            return swiped;
        }
        swiped = true;
        sleep(800);
    }
    return swiped;
}

bool InfrastMaterialCraftTask::rewind_formula_list_to_top()
{
    return rewind_formula_list(5);
}

bool
    InfrastMaterialCraftTask::swipe_formula_list(const cv::Mat& image, bool forward, int distance_percent, int duration)
{
    Rect box = formula_list_roi(image);
    if (box.empty()) {
        return false;
    }

    distance_percent = std::clamp(distance_percent, 1, 90);
    duration = std::max(100, duration);
    const int distance = std::max(84, box.height * distance_percent / 100);
    const int from_y = forward ? box.y + box.height * 66 / 100 : box.y + box.height * 34 / 100;
    const int to_y = std::clamp(forward ? from_y - distance : from_y + distance, box.y + 10, box.y + box.height - 10);
    Rect from(center_of(box).x - 10, from_y, 20, 20);
    Rect to(center_of(box).x - 10, to_y, 20, 20);
    return ctrler()->swipe(from, to, duration, false);
}

bool InfrastMaterialCraftTask::set_craft_count(int batches)
{
    if (batches <= 0) {
        return false;
    }
    if (batches == 1) {
        return verify_craft_count(1);
    }

    cv::Mat image = ctrler()->get_image();
    auto button = match_workshop_template(image, "WorkshopPlusButton.png", 0.84, plus_button_roi(image));
    if (!button) {
        save_img(utils::path("debug") / utils::path("material_craft") / utils::path("plus_button_failed"));
        return false;
    }

    const Point click_point = safe_plus_click_point(*button);
    for (int i = 1; i != batches; ++i) {
        ctrler()->click(click_point);
        sleep(100);
    }

    return verify_craft_count(batches);
}

bool InfrastMaterialCraftTask::verify_craft_count(int expected)
{
    for (int i = 0; i != 5; ++i) {
        const cv::Mat image = ctrler()->get_image();
        const auto count = read_craft_count(image);
        if (count && *count == expected) {
            Log.trace(__FUNCTION__, "| craft count verified", expected);
            return true;
        }

        if (count) {
            Log.error(__FUNCTION__, "| craft count mismatch, expected", expected, "actual", *count);
            save_img(utils::path("debug") / utils::path("material_craft") / utils::path("craft_count_mismatch"));
            return false;
        }

        sleep(200);
    }

    Log.error(__FUNCTION__, "| failed to read craft count, expected", expected);
    save_img(utils::path("debug") / utils::path("material_craft") / utils::path("craft_count_ocr_failed"));
    return false;
}

std::optional<int> InfrastMaterialCraftTask::read_craft_count(const cv::Mat& image) const
{
    OCRer ocr(image, clamp_rect(craft_count_roi(image), image));
    const auto results = ocr.analyze();
    if (!results) {
        return std::nullopt;
    }

    std::optional<TextRect> best_match;
    for (const TextRect& result : results.value()) {
        auto count = parse_count_text(result.text);
        if (!count) {
            continue;
        }

        Log.trace(__FUNCTION__, "| craft count OCR candidate", result.text, "count", *count, "rect", result.rect);

        if (!best_match || result.rect.area() > best_match->rect.area()) {
            best_match = result;
        }
    }

    if (!best_match) {
        return std::nullopt;
    }

    return parse_count_text(best_match->text);
}

bool InfrastMaterialCraftTask::click_start_button()
{
    for (int i = 0; i != 5; ++i) {
        cv::Mat image = ctrler()->get_image();
        auto start = match_workshop_template(image, "WorkshopStartButton.png", 0.80);
        if (start) {
            ctrler()->click(*start);
            sleep(500);
            return true;
        }
        sleep(250);
    }
    save_img(utils::path("debug") / utils::path("material_craft") / utils::path("start_button_failed"));
    return false;
}

bool InfrastMaterialCraftTask::click_complete_tick()
{
    bool has_obtained_items = false;
    for (int i = 0; i != 20; ++i) {
        cv::Mat image = ctrler()->get_image();
        if (has_obtained_items && (is_craft_page(image) || is_formula_selector(image))) {
            return true;
        }

        if (!is_obtain_items_page(image)) {
            sleep(250);
            continue;
        }

        has_obtained_items = true;
        auto tick = match_workshop_template(image, "WorkshopCompleteTick.png", 0.68, complete_tick_roi(image));
        if (tick) {
            ctrler()->click(center_of(*tick));
            sleep(1200);
            continue;
        }
        sleep(250);
    }
    save_img(utils::path("debug") / utils::path("material_craft") / utils::path("complete_tick_failed"));
    return false;
}

std::optional<Rect> InfrastMaterialCraftTask::match_workshop_template(
    const cv::Mat& image,
    const std::string& filename,
    double threshold,
    const Rect& roi,
    bool mask) const
{
    static std::unordered_map<std::string, cv::Mat> templ_cache;
    auto iter = templ_cache.find(filename);
    if (iter == templ_cache.end()) {
        const auto path = ResDir.get() / "template"_p / "InfrastPic"_p / "Processing"_p / utils::path(filename);
        cv::Mat templ = MAA_NS::imread(path);
        if (templ.empty()) {
            Log.error(__FUNCTION__, "| template empty", path);
            return std::nullopt;
        }
        iter = templ_cache.emplace(filename, std::move(templ)).first;
    }

    Matcher matcher(image, roi.empty() ? image_rect(image) : clamp_rect(roi, image));
    matcher.set_templ(iter->second);
    matcher.set_threshold(threshold);
    matcher.set_method(MatchMethod::Ccoeff);
    if (mask) {
        matcher.set_mask_range(1, 255, false, true);
    }

    if (!matcher.analyze()) {
        return std::nullopt;
    }
    return matcher.get_result().rect;
}

std::vector<TextRect> InfrastMaterialCraftTask::find_all_text(const cv::Mat& image, const Rect& roi) const
{
    OCRer ocr(image, roi.empty() ? image_rect(image) : clamp_rect(roi, image));
    if (!ocr.analyze()) {
        return {};
    }
    return ocr.get_result();
}

Rect InfrastMaterialCraftTask::formula_list_roi(const cv::Mat& image)
{
    if (!m_formula_list_roi.empty()) {
        return clamp_rect(m_formula_list_roi, image);
    }

    const int left = std::min(image.cols * 17 / 100, image.cols - 1);
    const int top = std::min(80, image.rows - 1);
    m_formula_list_roi = clamp_rect(Rect(left, top, image.cols - left, image.rows - top), image);
    return m_formula_list_roi;
}

Rect InfrastMaterialCraftTask::image_rect(const cv::Mat& image) const
{
    return Rect(0, 0, image.cols, image.rows);
}

Rect InfrastMaterialCraftTask::clamp_rect(const Rect& rect, const cv::Mat& image) const
{
    if (rect.empty()) {
        return image_rect(image);
    }

    int x = std::clamp(rect.x, 0, std::max(0, image.cols - 1));
    int y = std::clamp(rect.y, 0, std::max(0, image.rows - 1));
    int right = std::clamp(rect.x + rect.width, x + 1, image.cols);
    int bottom = std::clamp(rect.y + rect.height, y + 1, image.rows);
    return Rect(x, y, right - x, bottom - y);
}
