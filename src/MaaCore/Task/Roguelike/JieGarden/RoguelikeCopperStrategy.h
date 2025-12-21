#pragma once

#include "Config/Roguelike/JieGarden/RoguelikeCoppersConfig.h"
#include <functional>
#include <string>
#include <vector>

namespace asst
{

struct CopperPickupResult
{
    size_t selected_index = 0;
    std::string reason;
};

struct CopperExchangeResult
{
    bool should_exchange = false;
    size_t discard_index = 0;
    std::string reason;
};

enum class CopperStrategyType
{
    Default,
    FindLingMode
};

using CopperPickupStrategyFunction =
    std::function<CopperPickupResult(const std::vector<RoguelikeCopper>&, const std::vector<RoguelikeCopper>&)>;

using CopperExchangeStrategyFunction =
    std::function<CopperExchangeResult(const RoguelikeCopper&, const std::vector<RoguelikeCopper>&)>;

struct CopperStrategy
{
    CopperPickupStrategyFunction pickup;
    CopperExchangeStrategyFunction exchange;
};

CopperStrategy get_copper_strategy(CopperStrategyType type);
}
