#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
    // 集成战略模式快捷编队任务
    class RoguelikeFormationTaskPlugin : public AbstractTaskPlugin
    {
    public:
        static constexpr size_t MaxNumOfOperPerPage = 8;

    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeFormationTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

        void clear_and_reselect();
        size_t analyze_and_select();
    };
}
