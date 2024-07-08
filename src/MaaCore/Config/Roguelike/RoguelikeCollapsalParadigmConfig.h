#pragma once

#include "Config/AbstractConfig.h"

#include <vector>

#include "Common/AsstBattleDef.h"

namespace asst
{
    // 坍缩范式
    struct CollapsalParadigmClass
    {
        std::string level_1; // 一级范式
        std::string level_2; // 二级范式
        std::string rarity;  // 稀有度
    };

    class RoguelikeCollapsalParadigmConfig final : public SingletonHolder<RoguelikeCollapsalParadigmConfig>, public AbstractConfig
    {
    public:
        virtual ~RoguelikeCollapsalParadigmConfig() override = default;

        const auto& get_clp_pd_classes(const std::string& theme) const noexcept { return m_clp_pd_classes.at(theme); }
        const auto& get_clp_pd_dict(const std::string& theme) const noexcept { return m_clp_pd_dict.at(theme); }
        const auto& get_rare_clp_pds(std::string theme) const noexcept {
            if (auto it = m_rare_clp_pds.find(theme); it == m_rare_clp_pds.end()) {
                static std::unordered_set<std::string> empty_set;
                return empty_set;
            }
            return m_rare_clp_pds.at(theme);
        }
        
    private:
        virtual bool parse(const json::value& json) override;
        
        void clear();

        std::unordered_map<std::string, std::vector<CollapsalParadigmClass>> m_clp_pd_classes;
        std::unordered_map<std::string, std::unordered_map<std::string, unsigned int>> m_clp_pd_dict;
        std::unordered_map<std::string, std::unordered_set<std::string>> m_rare_clp_pds;
    };

    inline static auto& RoguelikeCollapsalParadigms = RoguelikeCollapsalParadigmConfig::get_instance();
}
