#include "RoguelikeFoldartalConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::RoguelikeFoldartalConfig::parse(const json::value& json)
{
    LogTraceFunction;

    const std::string theme = json.at("theme").as_string();
    m_foldartal_combination.erase(theme);

    for (const auto& combination_json : json.at("groups").as_array()) {
        RoguelikeFoldartalCombination combination;
        combination.usage = combination_json.get("usage", "none");

        for (const auto& pairs : combination_json.at("pairs").as_array()) {
            FoldartalPair pair;
            const json::array& up_boards = pairs.at("up").as_array();
            for (const auto& board : up_boards) {
                pair.up_board.emplace_back(board.as_string());
            }
            const json::array& down_boards = pairs.at("down").as_array();
            for (const auto& board : down_boards) {
                pair.down_board.emplace_back(board.as_string());
            }
            combination.pairs.emplace_back(pair);
        }
        m_foldartal_combination[theme].emplace_back(std::move(combination));
    }

    return true;
}
