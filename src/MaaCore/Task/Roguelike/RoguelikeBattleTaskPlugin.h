#pragma once

#include <queue>
#include <stack>

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/TilePack.h"
#include "Task/AbstractTaskPlugin.h"
#include "Task/BattleHelper.h"
#include "Vision/Miscellaneous/BattleImageAnalyzer.h"

namespace asst
{
    class RoguelikeBattleTaskPlugin : public AbstractTaskPlugin, private BattleHelper
    {
        using Time_Point = std::chrono::time_point<std::chrono::system_clock>;

        inline static const std::unordered_set<std::string> DiceSet = { "骰子", "8面骰子", "12面骰子" };

    public:
        RoguelikeBattleTaskPlugin(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
        virtual ~RoguelikeBattleTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
        virtual AbstractTask& this_task() override { return *this; }
        virtual void clear() override;

        bool do_once();
        bool calc_stage_info();
        void load_cache();

        void all_melee_retreat();

        std::optional<size_t> check_urgent(const std::unordered_set<std::string>& pre_cooling,
                                           const std::unordered_set<std::string>& cur_cooling,
                                           const std::map<std::string, Point>& pre_battlefield);

        std::optional<battle::DeploymentOper> calc_best_oper() const;

        struct DeployInfo
        {
            Point placed;
            battle::DeployDirection direction;
        };
        std::optional<DeployInfo> calc_best_loc(const battle::DeploymentOper& oper) const;
        battle::AttackRange get_attack_range(const battle::DeploymentOper& oper,
                                             battle::DeployDirection direction = battle::DeployDirection::Right) const;

        struct DirectionAndScore
        {
            battle::DeployDirection direction;
            int score = 0;
        };
        DirectionAndScore calc_best_direction_and_score(Point loc, const battle::DeploymentOper& oper,
                                                        battle::DeployDirection recommended_direction) const;

        void postproc_of_deployment_conditions(const battle::DeploymentOper& oper, const Point& placed_loc,
                                               battle::DeployDirection direction);

        void check_drone_tiles();
        void wait_until_start_button_clicked();

        std::string oper_name_in_config(const battle::DeploymentOper& oper) const;
        battle::LocationType get_oper_location_type(const battle::DeploymentOper& oper) const;
        std::vector<Point> available_locations(const battle::DeploymentOper& oper) const;
        std::vector<Point> available_locations(battle::LocationType type) const;
        bool get_position_full(const battle::DeploymentOper& oper) const;
        bool get_position_full(battle::LocationType loc_type) const;
        void set_position_full(const battle::DeploymentOper& oper, bool full);
        void set_position_full(battle::LocationType loc_type, bool full);
        void set_position_full(const Point& loc, bool full);
        battle::OperPosition get_role_position(const battle::Role& role) const;

        /* from config */

        std::vector<battle::roguelike::ReplacementHome> m_homes;
        bool m_allow_to_use_dice = true;
        std::unordered_set<Point> m_blacklist_location;
        std::array<battle::Role, 9> m_role_order {};
        std::unordered_map<Point, battle::roguelike::ForceDeployDirection> m_force_deploy_direction;

        struct AirDefenseData
        {
            /* from config */
            int stop_blocking_deploy_num = INT_MAX;
            int deploy_air_defense_num = 0;
            bool ban_medic = false;
            /* real time */
            int has_deployed_blocking_num = 0;
            int has_deployed_air_defense_num = 0;
            bool has_finished_deploy_air_defense = false;
        };
        AirDefenseData m_force_air_defense;

        /* real time */

        size_t m_cur_home_index = 0;
        bool m_first_deploy = true;
        bool m_melee_full = false;
        bool m_ranged_full = false;

        struct HomeInfo
        {
            bool wait_blocking = true;
            bool wait_medic = true;
            Point blocking_pos;
        };
        std::vector<HomeInfo> m_homes_status;
        std::unordered_map<Point, size_t> m_blocking_for_home_index;
        std::unordered_map<Point, std::vector<size_t>> m_medic_for_home_index;
        std::deque<size_t> m_urgent_home_index;

        struct DroneTile
        {
            Time_Point placed_time;
            Point placed_loc;
            DroneTile(Time_Point placed_time, Point placed_loc)
            {
                this->placed_time = placed_time;
                this->placed_loc = placed_loc;
            }
            bool operator<(const DroneTile& x) const { return x.placed_time < placed_time; }
        };
        std::priority_queue<DroneTile> m_need_clear_tiles;
    };
} // namespace asst
