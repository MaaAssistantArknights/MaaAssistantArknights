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

    bool checked = false;       // 同一次肉鸽StartExplore被调用复数次的情况只检查一次
    bool m_stop_at_max = false; // 是否在达到最高等级时停止
};
}
