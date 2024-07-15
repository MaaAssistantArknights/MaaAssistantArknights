#pragma once

#include "Config/AbstractConfig.h"

#include <vector>

#include "Common/AsstBattleDef.h"

namespace asst
{
    // 板子组合
    struct FoldartalPair
    {
        std::vector<std::string> up_board;   // 上板子
        std::vector<std::string> down_board; // 下板子
    };
    struct RoguelikeFoldartalCombination
    {
        std::string usage;                // 适用的节点类型和用法
        std::vector<FoldartalPair> pairs; // 适用的板子组合
    };

    class RoguelikeFoldartalConfig final : public SingletonHolder<RoguelikeFoldartalConfig>, public AbstractConfig
    {
    public:
        virtual ~RoguelikeFoldartalConfig() override = default;

        const auto& get_combination(const std::string& theme) const noexcept
        {
            return m_foldartal_combination.at(theme);
        }

    private:
        virtual bool parse(const json::value& json) override;

        std::unordered_map<std::string, std::vector<RoguelikeFoldartalCombination>> m_foldartal_combination;
    };

    inline static auto& RoguelikeFoldartal = RoguelikeFoldartalConfig::get_instance();
}
