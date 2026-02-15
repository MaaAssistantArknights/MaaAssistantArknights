#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
class InfrastReceptionTask final : public InfrastProductionTask
{
public:
    using InfrastProductionTask::InfrastProductionTask;
    virtual ~InfrastReceptionTask() override = default;

    virtual size_t max_num_of_opers() const noexcept override { return 2ULL; }

    void set_receive_message_board(bool value) noexcept { m_receive_message_board = value; }

    void set_enable_clue_exchange(bool value) noexcept { m_enable_clue_exchange = value; }

    void set_send_clue(bool value) noexcept { m_send_clue = value; }

protected:
    virtual bool _run() override;
    virtual bool on_run_fails() override;

private:
    virtual int operlist_swipe_times() const noexcept override { return 4; }

    // 收取信息板的周限300信用
    bool receive_message_board();
    bool close_end_of_clue_exchange();
    bool get_friend_clue();
    bool get_self_clue();
    bool use_clue();
    bool proc_clue_vacancy();
    bool unlock_clue_exchange();
    bool back_to_reception_main();
    bool send_clue();
    bool shift();

    bool m_receive_message_board = true;
    bool m_enable_clue_exchange = true;
    bool m_send_clue = true;
};
}
