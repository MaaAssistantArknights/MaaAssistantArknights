#pragma once

#include "Task/AbstractTaskPlugin.h"

namespace asst
{
class KarlanTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    bool verify(AsstMsg msg, const json::value& details) const override;

protected:
    bool _run() override;
};
}
