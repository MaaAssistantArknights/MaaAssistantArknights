#pragma once

#include "Vision/VisionHelper.h"

#include <functional>
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

    bool analyze();

    bool complete() const noexcept { return m_complete; }

    void set_cancel_check(std::function<bool()> check) { m_cancel_check = std::move(check); }

    const std::vector<MaterialRequirementInfo>& get_result() const noexcept { return m_result; }

private:
    struct RequirementSlot
    {
        std::string icon_task;
        std::string quantity_task;
    };

    std::vector<RequirementSlot> requirement_slots() const;
    bool analyze_slot(const RequirementSlot& slot, MaterialRequirementInfo& info) const;
    bool parse_quantity(const std::string& task_name, int& owned, int& required) const;
    bool match_item(const std::string& task_name, std::string& item_id, Rect& item_rect) const;

    std::vector<MaterialRequirementInfo> m_result;
    std::vector<std::string> m_candidates;
    bool m_complete = false;
    std::function<bool()> m_cancel_check;
};
}
