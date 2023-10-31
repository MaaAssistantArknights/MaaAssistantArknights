#include "AbstractRoguelikeTaskPlugin.h"

asst::AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin(const AsstCallback& callback, Assistant* inst,
                                                               std::string_view task_chain,
                                                               std::shared_ptr<RoguelikeConfig> roguelike_config)
    : AbstractTaskPlugin(callback, inst, task_chain), m_roguelike_config(roguelike_config)
{}
