#pragma once
#include "../AbstractRoguelikeTaskPlugin.h"
#include "Common/AsstTypes.h"
#include "Config/TaskData.h"
#include "Vision/OCRer.h"

namespace asst
{
class RoguelikeCollapsalParadigmTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeCollapsalParadigmTaskPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual bool load_params(const json::value& params) override;
    virtual void reset_in_run_variables() override;

protected:
    virtual bool _run() override;

private:
    void check_banner();            // 检查坍缩范式变动通知
    void check_panel();             // 检查坍缩范式状态栏

    void toggle_panel() const;      // 开关坍缩范式状态栏

    void wait_for_loading() const;  // 等待"正在反馈至神经"结束
    void wait_for_stage() const;    // 等待回到地图

    void exit_then_restart() const; // 退出当前肉鸽局并重开
    void exit_then_stop() const;    // 退出当前肉鸽局并停止任务

    bool new_zone() const;          // 判断是否进入了新的区域

    // 向 GUI 回调坍缩范式变动情况，表示坍缩范式 prev 变动为 cur
    // deepen_or_weaken = 1 表示加深
    // deepen_or_weaken = -1 表示消退
    // deepen_or_weaken = 0 表示其它信息
    void clp_pd_callback(const std::string& cur, const int& deepen_or_weaken = 0, const std::string& prev = "");

    // ————————  局内临时变量，在 reset 时 重制 ——————————————————————————————————
    std::vector<std::string> m_clp_pds;        // 当前拥有的坍缩范式
    mutable bool m_check_banner = false;       // 在下一次 run 时运行 check_banner()
    mutable bool m_check_panel = false;        // 在下一次 run 时运行 check_panel()
    mutable bool m_need_check_panel = false;   // 是否在下次回到关卡选择界面时检查坍缩范式
    mutable bool m_verification_check = false; // 下一次 panel check 负责验证坍缩范式
    mutable std::string m_zone;                // 当前所在区域

    // ———————— 插件配置信息，从 params 中读取  ——————————————————————————————————
    bool m_double_check_clp_pds = false; // 是否在每次回到地图时验证坍缩范式

    // ———————— 插件配置信息，从 tasks.json 中读取  ——————————————————————————————
    std::string m_deepen_text; // 坍缩范式变动通知中"坍缩范式加深"的文字

    std::unordered_set<std::string> m_banner_triggers_start;     // 通过 SubTaskStart 触发 banner check 的任务
    std::unordered_set<std::string> m_banner_triggers_completed; // 通过 SubTaskCompleted 触发 banner check 的任务
    std::unordered_set<std::string> m_panel_triggers;            // 通过 SubTaskStart 触发 panel check 的任务

    std::unordered_map<std::string, std::string> m_zone_dict; // template 名与区域名的映射关系

    // ———————— 插件内部常量  ———————————————————————————————————————————————
    const std::string_view m_banner_check_error_message =
        "CollapsalParadigm ｜ Task Plugin: Erroneous Recognition in Banner Check";
};
}
