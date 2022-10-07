#pragma once

#include <queue>
#include <stack>

#include "AbstractTaskPlugin.h"
#include "ImageAnalyzer/BattleImageAnalyzer.h"
#include "Resource/TilePack.h"
#include "Utils/AsstBattleDef.h"
#include "Utils/AsstTypes.h"

namespace asst
{
    class RoguelikeBattleTaskPlugin : public AbstractTaskPlugin
    {
        using Loc = asst::TilePack::BuildableType;
        using Time_Point = std::chrono::time_point<std::chrono::system_clock>;

    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeBattleTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

        void set_stage_name(std::string stage);

    protected:
        virtual bool _run() override;

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

        std::vector<Point> available_locations(BattleRole role);

        struct DeployInfo
        {
            Point placed;
            Point direction;
        };
        BattleAttackRange get_attack_range(const BattleRealTimeOper& oper);
        DeployInfo calc_best_plan(const BattleRealTimeOper& oper);

        // 计算摆放干员的朝向
        // 返回滑动的方向、得分
        std::pair<Point, int> calc_best_direction_and_score(Point loc, const BattleRealTimeOper& oper,
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
        int m_last_not_urgent = -1;
        int m_pre_hp = 0;
        int m_kills = 0;
        int m_total_kills = 0;
        size_t m_cur_home_index = 0;
        cv::Mat m_dice_image;

        std::array<BattleRole, 9> m_role_order;
        std::unordered_map<Point, TilePack::TileInfo> m_side_tile_info;
        std::unordered_map<Point, TilePack::TileInfo> m_normal_tile_info;
        std::vector<ReplacementHome> m_homes;
        std::vector<bool> m_wait_melee;
        std::vector<bool> m_wait_medic;
        std::vector<bool> m_indeed_no_medic;
        std::unordered_map<Point, size_t> m_melee_for_home_index;
        std::unordered_map<Point, std::vector<size_t>> m_medic_for_home_index;
        std::stack<size_t> m_next_urgent_index;
        std::vector<int> m_index_count;
        std::unordered_set<Point> m_blacklist_location;
        std::unordered_set<std::string> m_retreated_opers;
        std::queue<int> m_key_kills;
        std::unordered_map<Point, std::pair<std::string, BattleRole>> m_used_tiles;
        std::unordered_map<std::string, Point> m_opers_in_field;
        std::unordered_map<std::string, int64_t> m_restore_status;
        std::priority_queue<DroneTile> m_need_clear_tiles;

        std::string m_stage_name;
    };
} // namespace asst
