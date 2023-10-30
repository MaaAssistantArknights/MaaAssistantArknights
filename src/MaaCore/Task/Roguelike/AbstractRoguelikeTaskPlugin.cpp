#include "AbstractRoguelikeTaskPlugin.h"

asst::AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin(const AsstCallback& callback, Assistant* inst,
                                                               std::string_view task_chain,
                                                               std::shared_ptr<RoguelikeData> roguelike_data)
    : AbstractTaskPlugin(callback, inst, task_chain), m_roguelike_data(roguelike_data)
{}
