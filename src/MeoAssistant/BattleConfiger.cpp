#include "BattleConfiger.h"

#include <meojson/json.hpp>

#include "Logger.hpp"

bool asst::BattleConfiger::parse(const json::value& json)
{
	std::string stage_name = json.at("stage_name").as_string();

	BattleActionsGroup battle_actions;

	if (json.contains("opers_groups")) {
		for (const auto& group_info : json.at("opers_groups").as_array()) {
			std::string group_name = group_info.at("name").as_string();
			std::vector<BattleDeployOper> oper_vec;
			for (const auto& oper_info : group_info.at("opers").as_array()) {
				BattleDeployOper oper;
				oper.name = oper_info.at("name").as_string();
				oper.skill = oper_info.get("skill", 1);
				oper.skill_usage = static_cast<BattleSkillUsage>(oper_info.get("skill_usage", 0));
				oper_vec.emplace_back(std::move(oper));
			}
			battle_actions.opers_groups.emplace(std::move(group_name), std::move(oper_vec));
		}
	}

	if (json.contains("opers")) {
		for (const auto& oper_info : json.at("opers").as_array()) {
			BattleDeployOper oper;
			oper.name = oper_info.at("name").as_string();
			oper.skill = oper_info.get("skill", 1);
			oper.skill_usage = static_cast<BattleSkillUsage>(oper_info.get("skill_usage", 0));

			// 单个干员的，干员名直接作为组名
			std::string group_name = oper.name;

			battle_actions.opers_groups.emplace(std::move(group_name), std::vector{ std::move(oper) });
		}
	}

	for (const auto& action_info : json.at("actions").as_array()) {
		BattleAction action;
		action.type = static_cast<BattleActionType>(action_info.get("type", 0));
		action.kills = action_info.get("kills", 0);
		action.group_name = action_info.get("name", std::string());

		action.location.x = action_info.get("location", 0, 0);
		action.location.y = action_info.get("location", 1, 0);

		action.direction = static_cast<BattleDeployDirection>(action_info.get("direction", 0));
		action.pre_delay = action_info.get("pre_delay", 0);
		action.rear_delay = action_info.get("rear_delay", 0);
		action.time_out = action_info.get("timeout", INT_MAX);

		battle_actions.actions.emplace_back(std::move(action));
	}

	m_battle_actions[std::move(stage_name)] = std::move(battle_actions);

	return true;
}