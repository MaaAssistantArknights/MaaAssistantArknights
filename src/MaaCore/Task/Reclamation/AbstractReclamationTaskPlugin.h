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

    virtual bool load_params([[maybe_unused]] const ReclamationConfig::Params& params) { return true; }

protected:
    std::shared_ptr<ReclamationConfig> m_config = nullptr;
};
} // namespace asst
