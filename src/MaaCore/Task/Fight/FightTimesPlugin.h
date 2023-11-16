#pragma once
#include "Task/AbstractTaskPlugin.h"
namespace asst
{
    class FightTimesPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~FightTimesPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

    private:
        // 是否成功初始化为1次
        bool inited = false;
    };
}
