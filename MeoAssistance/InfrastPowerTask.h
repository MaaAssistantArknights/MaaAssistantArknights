#pragma once
#include "InfrastAbstractTask.h"

namespace asst {
    // 发电站换班任务
    // 任务开始前需要处在基建主界面
    class InfrastPowerTask : public InfrastAbstractTask
    {
    public:
        using InfrastAbstractTask::InfrastAbstractTask;
        virtual ~InfrastPowerTask() = default;

        virtual bool run() override;
    protected:
        constexpr static int PowerNum = 3;			// 发电站数量

    };
}