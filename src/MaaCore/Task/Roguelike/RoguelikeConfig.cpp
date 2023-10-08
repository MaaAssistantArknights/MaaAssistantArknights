#include "RoguelikeConfig.h"

void asst::RoguelikeConfig::set_roguelike_theme(std::string roguelike_theme)
{
    m_roguelike_theme = std::move(roguelike_theme);
}
