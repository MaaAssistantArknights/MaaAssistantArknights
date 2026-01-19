#pragma once
#include "InfrastAbstractTask.h"

namespace asst
{
class InfrastIntelligentTask final : public InfrastAbstractTask
{
public:
    using InfrastAbstractTask::InfrastAbstractTask;
    virtual ~InfrastIntelligentTask() override = default;

protected:
    virtual bool _run() override;

private:
    // 单次扫描处理函数（返回房间扫描信息）
    bool scan_overview_and_report();
};
}
