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
    void increase_craft_amount(const int& amount = 99);
    bool calc_craft_amount(int& value);

    // ———————— constants and variables ———————————————————————————————————————————————
    enum class IncrementMode // 点击加号按钮增加组装数量的方式
    {
        Click = 0,
        Hold = 1
    };

    std::vector<std::string> tools_to_craft;                // 要组装的支援道具
    int m_num_craft_batches = 16;                          // 支援道具组装批次数, 每批组装 99 个
    IncrementMode m_increment_mode = IncrementMode::Click; // 点击加号按钮增加组装数量的方式
};
} // namespace asst
