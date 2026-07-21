#include "RoguelikeBlackflowRoutingConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

using namespace asst;

const BlackflowGearInfo* RoguelikeBlackflowRoutingConfig::gear_by_name(const std::string& name) const
{
    for (const auto& gear : m_gears) {
        if (gear.name == name) {
            return &gear;
        }
    }
    return nullptr;
}

int RoguelikeBlackflowRoutingConfig::node_ap_gain(RoguelikeNodeType type) const
{
    auto it = m_node_effects.find(type);
    return it == m_node_effects.end() ? 0 : it->second.ap_gain;
}

bool RoguelikeBlackflowRoutingConfig::node_teleport_paired(RoguelikeNodeType type) const
{
    auto it = m_node_effects.find(type);
    return it != m_node_effects.end() && it->second.teleport_paired;
}

const BlackflowStrategyProfile* RoguelikeBlackflowRoutingConfig::strategy_for_mode(int mode) const
{
    auto mode_it = m_mode_strategies.find(mode);
    if (mode_it == m_mode_strategies.end()) {
        return nullptr;
    }
    auto strat_it = m_strategies.find(mode_it->second);
    return strat_it == m_strategies.end() ? nullptr : &strat_it->second;
}

bool RoguelikeBlackflowRoutingConfig::parse(const json::value& json)
{
    LogTraceFunction;

    m_gears.clear();
    m_node_effects.clear();
    m_combat_types.clear();
    m_trader_types.clear();
    m_endpoint_types.clear();
    m_strategies.clear();
    m_mode_strategies.clear();

    static const std::unordered_map<std::string, blackflow::GearRange> RANGE_MAPPING = {
        { "line", blackflow::GearRange::Line },
        { "ring8", blackflow::GearRange::Ring8 },
        { "ring12", blackflow::GearRange::Ring12 },
        { "any", blackflow::GearRange::Any },
        { "anyNonCombat", blackflow::GearRange::AnyNonCombat },
        { "anyTrader", blackflow::GearRange::AnyTrader },
        { "randomNonCombat", blackflow::GearRange::RandomNonCombat },
    };

    for (const auto& gear_json : json.at("gears").as_array()) {
        BlackflowGearInfo info;
        info.name = gear_json.at("name").as_string();
        const std::string range_name = gear_json.at("range").as_string();
        auto range_it = RANGE_MAPPING.find(range_name);
        if (range_it == RANGE_MAPPING.end()) {
            Log.error(__FUNCTION__, "| Unknown gear range:", range_name, "of gear", info.name);
            return false;
        }
        info.range = range_it->second;
        info.distance = gear_json.get("distance", 0);
        if (info.range == blackflow::GearRange::Line && info.distance <= 0) {
            Log.error(__FUNCTION__, "| Gear", info.name, "has line range but invalid distance", info.distance);
            return false;
        }
        info.max_uses = gear_json.get("maxUses", 1);
        info.ap_cost = gear_json.get("apCost", 1);
        info.ap_gain = gear_json.get("apGain", 0);
        info.carryover = gear_json.get("carryover", true);
        info.controllable = gear_json.get("controllable", true);
        m_gears.emplace_back(std::move(info));
    }

    if (auto effects_opt = json.find<json::object>("nodeEffects")) {
        for (const auto& [type_name, effect_json] : effects_opt.value()) {
            const RoguelikeNodeType type = RoguelikeMapConfig::name2type(type_name);
            if (type == RoguelikeNodeType::Unknown) {
                return false; // name2type 已打日志
            }
            NodeEffect effect;
            effect.ap_gain = effect_json.get("apGain", 0);
            effect.teleport_paired = effect_json.get("teleportPaired", false);
            m_node_effects.emplace(type, effect);
        }
    }

    auto parse_type_set = [&](const char* key, std::unordered_set<RoguelikeNodeType>& out) -> bool {
        auto sets_opt = json.find<json::object>("typeSets");
        if (!sets_opt) {
            return true;
        }
        auto arr_opt = sets_opt->find(key);
        if (!arr_opt || !arr_opt->is_array()) {
            return true;
        }
        for (const auto& name_json : arr_opt->as_array()) {
            const RoguelikeNodeType type = RoguelikeMapConfig::name2type(name_json.as_string());
            if (type == RoguelikeNodeType::Unknown) {
                return false;
            }
            out.emplace(type);
        }
        return true;
    };
    if (!parse_type_set("combat", m_combat_types) || !parse_type_set("trader", m_trader_types) ||
        !parse_type_set("endpoint", m_endpoint_types)) {
        return false;
    }

    for (const auto& [profile_name, profile_json] : json.at("strategies").as_object()) {
        BlackflowStrategyProfile profile;
        profile.name = profile_name;
        profile.endpoint_required = profile_json.get("endpointRequired", false);
        profile.shortest_endpoint = profile_json.get("shortestEndpoint", false);
        profile.avoid_combat_first = profile_json.get("avoidCombatFirst", false);
        const std::string unreachable = profile_json.get("endpointUnreachable", "bestEffort");
        if (unreachable != "bestEffort" && unreachable != "abandon") {
            Log.error(__FUNCTION__, "| Unknown endpointUnreachable policy:", unreachable, "of", profile_name);
            return false;
        }
        profile.best_effort_when_unreachable = unreachable == "bestEffort";
        profile.abandon_when_no_positive = profile_json.get("abandonWhenNoPositive", false);
        if (auto weights_opt = profile_json.find<json::object>("nodeWeights")) {
            for (const auto& [type_name, weight_json] : weights_opt.value()) {
                const RoguelikeNodeType type = RoguelikeMapConfig::name2type(type_name);
                if (type == RoguelikeNodeType::Unknown) {
                    return false;
                }
                profile.node_weights[type] = weight_json.as_double();
            }
        }
        profile.gear_use_cost = profile_json.get("gearUseCost", 0.5);
        profile.non_carryover_use_cost = profile_json.get("nonCarryoverUseCost", 0.0);
        if (auto rewards_opt = profile_json.find<json::object>("gearUseReward")) {
            for (const auto& [gear_name, reward_json] : rewards_opt.value()) {
                profile.gear_use_reward[gear_name] = reward_json.as_double();
            }
        }
        profile.leftover_ap_weight = profile_json.get("leftoverApWeight", 0.1);
        m_strategies.emplace(profile_name, std::move(profile));
    }

    for (const auto& [mode_str, strategy_json] : json.at("modeStrategies").as_object()) {
        int mode = 0;
        try {
            mode = std::stoi(mode_str);
        }
        catch (const std::exception&) {
            Log.error(__FUNCTION__, "| Invalid mode key in modeStrategies:", mode_str);
            return false;
        }
        const std::string strategy_name = strategy_json.as_string();
        if (!m_strategies.contains(strategy_name)) {
            Log.error(__FUNCTION__, "| modeStrategies refers to unknown strategy:", strategy_name);
            return false;
        }
        m_mode_strategies.emplace(mode, strategy_name);
    }

    return true;
}
