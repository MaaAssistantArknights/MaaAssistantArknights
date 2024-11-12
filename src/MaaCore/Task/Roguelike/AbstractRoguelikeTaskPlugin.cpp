#include "AbstractRoguelikeTaskPlugin.h"

asst::AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain,
    const std::shared_ptr<RoguelikeConfig>& config,
    const std::shared_ptr<RoguelikeControlTaskPlugin>& control) :
    AbstractTaskPlugin(callback, inst, task_chain),
    m_config(config),
    m_control_ptr(control)
{
}
