#pragma once
#include "AbstractTask.h"

namespace asst
{
    // 截图任务，主要是调试用的，制作模板匹配的素材啥的
    class ScreenCaptureTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~ScreenCaptureTask() = default;

        virtual bool run() override;

    private:
    };
}
