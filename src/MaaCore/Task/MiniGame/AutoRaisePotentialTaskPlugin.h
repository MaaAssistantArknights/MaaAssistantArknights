#pragma once

#include "Task/AbstractTaskPlugin.h"

#include <string_view>

namespace asst
{
class AutoRaisePotentialTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~AutoRaisePotentialTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

protected:
    virtual bool _run() override;

private:
    enum class PendingAction
    {
        None,
        ReadOperatorCount,
    };

    bool read_operator_count();
    void stop_process_task(std::string_view reason);

    mutable PendingAction m_pending = PendingAction::None;

    inline static constexpr std::string_view OperatorCountTask = "MiniGame@AutoRaisePotential@OperatorCountOcr";
    inline static constexpr std::string_view PotentialTask = "MiniGame@AutoRaisePotential@PotentialAvailable";
    inline static constexpr std::string_view PotentialAfterSwipeTask =
        "MiniGame@AutoRaisePotential@PotentialAvailableAfterSwipe";
    inline static constexpr std::string_view SwipeTask = "MiniGame@AutoRaisePotential@SwipeToNextOperator";
    inline static constexpr int MaxOperatorCount = 512;
    inline static constexpr int MaxPotentialLevels = 6;
};
} // namespace asst
