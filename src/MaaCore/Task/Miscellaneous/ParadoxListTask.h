#pragma once
#include "Task/AbstractTask.h"
#include <unordered_set>

namespace asst
{
class ParadoxListTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;

    void set_candidates(std::unordered_set<std::string> names) { m_candidates = std::move(names); }

    void set_include_completed(bool enabled) { m_include_completed = enabled; }

    bool completed() const noexcept { return m_completed; }

    const std::string& get_result() const noexcept { return m_result; }

private:
    bool _run() override;
    bool prepare();
    bool rewind();
    bool return_to_list();
    std::string detail_name();
    bool same_page(const cv::Mat& before, const cv::Mat& after) const;

    bool m_include_completed = false;
    bool m_completed = false;
    std::string m_result;
    std::unordered_set<std::string> m_candidates;
};
}
