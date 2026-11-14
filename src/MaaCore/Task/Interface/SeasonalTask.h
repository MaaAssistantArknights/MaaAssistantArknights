#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class SeasonalTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "SeasonalTask";

    SeasonalTask(const AsstCallback& callback, Assistant* inst);
    virtual ~SeasonalTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    virtual bool run() override;

    int m_spa_rounds = 0; // 卫戍协议轮数，0 表示跳过
};
}
