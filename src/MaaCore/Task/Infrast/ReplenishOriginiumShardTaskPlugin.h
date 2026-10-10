#pragma once

#include "Task/AbstractTaskPlugin.h"

namespace asst
{
enum class OriginiumShardRecipe;

// 制造站“源石碎片”自动补货任务插件
class ReplenishOriginiumShardTaskPlugin : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~ReplenishOriginiumShardTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    void set_use_device(bool use_device) noexcept { m_use_device = use_device; }

private:
    bool open_originium_shard_selector() const;
    bool close_originium_shard_selector() const;
    bool select_recipe(OriginiumShardRecipe recipe) const;
    bool replenish_original();

    virtual bool _run() override;

    bool m_use_device = false;
};
}
