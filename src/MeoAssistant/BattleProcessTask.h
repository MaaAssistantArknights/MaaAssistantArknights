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

        virtual bool get_stage_info();
        bool battle_pause();
        bool battle_speedup();
        bool cancel_selection();  // 取消选择干员
        bool analyze_opers_preview();
        bool update_opers_info(const cv::Mat& image);

        bool do_action(const BattleAction& action);
        bool wait_condition(const BattleAction& action);

        bool oper_deploy(const BattleAction& action);
        bool oper_retreat(const BattleAction& action);
        bool use_skill(const BattleAction& action);

        bool try_possible_skill(const cv::Mat& image);
        void sleep_with_possible_skill(unsigned millisecond);

        std::string m_stage_name;

        std::unordered_map<Point, TilePack::TileInfo> m_side_tile_info;
        std::unordered_map<Point, TilePack::TileInfo> m_normal_tile_info;
        BattleActionsGroup m_actions_group;
        std::unordered_map<std::string, BattleDeployOper> m_group_to_oper_mapping;

        /* 实时更新的数据 */
        int m_kills = 0;
        std::unordered_map<std::string, BattleRealTimeOper> m_all_opers_info;
        std::unordered_map<std::string, BattleRealTimeOper> m_cur_opers_info;
        std::unordered_map<std::string, BattleDeployInfo> m_used_opers;
    };
}
