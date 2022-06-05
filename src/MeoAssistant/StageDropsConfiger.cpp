#include "StageDropsConfiger.h"

#include <limits>
#include <meojson/json.hpp>

bool asst::StageDropsConfiger::parse(const json::value& json)
{
	for (const json::value& stage_json : json.as_array()) {
		if (!stage_json.contains("dropInfos")) { // 这种一般是以前的活动关，现在已经关闭了的
			continue;
		}
		std::string code = stage_json.at("code").as_string();           // 关卡名，举例 1-7，可能重复（例如磨难和普通是同一个关卡名）
		std::string stage_id = stage_json.at("stageId").as_string();    // 关卡id，举例 main_01-07，上传用的，唯一
		StageDifficulty difficulty = (stage_id.find("tough_") == 0) ? StageDifficulty::Tough : StageDifficulty::Normal;
		StageKey key{ code, difficulty };

		StageInfo info;
		info.stage_id = stage_id;
		info.ap_cost = stage_json.get("apCost", 0);
		for (const json::value& drops_json : stage_json.at("dropInfos").as_array()) {
			static const std::unordered_map<std::string, StageDropType> TypeMapping = {
				{ "NORMAL_DROP", StageDropType::Normal },
				{ "EXTRA_DROP", StageDropType::Extra },
				{ "FURNITURE", StageDropType::Furniture },
				{ "SPECIAL_DROP", StageDropType::Special }
			};
			StageDropType type = TypeMapping.at(drops_json.at("dropType").as_string());
			std::string item_id = drops_json.get("itemId", std::string());
			if (item_id.empty()) {
				continue;
			}
			info.drops[type].emplace_back(item_id);
			m_all_item_id.emplace(item_id);
		}

		for(const auto& [server_name, server_json] : stage_json.at("existence").as_object()) {
			if(!server_json.at("exist").as_boolean()) {
				continue;
			}
			info.openTime = (unsigned long)server_json.get("openTime",(time_t)0);
			info.closeTime = (unsigned long)server_json.get("closeTime", (std::numeric_limits<time_t>::max()));
			StageKey server_key = {key.code + server_name, key.difficulty};
			m_stage_info.insert_or_assign(std::move(server_key), info);
		}
		m_all_stage_code.emplace(code);
	}
	return true;
}