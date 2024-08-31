#pragma once
#include "Task/AbstractTaskPlugin.h"
#include "Task/BattleHelper.h"
#include "Task/Miscellaneous/BattleProcessTask.h"

namespace asst
{
    enum class ReclamationBattleMode
    {
        Giveup,
        BuyWater
    };

    enum class ReclamationTaskMode;

    class ReclamationBattlePlugin : public AbstractTaskPlugin, private BattleHelper
    {
    public:
        ReclamationBattlePlugin(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
        virtual ~ReclamationBattlePlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

        ReclamationBattlePlugin& set_battle_mode(const ReclamationBattleMode& mode);

    protected:
        virtual bool _run() override;
        virtual bool do_strategic_action(const cv::Mat& reusable = cv::Mat()) override;
        virtual AbstractTask& this_task() override { return *this; }

        bool quit_action();

        bool buy_water();
        bool communicate_with(const std::string& npcName);
        bool communicate_with_aux(const std::string& npcName,
                                  std::function<bool(const MatchRect&, const MatchRect&)> orderComp);
        bool do_dialog_procedure(const std::vector<std::string>& procedure);

        ReclamationBattleMode m_battle_mode = ReclamationBattleMode::Giveup;
    };
}
