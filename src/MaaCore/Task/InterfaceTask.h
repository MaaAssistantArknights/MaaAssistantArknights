#pragma once
#include "PackageTask.h"

namespace asst
{
class InterfaceTask : public PackageTask
{
public:
    using PackageTask::PackageTask;
    virtual ~InterfaceTask() override = default;

    virtual bool run() override;

    virtual bool set_params([[maybe_unused]] const json::value& params) { return true; }

    // 任务链失败时保存现场截图；取缓存帧而非现拍，避免失败现场再触发截图重试
    bool save_fail_img() { return save_img(utils::path("debug") / utils::path("interface"), true); }
};
}
