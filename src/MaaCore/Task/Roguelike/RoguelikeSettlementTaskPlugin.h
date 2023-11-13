#pragma once
#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{
    class RoguelikeSettlementTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        virtual ~RoguelikeSettlementTaskPlugin() override = default;
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    private:
        virtual bool _run() override;
        bool get_settlement_info(json::value& info, const cv::Mat& image);
        bool wait_for_whole_page();

        mutable bool m_game_pass = false;
    };
} // namespace asst
