#pragma once
#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{
class RoguelikeControlTaskPlugin;

class RoguelikeLevelTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeLevelTaskPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual bool load_params([[maybe_unused]] const json::value& params) override;

protected:
    virtual bool _run() override;

private:
    void stop_roguelike() const;

    bool m_stop_at_max = false; // 是否在达到最高等级时停止
};
}
