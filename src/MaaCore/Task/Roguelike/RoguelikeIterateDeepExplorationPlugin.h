#pragma once
#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{

class RoguelikeIterateDeepExplorationPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeIterateDeepExplorationPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual bool load_params(const json::value& params) override;

protected:
    virtual bool _run() override;

private:
    // 深入调查数量
    std::unordered_map<std::string, int> deepExplorationCount = { { "Phantom", 12 },
                                                                  { "Mizuki", 12 },
                                                                  { "Sami", 13 },
                                                                  { "Sarkaz", 12 },
                                                                  { "JieGarden", 12 } };
    bool m_completed;
    bool m_iterateDE;
};
}
