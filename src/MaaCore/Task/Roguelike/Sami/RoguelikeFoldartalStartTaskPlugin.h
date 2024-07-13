#pragma once
#include "../AbstractRoguelikeTaskPlugin.h"

namespace asst
{
    class RoguelikeFoldartalStartTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        virtual ~RoguelikeFoldartalStartTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

    private:
        bool check_foldartals();
    };
}
