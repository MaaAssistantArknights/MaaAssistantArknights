#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "InfrastProductionTask.h"

namespace asst
{
namespace infrast
{
struct DormSelectionCandidate
{
    std::string name;
    std::string operator_id;
    double mood_ratio = 0;
    bool selected = false;
    bool available = true;
};

std::vector<std::string> normalize_fiammetta_targets(const std::vector<std::string>& configured);

std::string fiammetta_target_id(std::string_view name);

std::optional<size_t> find_fiammetta_target(
    const std::vector<DormSelectionCandidate>& first_page,
    const std::vector<std::string>& fiammetta_targets,
    double mood_threshold);

std::optional<size_t> find_full_mood_fiammetta(const std::vector<DormSelectionCandidate>& first_page);
} // namespace infrast

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

    // detect_* 的识别结果：Found 时命中干员已挪到 opers 首位；
    // Error 表示截图/分析异常，与目标确实不在场的 NotFound 区分开。
    enum class DetectResult
    {
        Found,
        NotFound,
        Error,
    };

    virtual bool _run() override;

    bool fill_dorm_slots();
    bool should_select_dorm_managers() const noexcept;
    bool select_dorm_managers();
    bool run_fiammetta_preparation();
    FiammettaSelectionResult try_select_fiammetta_pair();
    // 识别当前排序第一页的配对干员；命中者挪到 opers 首位供点选。
    DetectResult detect_fiammetta_target(std::vector<infrast::Oper>& opers);
    DetectResult detect_full_mood_fiammetta(std::vector<infrast::Oper>& opers);
    bool keep_exhausted_fiammetta();
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
