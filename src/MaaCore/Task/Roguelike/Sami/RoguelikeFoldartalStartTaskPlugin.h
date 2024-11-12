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

    virtual bool load_params([[maybe_unused]] const json::value& params) override;

protected:
    virtual bool _run() override;

private:
    bool check_foldartals();

    std::vector<std::string> m_start_foldartal_list; // 需要凹的板子
};
}
