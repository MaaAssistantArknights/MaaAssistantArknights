#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
class InfrastControlTask final : public InfrastProductionTask
{
public:
    using InfrastProductionTask::InfrastProductionTask;
    virtual ~InfrastControlTask() override = default;

    void set_vacancy_only(bool enabled) noexcept { m_vacancy_only = enabled; }

    virtual size_t max_num_of_opers() const noexcept override { return 5ULL; }

protected:
    virtual int operlist_swipe_times() const noexcept override { return 4; }

private:
    virtual bool _run() override;

    bool m_vacancy_only = false;
};
}
