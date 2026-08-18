#pragma once

#include "Task/Infrast/InfrastAbstractTask.h"

namespace asst
{
class InfrastReceptionPresetTask final : public InfrastAbstractTask
{
public:
    using InfrastAbstractTask::InfrastAbstractTask;
    virtual ~InfrastReceptionPresetTask() override = default;

    InfrastReceptionPresetTask& set_receive_message_board(bool value) noexcept;
    InfrastReceptionPresetTask& set_receive_clue(bool value) noexcept;
    InfrastReceptionPresetTask& set_enable_clue_exchange(bool value) noexcept;
    InfrastReceptionPresetTask& set_send_clue(bool value) noexcept;

protected:
    virtual bool _run() override;
    virtual bool on_run_fails() override;
    virtual std::string facility_name() const override { return "Reception"; }

private:
    bool receive_message_board();
    bool close_end_of_clue_exchange();
    bool get_friend_clue();
    bool get_self_clue();
    bool send_clue();
    bool unlock_clue_exchange();
    bool exchange_clues_once();
    bool quick_insert_clues();
    bool fill_clue_vacancies_fallback();

    bool m_receive_message_board = true;
    bool m_receive_clue = true;
    bool m_enable_clue_exchange = true;
    bool m_send_clue = true;
};
} // namespace asst
