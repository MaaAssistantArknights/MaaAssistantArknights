#pragma once

#include "AbstractReclamationTaskPlugin.h"

namespace asst
{
class ReclamationCraftTaskPlugin : public AbstractReclamationTaskPlugin
{
public:
    using AbstractReclamationTaskPlugin::AbstractReclamationTaskPlugin;
    virtual ~ReclamationCraftTaskPlugin() override = default;
    virtual bool load_params(const ReclamationConfig::Params& params) override;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;

protected:
    virtual bool _run() override;

private:
    void increase_craft_amount(const ReclamationConfig::Params& params, int amount);
    bool calc_craft_amount(const std::string& theme, int& value);

    std::string m_craft_task_name;
};
} // namespace asst
