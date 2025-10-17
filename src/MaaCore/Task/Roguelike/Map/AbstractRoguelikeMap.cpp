#include "AbstractRoguelikeMap.h"
#include "RoguelikeBoskyPassageMap.h"

#include <unordered_map>

namespace asst
{

// 共用的节点类型映射表
static const std::unordered_map<std::string, RoguelikeNodeType> NodeTypeMapping = {
    { "CombatOps", RoguelikeNodeType::CombatOps },
    { "EmergencyOps", RoguelikeNodeType::EmergencyOps },
    { "DreadfulFoe", RoguelikeNodeType::DreadfulFoe },
    { "Encounter", RoguelikeNodeType::Encounter },
    { "Boons", RoguelikeNodeType::Boons },
    { "SafeHouse", RoguelikeNodeType::SafeHouse },
    { "Recreation", RoguelikeNodeType::Recreation },
    { "RogueTrader", RoguelikeNodeType::RogueTrader },
    { "RegionalCommissions", RoguelikeNodeType::RegionalCommissions },
    { "LostAndFound", RoguelikeNodeType::LostAndFound },
    { "Scout", RoguelikeNodeType::Scout },
    { "BoskyPassage", RoguelikeNodeType::BoskyPassage },
    { "Prophecy", RoguelikeNodeType::Prophecy },
    { "MysteriousPresage", RoguelikeNodeType::MysteriousPresage },
    { "FerociousPresage", RoguelikeNodeType::FerociousPresage },
    { "IdeaFilter", RoguelikeNodeType::IdeaFilter },
    { "FaceOff", RoguelikeNodeType::FaceOff },
    { "Legend", RoguelikeNodeType::Legend },
    { "Omissions", RoguelikeNodeType::Omissions },
    { "Doubts", RoguelikeNodeType::Doubts },
    { "Disaster", RoguelikeNodeType::Disaster },
    { "Playtime", RoguelikeNodeType::Playtime },
    { "OldShop", RoguelikeNodeType::OldShop },
    { "YiTrader", RoguelikeNodeType::YiTrader },
    { "Scheme", RoguelikeNodeType::Scheme }
};

RoguelikeNodeType name2type(const std::string& node_name)
{
    auto it = NodeTypeMapping.find(node_name);
    return it != NodeTypeMapping.end() ? it->second : RoguelikeNodeType::Unknown;
}

std::string type2name(RoguelikeNodeType node_type)
{
    static const std::unordered_map<RoguelikeNodeType, std::string> reverse_mapping = []() {
        std::unordered_map<RoguelikeNodeType, std::string> mapping;
        for (const auto& [name, type] : NodeTypeMapping) {
            mapping[type] = name;
        }
        return mapping;
    }();

    auto it = reverse_mapping.find(node_type);
    return it != reverse_mapping.end() ? it->second : "Unknown";
}

std::string subtype2name(RoguelikeBoskySubNodeType sub_node_type)
{
    // 界园树洞子节点类型映射表
    static const std::unordered_map<RoguelikeBoskySubNodeType, std::string> subtype_mapping = {
        { RoguelikeBoskySubNodeType::Unknown, "Unknown" },
        { RoguelikeBoskySubNodeType::Ling, "Ling" }, // 令 - 常乐 掷地有声
        { RoguelikeBoskySubNodeType::Shu, "Shu" },   // 黍 - 常乐 种因得果
        { RoguelikeBoskySubNodeType::Nian, "Nian" }  // 年 - 常乐 三缺一
    };

    auto it = subtype_mapping.find(sub_node_type);
    return it != subtype_mapping.end() ? it->second : "Unknown";
}

} // namespace asst
