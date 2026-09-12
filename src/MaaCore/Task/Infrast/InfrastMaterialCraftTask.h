#pragma once

#include "Task/Infrast/InfrastAbstractTask.h"
#include "Utils/MaterialCraftPlanner.h"

#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace cv
{
class Mat;
}

namespace asst
{
class InfrastMaterialCraftTask final : public InfrastAbstractTask
{
public:
    InfrastMaterialCraftTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
    virtual ~InfrastMaterialCraftTask() override = default;

    virtual bool set_params(const json::value& params);

    virtual std::string facility_name() const override { return m_facility; }

protected:
    virtual bool _run() override;

    virtual bool on_run_fails() override { return false; }

private:
    using Formula = MaterialFormula;
    using CraftOperation = MaterialCraftOperation;

    enum class FormulaScanResult
    {
        NotFound,
        Selected,
        VerificationFailed,
        Cancelled,
    };

    bool build_plan();

    struct ManufacturingRecipe
    {
        std::string item_id;
        std::string ingredient_id;
        int batches = 0;
        int weight = 1;
    };

    bool execute_manufacturing_operation(const CraftOperation& operation);
    bool ensure_manufacturing_page();
    bool enter_manufacturing_facility(int index);
    bool leave_manufacturing_page();
    bool is_manufacturing_page(const cv::Mat& image) const;
    std::optional<ManufacturingRecipe> read_manufacturing_recipe() const;
    std::optional<std::pair<int, int>>
        read_manufacturing_number(const std::string& task_name, bool fraction = false) const;
    bool manufacturing_product_matches(const std::string& item_id, const cv::Mat& image, bool allow_completed = false)
        const;
    bool select_manufacturing_recipe(const ManufacturingRecipe& recipe);
    bool set_manufacturing_count(int count);
    bool confirm_manufacturing_recipe(const ManufacturingRecipe& recipe);
    bool collect_manufacturing_product(const ManufacturingRecipe& recipe, int count);
    bool manufacturing_action(const std::string& task_name);
    void manufacturing_failure(const std::string& reason, const ManufacturingRecipe& original = {});
    std::string m_facility = "Processing";
    bool m_replenish_originium_shards = false;

    bool ensure_processing_room();
    bool craft_sleep(unsigned milliseconds) const;
    bool ensure_craft_page();
    bool is_processing_room(const cv::Mat& image) const;
    bool is_craft_page(const cv::Mat& image) const;
    bool is_formula_selector(const cv::Mat& image) const;
    bool is_obtain_items_page(const cv::Mat& image) const;
    bool execute_operation(const CraftOperation& operation);
    bool open_formula_selector();
    bool select_formula(const Formula& formula);
    bool prepare_formula_selector(const Formula& formula);
    bool click_formula_category(const Formula& formula);
    bool select_quality_filter(const Formula& formula);
    bool open_quality_menu();
    bool close_quality_menu();
    bool is_quality_menu_open(const cv::Mat& image) const;
    int max_formula_pages(const Formula& formula) const;
    FormulaScanResult scan_formula_pages(const Formula& formula, int page_limit);
    FormulaScanResult scan_and_click_formula(const Formula& formula);
    bool selected_formula_matches(const Formula& formula) const;
    bool rewind_formula_list(int swipe_times);
    bool rewind_formula_list_to_top();
    bool swipe_formula_list(bool forward);
    std::optional<int> read_craft_count() const;
    std::optional<int> set_craft_count(int batches);
    bool click_start_button();
    bool click_complete_tick(const CraftOperation& operation, int operation_id);
    void callback_operation(const std::string& what, const CraftOperation& operation, int operation_id);
    int m_next_operation_id = 0;
    bool m_inventory_complete = false;

    std::optional<Rect> match_workshop_template(const cv::Mat& image, const std::string& task_name) const;

    MaterialCraftRequest m_request;
    MaterialCraftPlan m_plan;
};
}
