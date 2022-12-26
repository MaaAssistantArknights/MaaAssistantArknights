#pragma once
#include "Config/AbstractConfig.h"

#include <unordered_map>

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"

namespace asst
{
    class BattleDataConfig final : public SingletonHolder<BattleDataConfig>, public AbstractConfig
    {
    public:
        virtual ~BattleDataConfig() override = default;

        battle::Role get_role(const std::string& name) const
        {
            auto iter = m_chars.find(name);
            if (iter == m_chars.cend()) {
                return battle::Role::Unknown;
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

        battle::LocationType get_location_type(const std::string& name) const
        {
            auto iter = m_chars.find(name);
            if (iter == m_chars.cend()) {
                return battle::LocationType::Invalid;
            }
            return iter->second.location_type;
        }

        static inline const battle::AttackRange& EmptyRange { { 0, 0 } };

        const battle::AttackRange& get_range(const std::string& name, size_t index) const
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

        const std::vector<std::string>& get_tokens(const std::string& name) const
        {
            auto char_iter = m_chars.find(name);
            if (char_iter == m_chars.cend()) {
                static const std::vector<std::string> Empty;
                return Empty;
            }
            return char_iter->second.tokens;
        }

    protected:
        virtual bool parse(const json::value& json) override;

    private:
        std::unordered_map<std::string, battle::OperProps> m_chars;
        std::unordered_map<std::string, battle::AttackRange> m_ranges;
    };
    inline static auto& BattleData = BattleDataConfig::get_instance();
} // namespace asst
