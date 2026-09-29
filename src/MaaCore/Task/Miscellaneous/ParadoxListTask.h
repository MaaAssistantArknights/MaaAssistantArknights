#pragma once
#include <optional>
#include <unordered_set>

#include "Task/AbstractTask.h"
#include "Vision/Oper/OperBoxImageAnalyzer.h"

namespace asst
{
class ParadoxListTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;

    void set_target(std::string name) { m_target = std::move(name); }

    void set_next_only(bool enabled, const std::vector<std::string>& candidates)
    {
        m_next_only = enabled;
        m_candidates = { candidates.begin(), candidates.end() };
    }

    const auto& get_result() const noexcept { return m_result; }

    bool found_target() const noexcept { return m_found; }

    bool target_completed() const noexcept { return m_completed; }

private:
    bool _run() override;
    bool prepare();
    bool rewind();
    bool return_to_list();
    bool ensure_role_panel_expanded();
    std::optional<std::vector<OperBoxInfo>> analyze_page();
    std::string detail_name();
    bool same_page(const cv::Mat& before, const cv::Mat& after) const;

    std::string m_target;
    std::vector<OperBoxInfo> m_result;
    bool m_found = false;
    bool m_completed = false;
    bool m_next_only = false;
    std::unordered_set<std::string> m_candidates;
};
}
