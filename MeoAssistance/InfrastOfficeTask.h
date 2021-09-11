#pragma once
#include "InfrastAbstractTask.h"

namespace asst {
    class InfrastOfficeTask : public InfrastAbstractTask
    {
    public:
        using InfrastAbstractTask::InfrastAbstractTask;
        virtual ~InfrastOfficeTask() = default;

        virtual bool run() override;
    protected:
        // 进入干员选择界面
        bool enter_operator_selection();
        // 选择干员，返回本次选择了几个干员，DuckType，办公室固定返回 1
        int select_operators(bool need_to_the_left = false);
    };

}