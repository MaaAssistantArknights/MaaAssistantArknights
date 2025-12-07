#pragma once

#include "Task/AbstractTaskPlugin.h"

#include <optional>

namespace asst
{
class RoguelikeCoppersRecastPlugin : public AbstractTaskPlugin
{
public:
    struct Snapshot
    {
        int recast_times = 0;            // 当前重新投钱次数
        std::optional<int> hp;           // 当前生命值
        std::optional<int> hope;         // 当前希望值
        std::optional<int> ingot;        // 当前源石锭数
        std::optional<int> ticket_count; // 当前票卷数
    };

    // 重新投钱停止条件 票卷/生命/次数 大于/小于/等于 某个值
    class Condition
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

        Condition() = default;

        Condition(int val, ConditionType cond_type, CompareType comp_type) :
            value(val),
            condition_type(cond_type),
            compare_type(comp_type)
        {
        }

        // 评估是否满足条件
        bool evaluate(const Snapshot& snapshot) const;

    private:
        int value = 0;
        ConditionType condition_type = ConditionType::None;
        CompareType compare_type = CompareType::EqualTo;
    };

    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~RoguelikeCoppersRecastPlugin() override = default;

    bool load_params(const json::value& params);
    void set_conditions(std::vector<Condition> conditions) noexcept;
    void clear_conditions() noexcept;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

private:
    virtual bool _run() override;

    void reset_snapshot() noexcept;

    Snapshot m_snapshot;                 // 存放当前状态
    std::vector<Condition> m_conditions; // 停止条件
};
} // namespace asst
