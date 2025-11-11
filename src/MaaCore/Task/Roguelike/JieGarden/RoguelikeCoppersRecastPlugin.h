#pragma once

#include "Task/AbstractTask.h"

#include <optional>

namespace asst
{
struct RoguelikeCoppersRecastSnapshot
{
    int recast_times = 0;            // 当前重新投钱次数
    std::optional<int> hp;           // 当前生命值
    std::optional<int> hope;         // 当前希望值
    std::optional<int> ingot;        // 当前源石锭数
    std::optional<int> ticket_count; // 当前票卷数
};

// 重新投钱停止条件 票卷/生命/次数 大于/小于/等于 某个值
class RoguelikeCoppersRecastCondition
{
public:
    // 条件类型
    enum ConditionType
    {
        None = -1,
        Count = 0,
        HP = 1,
        Hope = 2,
        Ingot = 3,
        Ticket = 4,
    };

    // 比较方式
    enum CompareType
    {
        EqualTo = 0,
        GreaterThan = 1,
        LessThan = 2,
        GreaterEqual = 3,
        LessEqual = 4,
    };

    RoguelikeCoppersRecastCondition() = default;

    RoguelikeCoppersRecastCondition(int val, ConditionType cond_type, CompareType comp_type) :
        value(val),
        condition_type(cond_type),
        compare_type(comp_type)
    {
    }

    // 评估是否满足条件
    bool evaluate(const RoguelikeCoppersRecastSnapshot& snapshot) const;

private:
    int value = 0;
    ConditionType condition_type = ConditionType::None;
    CompareType compare_type = CompareType::EqualTo;
};

class RoguelikeCoppersRecastPlugin final : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~RoguelikeCoppersRecastPlugin() override = default;

    bool configure(const json::value& payload);
    void set_conditions(std::vector<RoguelikeCoppersRecastCondition> conditions) noexcept;
    void clear_conditions() noexcept;

private:
    virtual bool _run() override;

    void reset_snapshot() noexcept;

    RoguelikeCoppersRecastSnapshot m_snapshot;                 // 存放当前状态
    std::vector<RoguelikeCoppersRecastCondition> m_conditions; // 停止条件
};
} // namespace asst
