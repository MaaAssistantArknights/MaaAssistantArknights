#pragma once
#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{
    // 肉鸽投资插件
    class RoguelikeInvestTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        virtual ~RoguelikeInvestTaskPlugin() override = default;
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    private:
        virtual bool _run() override;
        std::optional<int> ocr_current_count(const auto& img, const auto& task_name);
        bool is_investment_available(const cv::Mat& image) const;
        bool is_investment_error(const cv::Mat& image) const;
        void stop_roguelike();


        int m_invest_count = 0;
    };
} // namespace asst
