#include "AbstractRoguelikeTaskPlugin.h"

asst::AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin(const AsstCallback& callback, Assistant* inst,
                                                               std::string_view task_chain,
                                                               const std::shared_ptr<RoguelikeConfig>& config)
    : AbstractTaskPlugin(callback, inst, task_chain), m_config(config)
{}

std::optional<json::value> asst::AbstractRoguelikeTaskPlugin::notify_sibling_plugins(
    RoguelikeEvent event,
    const json::value& detail)
{
    for (const TaskPluginPtr& plugin : m_task_ptr->get_plugins()) {
        plugin->set_task_id(m_task_id);
        plugin->set_task_ptr(m_task_ptr);

        if (std::shared_ptr<AbstractRoguelikeTaskPlugin> roguelike_plugin =
                std::dynamic_pointer_cast<AbstractRoguelikeTaskPlugin>(plugin)) {
            if (std::optional<json::value> response =
                    roguelike_plugin->response_to_event(event, detail)) {
                return response;
            }
        }
    }
    return std::nullopt;
}
