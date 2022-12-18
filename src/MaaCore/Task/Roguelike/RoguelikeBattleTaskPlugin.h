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
    class RoguelikeBattleTaskPlugin : public AbstractTaskPlugin, public BattleHelper
    {
        using Time_Point = std::chrono::time_point<std::chrono::system_clock>;

        inline static const std::string Dice = "骰子";
        inline static const std::string UnknownName = "Unknown";

    public:
        RoguelikeBattleTaskPlugin(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
        virtual ~RoguelikeBattleTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

        void set_stage_name(std::string stage);

    protected:
        virtual bool _run() override;

        // 有些特殊的角色，他的职业并不一定和正常的位置相对应，比如“掠风”是地面辅助
        // get_role_position 可以仅知道干员职业的情况下，大概猜测一下位置
        // get_oper_position 可以在已知干员名的时候获得准确的位置
        battle::LocationType get_role_location_type(const battle::Role& role);
        battle::LocationType get_oper_location_type(const std::string& name);

        std::vector<Point> available_locations(battle::Role role);
        std::vector<Point> available_locations(const std::string& name);
        std::vector<Point> available_locations(battle::LocationType type);

        battle::OperPosition get_role_position(const battle::Role& role);

        void wait_for_start();
        bool get_stage_info();
        bool battle_pause();
        bool auto_battle();
        void all_melee_retreat();
        bool speed_up();
        bool use_skill(const Rect& rect);
        bool retreat(const Point& point);
        bool abandon();
        void clear();
        bool try_possible_skill(const cv::Mat& image);
        bool check_key_kills(const cv::Mat& image);
        bool wait_start();
        bool cancel_oper_selection();
        bool is_oper_name_error(const std::string& name);

        void set_position_full(const battle::LocationType& loc_type, bool full);
        void set_position_full(const Point& point, bool full);
        void set_position_full(const battle::Role& role, bool full);
        void set_position_full(const std::string& name, bool full);
        bool get_position_full(const battle::Role& role);

        struct DeployInfo
        {
            Point placed;
            Point direction;
        };
        battle::AttackRange get_attack_range(const battle::DeploymentOper& oper);
        DeployInfo calc_best_plan(const battle::DeploymentOper& oper);

        // 计算摆放干员的朝向
        // 返回滑动的方向、得分
        std::pair<Point, int> calc_best_direction_and_score(Point loc, const battle::DeploymentOper& oper,
                                                            Point recommended_direction);

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

        bool m_opers_used = false;
        bool m_is_cur_urgent = false;
        bool m_stage_use_dice = true;
        bool m_use_dice = false;
        bool m_first_deploy = true;
        bool m_melee_full = false;
        bool m_ranged_full = false;
        int m_last_not_urgent = -1;
        int m_pre_hp = 0;
        int m_kills = 0;
        int m_total_kills = 0;
        struct
        {
            int stop_blocking_deploy_num = INT_MAX;
            int has_deployed_blocking_num = 0;
            int deploy_air_defense_num = 0;
            int has_deployed_air_defense_num = 0;
            bool has_finished_deploy_air_defense = false;
            bool ban_medic = false;

            void clear() noexcept
            {
                stop_blocking_deploy_num = INT_MAX;
                deploy_air_defense_num = 0;
                has_deployed_blocking_num = 0;
                has_deployed_air_defense_num = 0;
                has_finished_deploy_air_defense = false;
                ban_medic = false;
            }
        } m_force_air_defense;

        int m_last_cooling_count = 0;
        size_t m_cur_home_index = 0;
        cv::Mat m_dice_image;

        std::array<battle::Role, 9> m_role_order;
        std::unordered_map<Point, TilePack::TileInfo> m_side_tile_info;
        std::unordered_map<Point, TilePack::TileInfo> m_normal_tile_info;
        std::vector<battle::roguelike::ReplacementHome> m_homes;
        std::vector<bool> m_wait_blocking;
        std::vector<bool> m_wait_medic;
        std::vector<bool> m_indeed_no_medic;
        std::unordered_map<Point, size_t> m_blocking_for_home_index;
        std::unordered_map<Point, std::vector<size_t>> m_medic_for_home_index;
        std::stack<size_t> m_next_urgent_index;
        std::unordered_set<Point> m_blacklist_location;
        std::set<std::string> m_retreated_opers;
        std::queue<int> m_key_kills;
        std::unordered_map<Point, std::string> m_used_tiles;
        std::unordered_map<std::string, Point> m_opers_in_field;
        std::unordered_map<std::string, int64_t> m_restore_status;
        std::priority_queue<DroneTile> m_need_clear_tiles;
        std::unordered_map<Point, battle::roguelike::ForceDeployDirection> m_force_deploy_direction;

        std::string m_stage_name;
    };
} // namespace asst
