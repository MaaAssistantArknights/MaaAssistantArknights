#pragma once
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/AbstractTaskPlugin.h"
#include "Task/BattleHelper.h"

namespace asst
{
    class ReclamationBattlePlugin : public AbstractTaskPlugin, private BattleHelper
    {
    public:
        ReclamationBattlePlugin(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
        virtual ~ReclamationBattlePlugin() override = default;
        
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
        virtual bool do_strategic_action(const cv::Mat& reusable = cv::Mat()) override;
        virtual AbstractTask& this_task() override { return *this; }
    };
}
