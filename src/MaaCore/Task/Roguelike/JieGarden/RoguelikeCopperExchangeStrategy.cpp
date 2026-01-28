#include "RoguelikeCopperExchangeStrategy.h"
#include "Utils/Logger.hpp"
#include <algorithm>

namespace asst
{

// 默认策略：基于优先级选择交换
CopperExchangeResult
    default_exchange_strategy(const RoguelikeCopper& new_copper, const std::vector<RoguelikeCopper>& existing_coppers)
{
    CopperExchangeResult result;

    if (existing_coppers.empty()) {
        result.reason = "No existing coppers to exchange";
        return result;
    }

    // 找到丢弃优先级最高的通宝（最不重要的）
    auto worst_it = std::max_element(
        existing_coppers.begin(),
        existing_coppers.end(),
        [](const RoguelikeCopper& a, const RoguelikeCopper& b) {
            return a.get_copper_discard_priority() < b.get_copper_discard_priority();
        });

    // 如果新通宝的丢弃优先级高于最差现有通宝，则进行交换
    if (worst_it->get_copper_discard_priority() > new_copper.get_copper_discard_priority()) {
        result.should_exchange = true;
        result.discard_index = std::distance(existing_coppers.begin(), worst_it);
        result.reason = "New copper has higher priority";
    }
    else {
        result.reason = "New copper has lower priority than existing coppers";
    }

    return result;
}

// FindLing模式策略：优先获取厉钱
CopperExchangeResult
    findling_exchange_strategy(const RoguelikeCopper& new_copper, const std::vector<RoguelikeCopper>& existing_coppers)
{
    CopperExchangeResult result;

    if (existing_coppers.empty()) {
        result.reason = "No existing coppers to exchange";
        return result;
    }

    // 在FindLing模式下，优先获取厉钱类型的通宝
    if (new_copper.type == CopperType::Li) {
        // 找到优先级最低的通宝进行交换
        auto worst_it = std::max_element(
            existing_coppers.begin(),
            existing_coppers.end(),
            [](const RoguelikeCopper& a, const RoguelikeCopper& b) {
                return a.get_copper_discard_priority() < b.get_copper_discard_priority();
            });

        result.should_exchange = true;
        result.discard_index = std::distance(existing_coppers.begin(), worst_it);
        result.reason = "FindLing mode: prioritize Li-type copper";
        return result;
    }

    // 对于非厉钱类型的新通宝，检查是否有非厉钱类型的现有通宝可以替换
    for (size_t i = 0; i < existing_coppers.size(); ++i) {
        if (existing_coppers[i].type != CopperType::Li &&
            existing_coppers[i].get_copper_discard_priority() > new_copper.get_copper_discard_priority()) {
            result.should_exchange = true;
            result.discard_index = i;
            result.reason = "FindLing mode: replace non-Li copper with better one";
            return result;
        }
    }

    result.reason = "FindLing mode: non-Li copper or insufficient priority";
    return result;
}

// 根据策略类型获取策略函数
CopperExchangeStrategyFunction get_exchange_strategy_function(CopperExchangeStrategyType type)
{
    switch (type) {
    case CopperExchangeStrategyType::FindLingMode:
        return findling_exchange_strategy;
    case CopperExchangeStrategyType::Default:
    default:
        return default_exchange_strategy;
    }
}

}
