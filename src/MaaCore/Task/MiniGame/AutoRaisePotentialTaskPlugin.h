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
        FirstOperatorEntered,
        PotentialFound,
        OperatorDone,
    };

    bool read_operator_count();
    void report_progress(bool has_potential);
    void stop_process_task(std::string_view reason);

    mutable PendingAction m_pending = PendingAction::None;
    int m_total = 0;
    int m_current = 0;
    bool m_potential_clicked = false;

    inline static constexpr int MaxOperatorCount = 512;
    inline static constexpr int MaxPotentialLevels = 6;
};
} // namespace asst
