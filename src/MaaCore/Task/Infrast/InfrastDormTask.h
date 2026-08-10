#pragma once

#include "InfrastAbstractTask.h"

namespace asst
{
class InfrastDormTask final : public InfrastAbstractTask
{
public:
    using InfrastAbstractTask::InfrastAbstractTask;
    virtual ~InfrastDormTask() override = default;

    virtual size_t max_num_of_opers() const noexcept override { return 5ULL; }

    InfrastDormTask& set_notstationed_enabled(bool notstationed_filter_enabled) noexcept;
    InfrastDormTask& set_trust_enabled(bool trust_autofill_enabled) noexcept;

protected:
    virtual bool on_run_fails() override;

private:
    virtual bool _run() override;

    bool fill_dorm_slots();
    bool set_notstationed_filter(bool enabled);
    bool restore_list_sort_for_selection_phase(asst::infrast::CustomRoomConfig const& room_config);
    bool switch_to_mood_sort();
    void switch_to_trust_autofill_phase();
    void advance_after_trust_sort();
    bool is_in_trust_autofill_phase() const noexcept;

    bool m_notstationed_filter_enabled = false;
    bool m_trust_autofill_enabled = true;
    int m_max_num_of_dorm = 4;

    enum class SelectionPhase
    {
        LowMood,
        ResortForTrust,
        TrustAutofill,
        FillRemaining,
    };

    SelectionPhase m_selection_phase = SelectionPhase::LowMood;
    bool m_notstationed_filter_active = false;
};
} // namespace asst
