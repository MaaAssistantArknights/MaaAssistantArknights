#pragma once
#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{
class RoguelikeResetTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeResetTaskPlugin() = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

protected:
    virtual bool _run() override;
};
}
