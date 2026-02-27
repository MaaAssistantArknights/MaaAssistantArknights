#pragma once

#include "Task/AbstractTaskPlugin.h"

#include <meojson/json.hpp>
#include <optional>

namespace asst
{
/**
 * @brief Plugin for automating JieGarden roguelike coppers recast mini-game
 *
 * This plugin continuously recasts coppers in the JieGarden roguelike mode
 * until any configured stop condition is met.
 *
 * 用于自动化界园肉鸽通宝重投小游戏的插件
 * 该插件会持续重投通宝,直到满足任一配置的停止条件
 */
class RoguelikeCoppersRecastPlugin : public AbstractTaskPlugin
{
public:
    /**
     * @brief Snapshot of current game state during recast
     * 重投过程中的游戏状态快照
     */
    struct Snapshot
    {
        int recast_times = 0;            ///< Number of recast attempts / 当前重新投钱次数
        std::optional<int> hp;           ///< Current HP value / 当前生命值
        std::optional<int> hope;         ///< Current hope value / 当前希望值
        std::optional<int> ingot;        ///< Current ingot count / 当前源石锭数
        std::optional<int> ticket_count; ///< Current ticket count / 当前票券数
    };

    /**
     * @brief Condition for stopping the recast operation
     * 重新投钱的停止条件
     *
     * Supports checking various metrics (HP, hope, ingot, ticket, recast count)
     * with different comparison operators (==, >, <, >=, <=).
     *
     * 支持检查各种指标(生命/希望/源石锭/票券/重投次数)
     * 配合不同的比较运算符(==, >, <, >=, <=)
     */
    class Condition
    {
    public:
        /**
         * @brief Type of metric to monitor
         * 监控的指标类型
         */
        enum class ConditionType
        {
            None = -1,        ///< Invalid type / 无效类型
            recast_count = 0, ///< Recast count / 重投次数
            hp = 1,           ///< Health points / 生命值
            hope = 2,         ///< Hope value / 希望值
            ingot = 3,        ///< Ingot count / 源石锭数
            ticket = 4,       ///< Ticket count / 票券数

            MEOJSON_ENUM_RANGE(None, ticket)
        };

        /**
         * @brief Type of comparison operator
         * 比较运算符类型
         */
        enum class CompareType
        {
            equal = 0,            ///< Equal to (==) / 等于
            greater = 1,          ///< Greater than (>) / 大于
            less = 2,             ///< Less than (<) / 小于
            greater_or_equal = 3, ///< Greater or equal (>=) / 大于等于
            less_or_equal = 4,    ///< Less or equal (<=) / 小于等于

            MEOJSON_ENUM_RANGE(equal, less_or_equal)
        };

        Condition() = default;

        /**
         * @brief Construct a condition with specified parameters
         * 构造一个带有指定参数的条件
         *
         * @param val Threshold value to compare against / 用于比较的阈值
         * @param cond_type Type of metric to monitor / 监控的指标类型
         * @param comp_type Type of comparison operator / 比较运算符类型
         */
        Condition(int val, ConditionType cond_type, CompareType comp_type) :
            value(val),
            condition_type(cond_type),
            compare_type(comp_type)
        {
        }

        /**
         * @brief Construct a condition from JSON value
         * 从 JSON 值构造条件
         *
         * @param json JSON value to parse / 要解析的 JSON 值
         */
        Condition(const json::value& json)
        {
            std::string error_key;
            if (!from_json(json, error_key)) {
                throw std::runtime_error("Failed to parse Condition from JSON: " + error_key);
            }
        }

        /**
         * @brief Evaluate if the condition is satisfied with current snapshot
         * 评估当前快照是否满足此条件
         *
         * @param snapshot Current game state / 当前游戏状态
         * @return true if condition is met, false otherwise / 满足条件返回true,否则返回false
         */
        bool evaluate(const Snapshot& snapshot) const;

    private:
        int value = 0;                                      ///< Threshold value / 阈值
        ConditionType condition_type = ConditionType::None; ///< Metric type / 指标类型
        CompareType compare_type = CompareType::equal;      ///< Comparison operator / 比较运算符

        MEO_JSONIZATION(
            MEO_KEY("threshold") value,
            MEO_KEY("metric") condition_type,
            MEO_KEY("comparator") compare_type)
    };

    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~RoguelikeCoppersRecastPlugin() override = default;

    /**
     * @brief Load parameters from JSON configuration
     * 从JSON配置加载参数
     *
     * Expected JSON format:
     * {
     *   "conditions": [
     *     {
     *       "metric": "hp|hope|ingot|ticket|recast_count",
     *       "comparator": "equal|less|greater|less_or_equal|greater_or_equal",
     *       "threshold": <integer>
     *     }
     *   ]
     * }
     *
     * @param params JSON parameters / JSON参数
     * @return true on success / 成功返回true
     */
    bool load_params(const json::value& params);

    /**
     * @brief Verify if this plugin should handle the message
     * 验证此插件是否应处理该消息
     *
     * @param msg Message type / 消息类型
     * @param details Message details in JSON / JSON格式的消息详情
     * @return true if plugin should handle / 应处理返回true
     */
    virtual bool verify(AsstMsg msg, const json::value& details) const override;

private:
    /**
     * @brief Main execution logic
     * 主要执行逻辑
     *
     * @return true on success or user stop, false on error / 成功或用户停止返回true,错误返回false
     */
    virtual bool _run() override;

    Snapshot m_snapshot;                 ///< Current game state / 当前游戏状态快照
    std::vector<Condition> m_conditions; ///< Stop conditions / 停止条件列表
};
} // namespace asst
