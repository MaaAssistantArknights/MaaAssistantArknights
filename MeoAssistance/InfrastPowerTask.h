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

        // 进入当前发电站的干员选择界面
        bool enter_operator_selection();
        // 选择干员，返回本次选择了几个干员，DuckType，发电站固定返回 1
        int select_operators(bool need_to_the_left = false);

    };
}