#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace asst::infrast
{
// 极简模式仅按无人机用途和会客室选项安排设施，不读取换班设施列表。
inline std::vector<std::string> build_simple_facilities(std::string_view drones, bool reception_enabled)
{
    std::vector<std::string> facilities;
    if (drones == "Money" || drones == "SyntheticJade") {
        facilities.emplace_back("Trade");
    }
    else if (drones == "CombatRecord" || drones == "PureGold" || drones == "OriginStone" || drones == "Chip") {
        facilities.emplace_back("Mfg");
    }
    if (reception_enabled) {
        facilities.emplace_back("Reception");
    }
    return facilities;
}
}
