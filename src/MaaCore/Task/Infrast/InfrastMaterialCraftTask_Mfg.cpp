#include "InfrastMaterialCraftTask.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/WorkingDir.hpp"
#include "Vision/Infrast/InfrastFacilityImageAnalyzer.h"
#include "Vision/Infrast/InfrastMaterialCraftImageAnalyzer.h"
#include "Vision/Miscellaneous/MaterialImageAnalyzer.h"
#include "Vision/RegionOCRer.h"

using namespace asst;

bool InfrastMaterialCraftTask::manufacturing_action(const std::string& task_name)
{
    return !need_exit() && ProcessTask(*this, { task_name }).set_retry_times(2).run() && !need_exit();
}

bool InfrastMaterialCraftTask::is_manufacturing_page(const cv::Mat& image) const
{
    return match_workshop_template(image, "MaterialCraft-MfgRoom") &&
           match_workshop_template(image, "MaterialCraft-MfgProductPanel");
}

std::optional<std::pair<int, int>>
    InfrastMaterialCraftTask::read_manufacturing_number(const std::string& task_name, bool fraction) const
{
    std::optional<std::pair<int, int>> previous;
    for (int attempt = 0; attempt < 3 && !need_exit(); ++attempt) {
        const auto image = ctrler()->get_image();
        if (!is_manufacturing_page(image)) {
            return std::nullopt;
        }
        RegionOCRer ocr(image);
        ocr.set_task_info(task_name);
        const auto result = ocr.analyze();
        if (!result || result->score < 0.95) {
            previous.reset();
            continue;
        }
        auto number = [](std::string_view text) -> std::optional<int> {
            if (text.empty() || text.find_first_not_of("0123456789") != text.npos) {
                return std::nullopt;
            }
            int value = 0;
            const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
            if (error != std::errc() || end != text.data() + text.size()) {
                return std::nullopt;
            }
            return value;
        };
        const auto slash = result->text.find('/');
        const auto left = number(std::string_view(result->text).substr(0, slash));
        const auto right = fraction && slash != std::string::npos
                               ? number(std::string_view(result->text).substr(slash + 1))
                               : std::optional<int>(0);
        if (!left || !right || (fraction && (slash == std::string::npos || *left > *right || *right == 0)) ||
            (!fraction && slash != std::string::npos)) {
            return std::nullopt;
        }
        const std::pair value { *left, *right };
        if (previous == value) {
            return value;
        }
        previous = value;
        if (!craft_sleep(100)) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

bool InfrastMaterialCraftTask::manufacturing_product_matches(
    const std::string& item_id,
    const cv::Mat& image,
    bool allow_completed) const
{
    if (!is_manufacturing_page(image)) {
        return false;
    }
    const bool is_chip = item_id.starts_with("32");
    RegionOCRer name(image);
    name.set_task_info(is_chip ? "MaterialCraft-MfgChipName" : "MaterialCraft-MfgProductName");
    const auto text = name.analyze();
    if (!text || text->score < 0.90 || text->text != ItemData.get_item_name(item_id)) {
        return false;
    }
    // Completion covers the middle of the icon. Only post-submission/collection
    // checks may use the completed-state banner together with the exact name.
    // Selection and submission still require the unobstructed product icon.
    if (allow_completed && match_workshop_template(image, "MaterialCraft-MfgCompleted")) {
        return true;
    }
    MaterialImageAnalyzer analyzer(image);
    analyzer.set_item_id(item_id);
    analyzer.set_task_info(is_chip ? "MaterialCraft-MfgSelectedChip" : "MaterialCraft-MfgSelectedProduct");
    analyzer.set_cancel_check([this] { return need_exit(); });
    return analyzer.analyze();
}

std::optional<InfrastMaterialCraftTask::ManufacturingRecipe> InfrastMaterialCraftTask::read_manufacturing_recipe() const
{
    const auto image = ctrler()->get_image();
    if (!is_manufacturing_page(image) || match_workshop_template(image, "MaterialCraft-MfgPending")) {
        return std::nullopt;
    }
    // A factory already making chips may have a reserved, partially completed order.
    // Use a regular production line instead of cancelling that order.
    const std::array<ManufacturingRecipe, 5> products = {
        ManufacturingRecipe { "2001", {}, 0, 2 },
        { "2002", {}, 0, 3 },
        { "2003", {}, 0, 5 },
        { "3003", {}, 0, 2 },
        { "3141", {}, 0, 3 },
    };
    std::optional<ManufacturingRecipe> found;
    for (auto product : products) {
        if (!manufacturing_product_matches(product.item_id, image, true)) {
            continue;
        }
        if (found) {
            return std::nullopt;
        }
        if (product.item_id == "3141") {
            // Originium shards have two recipes with the same product icon.
            for (const auto ingredient : { "30012", "30062" }) {
                MaterialImageAnalyzer analyzer(image);
                analyzer.set_item_id(ingredient);
                analyzer.set_task_info("MaterialCraft-MfgIngredient");
                if (analyzer.analyze()) {
                    if (!product.ingredient_id.empty()) {
                        return std::nullopt;
                    }
                    product.ingredient_id = ingredient;
                }
            }
            if (product.ingredient_id.empty()) {
                return std::nullopt;
            }
        }
        found = std::move(product);
    }
    const auto count = read_manufacturing_number("MaterialCraft-MfgQuantity");
    if (!found || !count || count->first > 99) {
        return std::nullopt;
    }
    found->batches = count->first;
    return found;
}

bool InfrastMaterialCraftTask::enter_manufacturing_facility(int index)
{
    InfrastFacilityImageAnalyzer facilities(ctrler()->get_image());
    facilities.set_to_be_analyzed({ "Mfg" });
    if (need_exit() || !facilities.analyze()) {
        return false;
    }
    const auto bar = facilities.get_rect("Mfg", index);
    // The facility analyzer locates the narrow colored strip, which is not
    // clickable in every base view. Click inside the adjacent room instead.
    if (bar.empty() || !ctrler()->click(Rect { bar.x + 30, bar.y + bar.height / 2 - 16, 32, 32 })) {
        return false;
    }
    m_cur_facility_index = index;
    callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("EnterFacility"));
    return craft_sleep(Task.get("InfrastEnterFacility")->post_delay);
}

bool InfrastMaterialCraftTask::ensure_manufacturing_page()
{
    auto positions = [&] {
        InfrastFacilityImageAnalyzer facilities(ctrler()->get_image());
        facilities.set_to_be_analyzed({ "Mfg" });
        std::vector<Rect> result;
        if (facilities.analyze()) {
            for (int i = 0; i < static_cast<int>(facilities.get_quantity("Mfg")); ++i) {
                result.push_back(facilities.get_rect("Mfg", i));
            }
        }
        return result;
    };
    auto overview = [&] {
        if (!leave_manufacturing_page() || !ProcessTask(*this, { "MaterialCraft@InfrastBegin" }).run() || need_exit()) {
            return false;
        }
        auto previous = positions();
        // A jump from an operator page can leave the base horizontally offset.
        // Do not count a partially visible set of factories as the entire base.
        for (int attempt = 0; attempt < 3 && !need_exit(); ++attempt) {
            swipe_to_the_left_of_main_ui();
            const auto current = positions();
            if (!current.empty() && current.size() == previous.size() &&
                std::equal(current.begin(), current.end(), previous.begin(), [](const Rect& lhs, const Rect& rhs) {
                    return std::abs(lhs.x - rhs.x) <= 3 && std::abs(lhs.y - rhs.y) <= 3;
                })) {
                return !need_exit();
            }
            previous = current;
        }
        return false;
    };
    if (need_exit()) {
        return false;
    }
    // Start from the overview so the recovery record has an identified facility index.
    if (!ProcessTask(*this, { "MaterialCraft-LeaveTraining" }).run() || !overview()) {
        return false;
    }
    InfrastFacilityImageAnalyzer facilities(ctrler()->get_image());
    facilities.set_to_be_analyzed({ "Mfg" });
    if (!facilities.analyze()) {
        manufacturing_failure("NoAvailableFactory");
        return false;
    }
    const int count = static_cast<int>(facilities.get_quantity("Mfg"));
    int originium_index = -1;
    bool all_originium = true;
    for (int index = 0; index < count && !need_exit(); ++index) {
        if (index > 0 && !overview()) {
            return false;
        }
        if (!enter_manufacturing_facility(index) ||
            !match_workshop_template(ctrler()->get_image(), "MaterialCraft-MfgRoom") || !click_bottom_left_tab()) {
            all_originium = false;
            continue;
        }
        const auto recipe = read_manufacturing_recipe();
        if (!recipe) {
            // An unreadable room (or an existing chip order) is not evidence
            // that every factory is making Originium shards.
            all_originium = false;
            continue;
        }
        const bool level_three = match_workshop_template(ctrler()->get_image(), "MaterialCraft-MfgLevel3").has_value();
        if (recipe->item_id != "3141") {
            all_originium = false;
            if (level_three) {
                return true;
            }
        }
        else if (level_three && originium_index < 0) {
            originium_index = index;
        }
    }
    // Only fall back to shards after inspecting all factories. Re-read the
    // selected room on return so its saved quantity is current, not from the scan.
    if (!need_exit() && all_originium && originium_index >= 0) {
        if (m_cur_facility_index != originium_index &&
            (!overview() || !enter_manufacturing_facility(originium_index) ||
             !match_workshop_template(ctrler()->get_image(), "MaterialCraft-MfgRoom") || !click_bottom_left_tab())) {
            return false;
        }
        const auto recipe = read_manufacturing_recipe();
        if (recipe && recipe->item_id == "3141" &&
            match_workshop_template(ctrler()->get_image(), "MaterialCraft-MfgLevel3")) {
            return true;
        }
    }
    manufacturing_failure("NoAvailableFactory");
    return false;
}

bool InfrastMaterialCraftTask::leave_manufacturing_page()
{
    if (match_workshop_template(ctrler()->get_image(), "MaterialCraft-MfgQuickMenu") &&
        !manufacturing_action("MaterialCraft-MfgCloseQuickMenu")) {
        return false;
    }
    if (match_workshop_template(ctrler()->get_image(), "MaterialCraft-MfgChipsCategory") &&
        !manufacturing_action("MaterialCraft-MfgBack")) {
        return false;
    }
    if (match_workshop_template(ctrler()->get_image(), "MaterialCraft-MfgPending")) {
        // Do not navigate through an uncommitted manufacturing edit.
        return false;
    }
    for (int back = 0; back < 2 && !need_exit(); ++back) {
        if (!match_workshop_template(ctrler()->get_image(), "MaterialCraft-MfgRoom")) {
            return true;
        }
        if (!manufacturing_action("MaterialCraft-MfgBack")) {
            return false;
        }
    }
    return !need_exit() && !match_workshop_template(ctrler()->get_image(), "MaterialCraft-MfgRoom");
}

bool InfrastMaterialCraftTask::select_manufacturing_recipe(const ManufacturingRecipe& recipe)
{
    if (!is_manufacturing_page(ctrler()->get_image()) || !manufacturing_action("MaterialCraft-MfgOpenSelector")) {
        return false;
    }
    const std::string category = recipe.item_id == "3003"            ? "Gold"
                                 : recipe.item_id == "3141"          ? "Originium"
                                 : recipe.item_id.starts_with("200") ? "Records"
                                                                     : "Chips";
    if (!manufacturing_action("MaterialCraft-Mfg" + category + "Category")) {
        return false;
    }
    // Eight chips occupy four rows; three rows fit on screen. Inspect the
    // current page first, then at most one swipe towards the requested half.
    const bool is_chip = recipe.item_id.starts_with("32");
    const int pages = is_chip ? 2 : 1;
    for (int page = 0; page < pages && !need_exit(); ++page) {
        const auto image = ctrler()->get_image();
        InfrastMaterialCraftImageAnalyzer analyzer(image);
        analyzer.set_task_info(is_chip ? "MaterialCraft-MfgFormulaChip" : "MaterialCraft-MfgFormulaProduct");
        analyzer.set_item_id(recipe.item_id);
        analyzer.set_cancel_check([this] { return need_exit(); });
        const bool found = is_chip ? analyzer.analyze_with_name(
                                         "MaterialCraft-MfgFormulaName",
                                         ItemData.get_item_name(recipe.item_id),
                                         0.90)
                                   : analyzer.analyze();
        if (found) {
            auto candidates = analyzer.get_result();
            std::ranges::sort(candidates, [](const auto& lhs, const auto& rhs) { return lhs.score > rhs.score; });
            for (const auto& candidate : candidates) {
                if (!recipe.ingredient_id.empty()) {
                    MaterialImageAnalyzer ingredient(image);
                    ingredient.set_item_id(recipe.ingredient_id);
                    ingredient.set_task_info("MaterialCraft-MfgFormulaProduct");
                    ingredient.set_scales({ 0.70, 0.75, 0.80, 0.85, 0.90 });
                    // Only inspect the costs belonging to this product card.
                    ingredient.set_roi(candidate.product_rect.move({ 160, 0, 280, 160 }));
                    if (!ingredient.analyze()) {
                        continue;
                    }
                }
                if (need_exit() || !ctrler()->click(candidate.click_rect) || !craft_sleep(500)) {
                    return false;
                }
                return manufacturing_product_matches(recipe.item_id, ctrler()->get_image());
            }
        }
        if (page + 1 == pages) {
            save_img(utils::path("debug") / utils::path("material_craft") / utils::path("manufacturing_formula"));
            break;
        }
        if (!manufacturing_action(recipe.item_id < "3253" ? "MaterialCraft-MfgRewind" : "MaterialCraft-MfgSwipe")) {
            return false;
        }
    }
    return false;
}

bool InfrastMaterialCraftTask::set_manufacturing_count(int count)
{
    if (count < 0 || count > 99 || !is_manufacturing_page(ctrler()->get_image())) {
        return false;
    }
    if (count == 99) {
        if (!manufacturing_action("MaterialCraft-MfgMaximum")) {
            return false;
        }
        const auto current = read_manufacturing_number("MaterialCraft-MfgQuantity");
        return current && current->first == count;
    }
    if (!manufacturing_action("MaterialCraft-MfgMinimum")) {
        return false;
    }
    auto current = read_manufacturing_number("MaterialCraft-MfgQuantity");
    if (!current || current->first > count) {
        return false;
    }
    while (current->first < count) {
        const int before = current->first;
        if (!manufacturing_action("MaterialCraft-MfgPlus")) {
            return false;
        }
        current = read_manufacturing_number("MaterialCraft-MfgQuantity");
        // A game-side material/capacity cap must not silently reduce the requested order.
        if (!current || current->first != before + 1 || current->first > count) {
            return false;
        }
    }
    return true;
}

bool InfrastMaterialCraftTask::confirm_manufacturing_recipe(const ManufacturingRecipe& recipe)
{
    auto image = ctrler()->get_image();
    const auto count = read_manufacturing_number("MaterialCraft-MfgQuantity");
    if (!count || count->first != recipe.batches || !manufacturing_product_matches(recipe.item_id, image)) {
        return false;
    }
    if (!recipe.ingredient_id.empty()) {
        MaterialImageAnalyzer ingredient(image);
        ingredient.set_task_info("MaterialCraft-MfgIngredient");
        ingredient.set_item_id(recipe.ingredient_id);
        if (!ingredient.analyze()) {
            return false;
        }
    }
    if (match_workshop_template(image, "MaterialCraft-MfgPending")) {
        if (!manufacturing_action("MaterialCraft-MfgPending") || !craft_sleep(500)) {
            return false;
        }
        image = ctrler()->get_image();
        if (match_workshop_template(image, "MaterialCraft-MfgFinalConfirm") &&
            !manufacturing_action("MaterialCraft-MfgFinalConfirm")) {
            return false;
        }
    }
    if (!craft_sleep(500)) {
        return false;
    }
    image = ctrler()->get_image();
    return manufacturing_product_matches(recipe.item_id, image, true) &&
           !match_workshop_template(image, "MaterialCraft-MfgPending") &&
           !match_workshop_template(image, "MaterialCraft-MfgFinalConfirm");
}

bool InfrastMaterialCraftTask::collect_manufacturing_product(const ManufacturingRecipe& recipe, int count)
{
    const auto stored = read_manufacturing_number("MaterialCraft-MfgStorage", true);
    if (!stored || stored->first != count * recipe.weight ||
        !manufacturing_product_matches(recipe.item_id, ctrler()->get_image(), true) ||
        !manufacturing_action("MaterialCraft-MfgCollect")) {
        return false;
    }
    // Collection animations can temporarily obscure the product header. Wait
    // for the page to settle and require a confirmed empty warehouse; never
    // click Collect again while acknowledgement of the first click is pending.
    for (int poll = 0; poll < 20 && !need_exit(); ++poll) {
        const auto after = read_manufacturing_number("MaterialCraft-MfgStorage", true);
        if (after && after->first == 0 && manufacturing_product_matches(recipe.item_id, ctrler()->get_image(), true)) {
            return true;
        }
        if (!craft_sleep(300)) {
            return false;
        }
    }
    return false;
}

void InfrastMaterialCraftTask::manufacturing_failure(const std::string& reason, const ManufacturingRecipe& original)
{
    Log.error(__FUNCTION__, reason, original.item_id, original.batches, m_cur_facility_index);
    save_img(utils::path("debug") / utils::path("material_craft") / utils::path("manufacturing"));
    auto info = basic_info_with_what(
        original.item_id.empty() ? "MaterialCraftManufacturingFailed" : "MaterialCraftManufacturingRestoreRequired");
    info["details"]["reason"] = reason;
    info["details"]["item_id"] = original.item_id;
    info["details"]["batches"] = original.batches;
    info["details"]["ingredient_id"] = original.ingredient_id;
    callback(AsstMsg::SubTaskExtraInfo, info);
}

bool InfrastMaterialCraftTask::execute_manufacturing_operation(const CraftOperation& operation)
{
    if (!ensure_manufacturing_page()) {
        return false;
    }
    auto original = read_manufacturing_recipe();
    auto storage = read_manufacturing_number("MaterialCraft-MfgStorage", true);
    if (!original || !storage || storage->first % original->weight != 0) {
        manufacturing_failure("OriginalRecipeUnrecognized");
        return false;
    }
    if (storage->first > 0) {
        const int collected = storage->first / original->weight;
        MaterialFormula product;
        product.item_id = original->item_id;
        product.facility = "Mfg";
        const CraftOperation collection { product, collected };
        const int operation_id = m_next_operation_id++;
        callback_operation("MaterialCraftOperationStarted", collection, operation_id);
        if (!collect_manufacturing_product(*original, collected)) {
            manufacturing_failure("CollectOriginalProductFailed");
            return false;
        }
        m_inventory_complete = true;
        callback_operation("MaterialCraftOperationCompleted", collection, operation_id);
        original = read_manufacturing_recipe();
        if (!original) {
            manufacturing_failure("OriginalRecipeUnrecognized");
            return false;
        }
    }
    auto restore = *original;
    if (restore.item_id == "3141" && m_replenish_originium_shards) {
        restore.batches = 99;
    }
    // Keep both the original quantity and intended restoration even if killed.
    const auto recovery_path = UserDir.get() / "debug" / "material_craft" /
                               ("manufacturing_restore_" + std::to_string(get_task_id()) + ".json");
    std::error_code error;
    std::filesystem::create_directories(recovery_path.parent_path(), error);
    std::ofstream recovery(recovery_path);
    recovery << json::object {
        { "item_id", original->item_id }, { "ingredient_id", original->ingredient_id },
        { "batches", restore.batches }, { "original_batches", original->batches },
        { "facility_index", m_cur_facility_index },
    }.format();
    recovery.close();
    if (!recovery) {
        manufacturing_failure("SaveOriginalRecipeFailed");
        return false;
    }

    int remaining = operation.batches;
    bool submitted = false;
    bool complete = true;
    while (remaining > 0 && !need_exit()) {
        // A dualchip occupies five storage slots. Never rely on auto-replenishment.
        int batches = std::min({ remaining, storage->second / 5, 99 });
        ManufacturingRecipe target { operation.formula.item_id, {}, batches, 5 };
        if (batches <= 0 || !select_manufacturing_recipe(target)) {
            complete = false;
            break;
        }
        // Operators can have product-dependent storage bonuses.
        storage = read_manufacturing_number("MaterialCraft-MfgStorage", true);
        if (!storage) {
            complete = false;
            break;
        }
        target.batches = batches = std::min(batches, storage->second / 5);
        if (batches <= 0 || !set_manufacturing_count(batches)) {
            complete = false;
            break;
        }
        CraftOperation actual { operation.formula, batches };
        const int operation_id = m_next_operation_id++;
        callback_operation("MaterialCraftOperationStarted", actual, operation_id);
        submitted = true;
        if (!confirm_manufacturing_recipe(target)) {
            manufacturing_failure("SubmitUnconfirmed", restore);
            return false;
        }
        bool finished = false;
        for (int poll = 0; poll < batches * 4 + 40 && !need_exit(); ++poll) {
            const auto quantity = read_manufacturing_number("MaterialCraft-MfgQuantity");
            const auto produced = read_manufacturing_number("MaterialCraft-MfgStorage", true);
            if (quantity && produced && quantity->first == 0 && produced->first == batches * 5) {
                finished = true;
                break;
            }
            if (!craft_sleep(250)) {
                break;
            }
        }
        if (!finished || !collect_manufacturing_product(target, batches)) {
            // Do not cancel a possibly unfinished order while trying to restore the original.
            manufacturing_failure("CompletionUnconfirmed", restore);
            return false;
        }
        m_inventory_complete = true;
        callback_operation("MaterialCraftOperationCompleted", actual, operation_id);
        remaining -= batches;
    }

    bool restored = false;
    if (!need_exit()) {
        if (match_workshop_template(ctrler()->get_image(), "MaterialCraft-MfgChipsCategory")) {
            manufacturing_action("MaterialCraft-MfgBack");
        }
        // Discard any unsubmitted edit before restoring a known original recipe.
        if (match_workshop_template(ctrler()->get_image(), "MaterialCraft-MfgPending")) {
            manufacturing_action("MaterialCraft-MfgCancel");
        }
        if (!submitted) {
            const auto current = read_manufacturing_recipe();
            restored = current && current->item_id == original->item_id &&
                       current->ingredient_id == original->ingredient_id && current->batches == original->batches;
        }
        else {
            restored = select_manufacturing_recipe(restore) && set_manufacturing_count(restore.batches) &&
                       confirm_manufacturing_recipe(restore);
            const auto current = restored ? read_manufacturing_recipe() : std::nullopt;
            restored = current && current->item_id == restore.item_id &&
                       current->ingredient_id == restore.ingredient_id && current->batches == restore.batches;
        }
    }
    if (!restored) {
        manufacturing_failure("RestoreUnconfirmed", submitted ? restore : *original);
        return false;
    }
    std::filesystem::remove(recovery_path, error);
    if (!complete || remaining != 0) {
        manufacturing_failure("RecipeOrQuantityUnavailable");
        return false;
    }
    return true;
}
