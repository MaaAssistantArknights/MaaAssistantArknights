#include "AbstractReclamationTaskPlugin.h"

using namespace asst;

AbstractReclamationTaskPlugin::AbstractReclamationTaskPlugin(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain,
    const std::shared_ptr<ReclamationConfig>& config) :
    AbstractTaskPlugin(callback, inst, task_chain),
    m_config(config)
{
}
