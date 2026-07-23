#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "BlackFlowTaskPluginBase.h"
#include "Common/AsstTypes.h"

namespace asst::blackflow
{
class BlackFlowMovementTaskPlugin final : public BlackFlowTaskPluginBase
{
public:
    using BlackFlowTaskPluginBase::BlackFlowTaskPluginBase;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual void reset_in_run_variables() override;

protected:
    virtual bool _run() override;

private:
    enum class SelectionOutcome
    {
        Selected,
        Unavailable,
        Failed,
    };

    struct PanelItem
    {
        MovementKind movement = MovementKind::Walk;
        Rect name_rect;
        double name_score = 0.0;
        std::optional<int> remaining_uses;
        bool loaded = false;
        int name_match_count = 1;
        int remaining_match_count = 0;
    };

    struct PanelFrame
    {
        std::vector<PanelItem> items;
        std::optional<MovementKind> loaded_movement;
    };

    SelectionOutcome select_movement(MovementKind target, std::string* error);
    bool ensure_panel_open(std::string* error);
    bool close_panel(std::string* error);
    std::optional<MovementKind> recognize_loaded_movement(const cv::Mat& image) const;
    bool title_visible(const cv::Mat& image) const;
    bool scan_current_frame(PanelFrame& frame, std::string* error) const;
    bool analyze_frame(const cv::Mat& image, PanelFrame& frame, std::string* error) const;
    bool run_fixed_task(std::string_view task);
    bool report_target_observation(
        MovementKind target,
        const PanelItem& item,
        int completed_swipes,
        std::optional<MovementKind> active_movement,
        std::string* error);

    static const MovementSpec* movement_from_name(std::string_view name) noexcept;
    static std::optional<int> remaining_uses_from_text(std::string_view text) noexcept;
    static std::vector<std::string> ocr_candidates();

    mutable bool m_selection_pending = false;
};
} // namespace asst::blackflow
