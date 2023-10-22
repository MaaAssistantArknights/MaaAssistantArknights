#pragma once

#include "Config/AbstractConfig.h"

#include <vector>

#include "Common/AsstBattleDef.h"

namespace asst
{
    // 板子组合
    struct CiphertextBoardPair
    {
        std::vector<std::string> up_board;   // 上板子
        std::vector<std::string> down_board; // 下板子
    };
    struct RoguelikeCiphertextBoardCombination
    {
        std::string usage;                      // 适用的节点类型和用法
        std::vector<CiphertextBoardPair> pairs; // 适用的板子组合
    };

    class RoguelikeCiphertextBoardConfig final : public SingletonHolder<RoguelikeCiphertextBoardConfig>,
                                                 public AbstractConfig
    {
    public:
        virtual ~RoguelikeCiphertextBoardConfig() override = default;

        const auto& get_combination(const std::string& theme) const noexcept
        {
            return m_ciphertext_board_combination.at(theme);
        }

    private:
        virtual bool parse(const json::value& json) override;

        std::unordered_map<std::string, std::vector<RoguelikeCiphertextBoardCombination>>
            m_ciphertext_board_combination;
    };

    inline static auto& RoguelikeCiphertextBoard = RoguelikeCiphertextBoardConfig::get_instance();
}
