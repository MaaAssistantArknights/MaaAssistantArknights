#pragma once

#include "Config/Roguelike/JieGarden/RoguelikeCoppersConfig.h"
#include <functional>
#include <string>
#include <vector>

namespace asst
{

// 通宝交换策略结果
struct CopperExchangeResult
{
    bool should_exchange = false; // 是否决定交换
    size_t discard_index = 0;     // 要丢弃的通宝索引
    std::string reason;           // 决策原因
};

// 通宝交换策略类型枚举
enum class CopperExchangeStrategyType
{
    Default,     // 默认策略：基于优先级
    FindLingMode // FindLing模式：优先获取厉钱
};

// 通宝交换策略函数类型
// 参数：新通宝、现有通宝列表
// 返回：交换决策结果
using CopperExchangeStrategyFunction =
    std::function<CopperExchangeResult(const RoguelikeCopper&, const std::vector<RoguelikeCopper>&)>;
// 根据策略类型获取策略函数
CopperExchangeStrategyFunction get_exchange_strategy_function(CopperExchangeStrategyType type);
}
