#include "StageDropsConfig.h"

#include "Utils/Logger.hpp"

#include <meojson/json.hpp>

bool asst::StageDropsConfig::parse(const json::value& json)
{
    LogTraceFunction;

    for (const json::value& stage_json : json.as_array()) {
        auto drop_infos_opt = stage_json.find<json::array>("dropInfos");
        if (!drop_infos_opt) { // 这种一般是以前的活动关，现在已经关闭了的
            continue;
        }
        std::string code = stage_json.at("code").as_string(); // 关卡名，
        // 举例 1-7，可能重复（例如磨难和普通是同一个关卡名）
        std::string stage_id = stage_json.at("stageId").as_string(); // 关卡id，举例 main_01-07，上传用的，唯一
        StageDifficulty difficulty = (stage_id.find("tough_") == 0) ? StageDifficulty::Tough : StageDifficulty::Normal;
        StageKey key { code, difficulty };

        StageInfo info;
        info.stage_id = stage_id;
        info.ap_cost = stage_json.get("apCost", 0);
        for (const json::value& drops_json : drop_infos_opt.value()) {
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

        m_all_stage_code.emplace(code);
        m_stage_info.emplace(std::move(key), std::move(info));
    }

    return true;
}
