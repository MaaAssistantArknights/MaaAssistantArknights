#pragma once

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Vision/VisionHelper.h"

namespace asst
{
struct OperBoxInfo
{
    std::string id;
    std::string name;
    int level = 0;     // 等级
    int elite = 0;     // 精英度
    int potential = 0; // 潜能
    int rarity = 0;    // 稀有度

    Rect rect;
    bool own = false;

    struct SortCmp
    {
        bool operator()(const OperBoxInfo& lhs, const OperBoxInfo& rhs) const
        {
            if (lhs.elite != rhs.elite) {
                return lhs.elite > rhs.elite;
            }
            if (lhs.level != rhs.level) {
                return lhs.level > rhs.level;
            }
            if (lhs.rarity != rhs.rarity) {
                return lhs.rarity > rhs.rarity;
            }
            const auto& all_chars = BattleData.get_all_chars();
            const auto& lhs_iter = all_chars.at(lhs.id);
            const auto& rhs_iter = all_chars.at(rhs.id);
            if (lhs_iter->role != rhs_iter->role) {
                return lhs_iter->role < rhs_iter->role;
            }
            return lhs_iter->sort_index < rhs_iter->sort_index;
        }
    };
};

class OperBoxImageAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~OperBoxImageAnalyzer() override = default;
    bool analyze();

    const auto& get_result() const noexcept { return m_result; }

private:
    int level_num(const std::string& level);
    bool analyzer_oper_box();
    // 获取lv rect
    bool opers_analyze();
    bool level_analyze();
    bool elite_analyze();
    bool potential_analyze();

    std::vector<asst::OperBoxInfo> m_result;
};
}
