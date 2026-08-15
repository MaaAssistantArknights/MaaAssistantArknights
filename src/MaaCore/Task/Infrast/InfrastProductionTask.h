#pragma once

#include "InfrastAbstractTask.h"

#include "Common/AsstInfrastDef.h"

namespace asst
{
// 生产类设施的任务，适用于制造站、贸易站、控制中枢
class InfrastProductionTask : public InfrastAbstractTask
{
public:
    using InfrastAbstractTask::InfrastAbstractTask;
    virtual ~InfrastProductionTask() override = default;

    // 来自params的无人机使用参数，自定义基建配置启用时不触发
    InfrastProductionTask& set_drones_usage_from_params(std::string usage) noexcept;
    std::string get_drones_usage_from_params() const noexcept;
    // 无人机自动平衡（贸易站消耗至低于阈值），参数为模式（"PureGold-Money"/"OriginStone-SyntheticJade"），空表示未启用
    InfrastProductionTask& set_drones_balance_config(std::string mode, int threshold) noexcept;
    void set_custom_drones_config(infrast::CustomDronesConfig drones_config);
    void clear_custom_drones_config();

    void set_skip_shift(bool skip) noexcept { m_skip_shift = skip; }

protected:
    bool shift_facility_list();
    bool facility_list_detect();
    bool opers_detect_with_swipe();
    // 返回当前页面的干员数 (可用?
    size_t opers_detect();
    bool optimal_calc();
    bool opers_choose();
    bool use_drone();
    // 无人机自动平衡专用：单次加速并完成一个贸易站订单（点加速→MAX→确认），返回是否成功
    bool use_drone_once();
    // 无人机自动平衡专用：提交当前已完成的贸易站订单（交付），返回是否成功
    bool use_drone_once_deliver();
    // 无人机自动平衡专用：判断第一格是否处于「无未完成订单」状态（加速按钮可见）
    bool is_accelerate_button_visible();
    void set_product(std::string product_name) noexcept;
    // 贸易站无人机自动平衡：识别当前物品剩余量并加速消耗订单直至低于阈值，尽力而为
    void drones_balance_consume();
    // 识别贸易站界面当前订单的「库存/消耗量」（OCR 形如 X/Y，999+ 按 1000 计），成功返回 true；失败表示第一格处于「无未完成订单」状态
    bool try_get_stock_and_consumption(int& stock, int& consumption);

    infrast::SkillsComb efficient_regex_calc(std::unordered_set<infrast::Skill> skills) const;

    std::string m_product;
    std::string m_drones_usage_from_params;
    // 无人机自动平衡模式（"PureGold-Money"/"OriginStone-SyntheticJade"），为空表示未启用
    std::string m_drones_balance_mode;
    // 无人机自动平衡阈值：剩余量小于该值即停止消耗
    int m_drones_balance_threshold = 10;
    // 防止一个 visit 内重复执行消耗循环
    bool m_drones_balance_consume_done = true;
    int m_cur_num_of_locked_opers = 0;
    std::vector<infrast::Oper> m_all_available_opers;
    std::vector<infrast::SkillsComb> m_optimal_combs;
    std::vector<Rect> m_facility_list_tabs;
    size_t max_num_of_opers_per_page = 0;
    // 来自自定义基建配置的无人机使用参数
    bool m_is_use_drones_from_custom = false;
    infrast::CustomDronesConfig m_custom_drones_config;
    bool m_skip_shift = false;

protected:
    bool change_product();
    bool m_is_product_incorrect = false;
};
}
