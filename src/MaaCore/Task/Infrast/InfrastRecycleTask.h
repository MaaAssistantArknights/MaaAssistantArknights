#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
// 回收站换班：当前仅旅骨拥有回收站技能，按技能评分挑选意义有限，
// 清空后直接按位置补满进驻位，不做技能挑选。
class InfrastRecycleTask final : public InfrastProductionTask
{
public:
    using InfrastProductionTask::InfrastProductionTask;
    virtual ~InfrastRecycleTask() override = default;

    virtual size_t max_num_of_opers() const noexcept override { return 2ULL; }

protected:
    virtual bool _run() override;

private:
    bool fill_slots();
};
}
