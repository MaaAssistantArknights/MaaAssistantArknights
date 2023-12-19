#include "AbstractRoguelikeTaskPlugin.h"

asst::AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin(const AsstCallback& callback, Assistant* inst,
                                                               std::string_view task_chain,
                                                               std::shared_ptr<RoguelikeConfig> config)
    : AbstractTaskPlugin(callback, inst, task_chain), m_config(config)
{}
