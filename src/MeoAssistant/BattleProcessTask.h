#pragma once
#include "AbstractTask.h"

#include "AsstTypes.h"
#include "TilePack.h"
#include "AsstBattleDef.h"
#include "BattleImageAnalyzer.h"

namespace asst
{
    class BattleProcessTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~BattleProcessTask() = default;

        void set_stage_name(std::string name);

    protected:
        virtual bool _run() override;

        bool get_stage_info();
        bool battle_pause();
        bool battle_speedup();
        bool cancel_selection();  // 取消选择干员
        bool analyze_opers_preview();
        bool update_opers_info();

        bool do_action(const BattleAction& action);
        bool wait_condition(const BattleAction& action);

        bool oper_deploy(const BattleAction& action);
        bool use_skill(const BattleAction& action);

        std::string m_stage_name;

        std::unordered_map<Point, TilePack::TileInfo> m_side_tile_info;
        std::unordered_map<Point, TilePack::TileInfo> m_normal_tile_info;
        BattleActionsGroup m_actions;

        /* 实时更新的数据 */
        std::unordered_map<std::string, BattleRealTimeOper> m_opers_info;
        std::unordered_map<std::string, Point> m_used_opers_loc;
    };
}
