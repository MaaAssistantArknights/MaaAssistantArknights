#pragma once

#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "BlackFlowInventoryModel.h"

namespace cv
{
class Mat;
}

namespace asst::blackflow
{
struct InventoryCleanupPolicy;

class BlackFlowInventoryContext
{
public:
    virtual ~BlackFlowInventoryContext() = default;

    virtual cv::Mat capture() const = 0;
    virtual bool click(const Rect& rect) const = 0;
    virtual bool execute(std::vector<std::string> tasks, std::string* error) = 0;
    virtual const std::string& last_task() const noexcept = 0;
    virtual void report_cleanup_status(std::string_view status, std::string detail = {}) = 0;
    virtual bool interrupted() const = 0;
    virtual void wait(unsigned milliseconds) const = 0;
    virtual bool precise_swipe_supported() const = 0;
    virtual bool swipe_by(std::string_view task, int distance, std::string* error) = 0;
};

class BlackFlowInventoryCleanup final
{
public:
    bool run(BlackFlowInventoryContext& context, std::string* error);

private:
    struct Shift
    {
        int distance = 0;
        std::string anchor;
        int row = 0;
        int measured = 0;
    };

    enum class WalkToStatus
    {
        Reached,
        Interrupted,
        ViewportUnresolved,
        TargetUnavailable,
        Failed,
    };

    struct WalkToResult
    {
        WalkToStatus status = WalkToStatus::Failed;
        std::optional<VisibleScrap> scrap;
        std::string error;
    };

    const InventoryCleanupPolicy* resolve_policy(std::string* error) const;

    bool full_flag_visible(BlackFlowInventoryContext& context) const;
    std::vector<VisibleScrap> recognize(BlackFlowInventoryContext& context, const InventoryCleanupPolicy& policy) const;
    [[nodiscard]] bool same_view(const std::vector<VisibleScrap>& lhs, const std::vector<VisibleScrap>& rhs) const;
    std::optional<Shift>
        measure_shift(const std::vector<VisibleScrap>& before, const std::vector<VisibleScrap>& after) const;
    bool swipe(BlackFlowInventoryContext& context, bool forward, std::string* error) const;
    bool rewind_to_left(BlackFlowInventoryContext& context, std::string* error) const;
    bool advance(
        BlackFlowInventoryContext& context,
        const std::vector<VisibleScrap>& visible,
        bool forward,
        std::string* error) const;

    bool survey(BlackFlowInventoryContext& context, const InventoryCleanupPolicy& policy, std::string* error);
    WalkToResult walk_to(BlackFlowInventoryContext& context, const InventoryCleanupPolicy& policy, InventorySlot slot);
    std::optional<VisibleScrap> walk_to_name(
        BlackFlowInventoryContext& context,
        const InventoryCleanupPolicy& policy,
        std::string_view name,
        std::string* error) const;
    bool discard(BlackFlowInventoryContext& context, const VisibleScrap& scrap, std::string* error);

    bool finish(BlackFlowInventoryContext& context, bool report, std::string* error);
    bool abandon(BlackFlowInventoryContext& context, std::string* error) const;

    InventoryModel m_model;
    std::set<InventorySlot> m_failed;
    std::map<std::size_t, int> m_attempts;
    int m_base_x = 0;
    int m_offset = 0;
    std::optional<int> m_view_left_column;
    int m_discarded = 0;
    int m_rescans = 0;
    bool m_rank_fallback = false;
    std::optional<InventoryCell> m_fallback_best;
};
}
