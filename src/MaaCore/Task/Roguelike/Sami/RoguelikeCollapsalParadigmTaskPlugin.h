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
    virtual bool set_params(const json::value& params) override;
    static bool enabled(std::shared_ptr<RoguelikeConfig> config)
    {
        return config->get_theme() == RoguelikeTheme::Sami && config->get_check_clp_pds();
    }

public:
    void set_double_check_clp_pds(bool value) { m_double_check_clp_pds = value; }
    bool check_collapsal_paradigm_banner();

protected:
    virtual bool _run() override;

    bool new_zone() const;

    bool check_collapsal_paradigm_panel();

    void toggle_collapsal_status_panel();
    void exit_then_restart();
    void exit_then_stop();
    void wait_for_loading(unsigned int millisecond = 0);
    void wait_for_stage(unsigned int millisecond = 0);
    void clp_pd_callback(std::string cur, int deepen_or_weaken = 0, std::string prev = "");

private:
    mutable bool m_check_banner = false;
    mutable bool m_check_panel = false;
    mutable bool m_verification_check = false;

    std::string m_deepen_text;

    std::unordered_set<std::string> m_banner_triggers_start;

    std::unordered_set<std::string> m_banner_triggers_completed;

    mutable std::string m_zone;

    Rect m_roi;          // 屏幕上方居中区域，点击以检查坍缩状态
    Point m_swipe_begin; // 滑动起点
    Point m_swipe_end;   // 滑动终点

    std::unordered_set<std::string> m_panel_triggers;
    std::unordered_map<std::string, std::string> m_zone_dict;

    bool m_double_check_clp_pds = false; // 是否反复检查坍缩范式
};
}
