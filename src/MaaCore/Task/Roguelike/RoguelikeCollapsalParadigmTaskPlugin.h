#pragma once
#include "AbstractRoguelikeTaskPlugin.h"
#include "Common/AsstTypes.h"
#include "Vision/OCRer.h"
#include "Config/TaskData.h"

namespace asst
{
    class RoguelikeCollapsalParadigmTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        RoguelikeCollapsalParadigmTaskPlugin(const AsstCallback& callback,
                                             Assistant* inst,
                                             std::string_view task_chain,
                                             std::shared_ptr<RoguelikeConfig> config)
                                             : AbstractRoguelikeTaskPlugin(callback, inst, task_chain, config)
        {
            const std::string& theme = config->get_theme();
            
            std::shared_ptr<OcrTaskInfo> bannerCheckConfig =
                Task.get<OcrTaskInfo>(theme + "@Roguelike@CheckCollapsalParadigms_bannerCheckConfig");
            m_deepen_text = bannerCheckConfig->text.front();
            m_banner_triggers_start.insert(bannerCheckConfig->sub.begin(), bannerCheckConfig->sub.end());
            m_banner_triggers_completed.insert(bannerCheckConfig->next.begin(), bannerCheckConfig->next.end());

            std::shared_ptr<OcrTaskInfo> panelCheckConfig =
                Task.get<OcrTaskInfo>(theme + "@Roguelike@CheckCollapsalParadigms_panelCheckConfig");
            m_roi = panelCheckConfig->roi;
            m_swipe_begin.x = panelCheckConfig->specific_rect.x;
            m_swipe_begin.y = panelCheckConfig->specific_rect.y;
            m_swipe_end.x = panelCheckConfig->rect_move.x;
            m_swipe_end.y = panelCheckConfig->rect_move.y;
            m_panel_triggers.insert(panelCheckConfig->sub.begin(), panelCheckConfig->sub.end());
            std::ranges::for_each(panelCheckConfig->replace_map,
                [&](const std::pair<std::string, std::string>& p) { m_zone_dict.emplace(p.first, p.second); });
        }
        // using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        virtual ~RoguelikeCollapsalParadigmTaskPlugin() override = default;

        static bool enabled(std::shared_ptr<RoguelikeConfig> config) {
            return config->get_theme() == RoguelikeTheme::Sami && config->get_check_clp_pds();
        }

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

        bool check_collapsal_paradigm_banner();

    protected:
        virtual bool _run() override;

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

        std::function<bool(const OCRer::Result&, const OCRer::Result&)> m_compare =
            [](const OCRer::Result& x1, const OCRer::Result& x2){ return x1.rect.y < x2.rect.y; };

        bool new_zone() const;

        bool check_collapsal_paradigm_panel();

        void toggle_collapsal_status_panel();
        void exit_then_restart();
        void exit_then_stop();
        void wait_for_loading(unsigned int millisecond = 0);
        void wait_for_stage(unsigned int millisecond = 0);
        void clp_pd_callback(std::string cur, int deepen_or_weaken = 0, std::string prev = "");
    };
}
