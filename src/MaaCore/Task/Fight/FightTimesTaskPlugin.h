#pragma once
#include "Task/AbstractTaskPlugin.h"
namespace asst
{
    class FightTimesTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~FightTimesTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

    private:
        bool inited = false; // 是否成功初始化为1次，初始化后后续不再检测 (实现调整次数后移除此变量
    };
}
