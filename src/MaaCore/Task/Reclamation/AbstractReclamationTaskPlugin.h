#pragma once

#include <memory>

#include "ReclamationConfig.h"
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
class AbstractReclamationTaskPlugin : public AbstractTaskPlugin
{
public:
    AbstractReclamationTaskPlugin(
        const AsstCallback& callback,
        Assistant* inst,
        std::string_view task_chain,
        const std::shared_ptr<ReclamationConfig>& config);

    // 根据 params 设置插件专用参数, 返回插件是否应被启用
    virtual bool load_params([[maybe_unused]] const json::value& params) { return true; }

protected:
    std::shared_ptr<ReclamationConfig> m_config = nullptr;
};
} // namespace asst
