#pragma once

#include "Task/Infrast/InfrastAbstractTask.h"

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

    virtual std::string facility_name() const override { return "Processing"; }

protected:
    virtual bool _run() override;

    virtual bool on_run_fails() override { return false; }

private:
    struct CraftTarget
    {
        std::string item_id;
        int count = 0;
    };

    struct FormulaCost
    {
        std::string item_id;
        int count = 0;
    };

    struct Formula
    {
        std::string formula_id;
        int sort_id = 0;
        int rarity = 0;
        std::string item_id;
        std::string category;
        int count = 1;
        int gold_cost = 0;
        int ap_cost = 0;
        std::vector<FormulaCost> costs;
    };

    struct CraftOperation
    {
        const Formula* formula = nullptr;
        int batches = 0;
    };

    enum class FormulaScanResult
    {
        NotFound,
        Selected,
        NeedReselect,
        VerificationFailed,
    };

    struct CraftState
    {
        std::unordered_map<std::string, int> inventory;
        std::unordered_map<std::string, int> missing;
        std::vector<CraftOperation> operations;
        long long gold_cost = 0;
        long long ap_cost = 0;

        int get_inventory(const std::string& item_id) const;
        void add_missing(const std::string& item_id, int count);
        int missing_total() const;
    };

    bool parse_targets(const json::value& params);
    bool parse_inventory(const json::value& params);
    bool load_formulas();
    bool build_plan();
    bool craft_material(
        const std::string& item_id,
        int count,
        CraftState& state,
        std::unordered_set<std::string>& stack);
    bool apply_formula(const Formula& formula, int count, CraftState& state, std::unordered_set<std::string>& stack);
    bool consume_material(
        const std::string& item_id,
        int count,
        CraftState& state,
        std::unordered_set<std::string>& stack);
    void append_operation(CraftState& state, const Formula& formula, int batches) const;
    void callback_plan();
    void callback_plan_failed(const CraftState& state);

    bool ensure_craft_page();
    bool is_craft_page(const cv::Mat& image) const;
    bool is_formula_selector(const cv::Mat& image) const;
    bool is_obtain_items_page(const cv::Mat& image) const;
    bool execute_operation(const CraftOperation& operation);
    bool open_formula_selector();
    bool select_formula(const Formula& formula);
    bool prepare_formula_selector(const Formula& formula);
    bool click_elite_category();
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
    bool swipe_formula_list(const cv::Mat& image, bool forward, int distance_percent, int duration);
    bool set_craft_count(int batches);
    bool click_start_button();
    bool click_complete_tick();

    std::optional<Rect> match_workshop_template(
        const cv::Mat& image,
        const std::string& filename,
        double threshold,
        const Rect& roi = Rect(),
        bool mask = true) const;
    std::vector<TextRect> find_all_text(const cv::Mat& image, const Rect& roi = Rect()) const;
    Rect formula_list_roi(const cv::Mat& image);
    Rect image_rect(const cv::Mat& image) const;
    Rect clamp_rect(const Rect& rect, const cv::Mat& image) const;

    std::vector<CraftTarget> m_targets;
    std::unordered_map<std::string, int> m_inventory;
    std::unordered_map<std::string, Formula> m_formulas_by_id;
    std::unordered_map<std::string, std::vector<const Formula*>> m_formulas_by_item;
    std::vector<CraftOperation> m_plan;
    Rect m_formula_list_roi;
    long long m_plan_gold_cost = 0;
    long long m_plan_ap_cost = 0;
};
}
