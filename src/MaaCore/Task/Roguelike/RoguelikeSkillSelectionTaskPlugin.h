#pragma once

#include <unordered_map>

#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{
class RoguelikeSkillSelectionTaskPlugin final : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeSkillSelectionTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

protected:
    virtual bool _run() override;
};
}
