#pragma once

#include "AbstractReclamationTaskPlugin.h"

namespace asst
{
class ReclamationCraftTaskPlugin : public AbstractReclamationTaskPlugin
{
public:
    using AbstractReclamationTaskPlugin::AbstractReclamationTaskPlugin;
    virtual ~ReclamationCraftTaskPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual bool load_params(const json::value& params) override;

protected:
    virtual bool _run() override;

private:
    bool calc_craft_amount(int& value);

    // ———————— constants and variables ———————————————————————————————————————————————
    std::string m_tool_to_craft; // 要组装的支援道具
    int m_num_craft_batches = 0; // 支援道具组装批次数, 每批组装 99 个
};
} // namespace asst
