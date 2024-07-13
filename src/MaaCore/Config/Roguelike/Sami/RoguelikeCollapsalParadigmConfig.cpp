#include "RoguelikeCollapsalParadigmConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::RoguelikeCollapsalParadigmConfig::parse(const json::value& json)
{
    LogTraceFunction;

    const std::string theme = json.at("theme").as_string();
    m_clp_pd_classes.erase(theme);
    m_clp_pd_dict.erase(theme);
    m_rare_clp_pds.erase(theme);

    const auto& theme_json = json.at("collapsal_paradigms");
    unsigned int index = 0;
    for (const auto& clp_pd_class_json : theme_json.as_array()) {
        CollapsalParadigmClass clp_pd_class;
        clp_pd_class.level_1 = clp_pd_class_json.at("level_1").as_string();
        clp_pd_class.level_2 = clp_pd_class_json.at("level_2").as_string();
        clp_pd_class.rarity = clp_pd_class_json.at("rarity").as_string();

        m_clp_pd_dict[theme][clp_pd_class.level_1] = index;
        m_clp_pd_dict[theme][clp_pd_class.level_2] = index;

        if (clp_pd_class.rarity == "rare") {
            m_rare_clp_pds[theme].insert(clp_pd_class.level_1);
            m_rare_clp_pds[theme].insert(clp_pd_class.level_2);
        }

        m_clp_pd_classes[theme].emplace_back(std::move(clp_pd_class));

        index++;
    }
    return true;
}

void asst::RoguelikeCollapsalParadigmConfig::clear()
{
    m_clp_pd_classes.clear();
    m_clp_pd_dict.clear();
    m_rare_clp_pds.clear();
}
