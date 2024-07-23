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

    virtual bool set_params([[maybe_unused]] const json::value& params) override
    {
        return m_config->get_theme() == RoguelikeTheme::Sami;
    }

    void set_start_foldartal(bool value) { m_start_foldartal = value; }

    void set_start_foldartal_list(std::vector<std::string> value) { m_start_foldartal_list = std::move(value); }

protected:
    virtual bool _run() override;

private:
    bool check_foldartals();

    bool m_start_foldartal = false;                  // 生活队凹开局密文板
    std::vector<std::string> m_start_foldartal_list; // 需要凹的板子
};
}
