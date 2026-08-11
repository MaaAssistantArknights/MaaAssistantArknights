#pragma once

#include "InfrastDormSelection.h"
#include "InfrastProductionTask.h"

namespace asst
{
class InfrastDormTask final : public InfrastProductionTask
{
public:
    using InfrastProductionTask::InfrastProductionTask;
    virtual ~InfrastDormTask() override = default;

    virtual size_t max_num_of_opers() const noexcept override { return 5ULL; }

    InfrastDormTask& set_notstationed_enabled(bool notstationed_filter_enabled) noexcept;
    InfrastDormTask& set_trust_enabled(bool trust_autofill_enabled) noexcept;
    InfrastDormTask& set_prepare_phase(bool enabled) noexcept;
    InfrastDormTask& set_fiammetta_targets(std::vector<std::string> targets) noexcept;

protected:
    virtual bool on_run_fails() override;

private:
    enum class FiammettaSelectionResult
    {
        Selected,
        NotFound,
        Error,
    };

    virtual bool _run() override;

    bool fill_dorm_slots(bool low_mood_only);
    bool select_dorm_managers();
    FiammettaSelectionResult try_select_fiammetta_pair();
    bool set_notstationed_filter(bool enabled);
    bool restore_list_sort_for_selection_phase(asst::infrast::CustomRoomConfig const& room_config);
    bool switch_to_mood_sort();
    bool switch_to_low_mood_sort();
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
    bool m_prepare_phase = false;
    bool m_fiammetta_checked = false;
    std::vector<std::string> m_fiammetta_targets;
};
} // namespace asst
