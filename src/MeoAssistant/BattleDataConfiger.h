#pragma once
#include "AbstractConfiger.h"

#include <unordered_map>

#include "AsstTypes.h"
#include "AsstBattleDef.h"

namespace asst
{
    class BattleDataConfiger : public AbstractConfiger
    {
    public:
        virtual ~BattleDataConfiger() override = default;

        BattleRole get_role(const std::string& name) const
        {
            auto iter = m_chars.find(name);
            if (iter == m_chars.cend()) {
                return BattleRole::Unknown;
            }
            return iter->second.role;
        }

        int get_rarity(const std::string& name) const
        {
            auto iter = m_chars.find(name);
            if (iter == m_chars.cend()) {
                return 0;
            }
            return iter->second.rarity;
        }

        static inline const BattleAttackRange& EmptyRange{ {0, 0} };

        const BattleAttackRange& get_range(const std::string& name, size_t index) const
        {
            auto char_iter = m_chars.find(name);
            if (char_iter == m_chars.cend()) {
                return EmptyRange;
            }
            const auto& ranges = char_iter->second.ranges;
            if (index >= ranges.size()) {
                if (ranges.empty()) {
                    return EmptyRange;
                }
                index = 0;
            }

            const std::string& range_name = ranges.at(index);
            auto range_iter = m_ranges.find(range_name);
            if (range_iter == m_ranges.cend()) {
                return EmptyRange;
            }
            return range_iter->second;
        }

    protected:
        virtual bool parse(const json::value& json) override;

    private:
        std::unordered_map<std::string, BattleCharData> m_chars;
        std::unordered_map<std::string, BattleAttackRange> m_ranges;
    };
}
