#pragma once
#include "Task/AbstractTask.h"

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/TilePack.h"
#include "Vision/Miscellaneous/BattleImageAnalyzer.h"

namespace asst
{
    class BattleProcessTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~BattleProcessTask() override = default;

        bool set_stage_name(std::string name);

    protected:
        virtual bool _run() override;

        virtual bool get_stage_info();
        bool battle_pause();
        bool battle_speedup();
        bool analyze_opers_preview();
        bool update_opers_info(const cv::Mat& image);

        bool do_action(size_t action_index);
        bool wait_condition(const BattleAction& action);

        bool oper_deploy(const BattleAction& action, bool only_pre_process = false);
        bool oper_retreat(const BattleAction& action, bool only_pre_process = false);
        bool use_skill(const BattleAction& action, bool only_pre_process = false);
        bool wait_to_end(const BattleAction& action);

        bool try_possible_skill(const cv::Mat& image);
        void sleep_with_possible_skill(unsigned millisecond);

        template <typename GroupNameType = std::string, typename CharNameType = std::string>
        static std::optional<std::unordered_map<GroupNameType, CharNameType>> get_char_allocation_for_each_group(
            const std::unordered_map<GroupNameType, std::vector<CharNameType>>& group_list,
            const std::vector<CharNameType>& char_list);

        std::string m_stage_name;

        std::unordered_map<Point, TilePack::TileInfo> m_side_tile_info;
        std::unordered_map<Point, TilePack::TileInfo> m_normal_tile_info;
        BattleCopilotData m_copilot_data;
        std::unordered_map<std::string, BattleDeployOper> m_group_to_oper_mapping;

        /* 实时更新的数据 */
        int m_kills = 0;
        int m_total_kills = 0;
        std::unordered_map<std::string, BattleRealTimeOper> m_all_opers_info;
        std::unordered_map<std::string, BattleRealTimeOper> m_cur_opers_info;
        std::unordered_map<std::string, BattleDeployInfo> m_used_opers;

        bool m_in_bullet_time = false;
    };
}
