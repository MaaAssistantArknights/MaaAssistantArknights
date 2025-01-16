#pragma once
#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{
class RoguelikeInputSeedTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeInputSeedTaskPlugin() override = default;
    virtual bool load_params(const json::value& params) override;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;

protected:
    virtual bool _run() override;
};
}
