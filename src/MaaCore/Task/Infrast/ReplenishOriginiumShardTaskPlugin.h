#pragma once

#include <optional>
#include <string_view>

#include "Task/AbstractTaskPlugin.h"

namespace asst
{
// 制造站“源石碎片”自动补货任务插件
class ReplenishOriginiumShardTaskPlugin : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~ReplenishOriginiumShardTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    void set_use_device(bool use_device) noexcept { m_use_device = use_device; }

private:
    struct MaterialCount
    {
        int current = 0;
        int required = 0;
    };

    static std::optional<MaterialCount> parse_material_count(std::string_view text, int expected_required) noexcept;
    bool replenish_original();

    virtual bool _run() override;

    bool m_use_device = false;
};
}
