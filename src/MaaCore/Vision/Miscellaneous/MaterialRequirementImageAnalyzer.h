#pragma once

#include "Vision/VisionHelper.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace asst
{
struct MaterialRequirementInfo
{
    std::string item_id;
    std::string item_name;
    int owned = 0;
    int required = 0;
    int shortage = 0;
    Rect item_rect;
    Rect quantity_rect;
};

class MaterialRequirementImageAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~MaterialRequirementImageAnalyzer() override = default;

    // Deferred names must be resolved before the result can be complete.
    bool analyze(bool defer_material_names = false);

    bool complete() const noexcept { return m_complete; }

    void set_cancel_check(std::function<bool()> check) { m_cancel_check = std::move(check); }

    const std::vector<MaterialRequirementInfo>& get_result() const noexcept { return m_result; }

    const std::optional<MaterialRequirementInfo>& pending_chip() const noexcept { return m_pending_chip; }

    const std::vector<MaterialRequirementInfo>& pending_materials() const noexcept { return m_pending_materials; }

    bool confirm_chip_name(const std::string& name);
    bool confirm_material_name(const MaterialRequirementInfo& pending, const std::string& name);
    bool recognize_material_icon(const MaterialRequirementInfo& pending);
    static bool is_promotion_page(const cv::Mat& image);
    static bool has_item_popup(const cv::Mat& image);
    static bool is_requirement_page(const cv::Mat& image);
    static std::optional<std::string> read_popup_name(const cv::Mat& image);

private:
    struct RequirementSlot
    {
        std::string icon_task;
        std::string quantity_task;
        std::string expected_item_id;
    };

    std::vector<RequirementSlot> requirement_slots() const;
    bool analyze_slot(const RequirementSlot& slot, MaterialRequirementInfo& info, bool defer_material_names) const;
    bool parse_quantity(const std::string& task_name, int& owned, int& required) const;
    bool match_item(
        const std::string& task_name,
        std::string& item_id,
        Rect& item_rect,
        const std::string& expected_item_id = {}) const;

    std::vector<MaterialRequirementInfo> m_result;
    std::vector<std::string> m_candidates;
    bool m_complete = false;
    bool m_slots_complete = false;
    std::optional<MaterialRequirementInfo> m_pending_chip;
    std::vector<MaterialRequirementInfo> m_pending_materials;
    std::function<bool()> m_cancel_check;
};
}
