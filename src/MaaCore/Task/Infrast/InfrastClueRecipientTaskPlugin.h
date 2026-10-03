#pragma once

#include <array>
#include <set>
#include <string>

#include "ClueRecipient.h"
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
class InfrastClueRecipientTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;

    void set_recipient(std::string recipient) { m_recipient = std::move(recipient); }

    bool is_recipient_unavailable() const noexcept { return m_recipient_unavailable; }

    bool is_using_default_strategy() const noexcept { return m_using_default_strategy; }

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

private:
    virtual bool _run() override;

    infrast::ClueRecipientPage read_names(const cv::Mat& image) const;
    bool on_recipient_not_found(ProcessTask& task);

    std::string m_recipient;
    std::set<std::array<std::string, 4>> m_seen_pages;
    infrast::ClueRecipientSearchAttempts m_search_attempts;
    bool m_search_started = false;
    bool m_recipient_unavailable = false;
    bool m_using_default_strategy = false;
};
}
