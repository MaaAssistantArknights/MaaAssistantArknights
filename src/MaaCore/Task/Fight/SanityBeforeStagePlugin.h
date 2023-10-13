#pragma once
#include "Task/AbstractTaskPlugin.h"

#include <meojson/json.hpp>
namespace asst
{

    class SanityBeforeStagePlugin final : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~SanityBeforeStagePlugin() override = default;
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    private:
        virtual bool _run() override;

        void get_sanity_before_stage();
    };
}
