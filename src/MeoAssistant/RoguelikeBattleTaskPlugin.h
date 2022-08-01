#pragma once

#include <queue>

#include "AbstractTaskPlugin.h"
#include "AsstTypes.h"
#include "TilePack.h"
#include "BattleImageAnalyzer.h"
#include "AsstBattleDef.h"

namespace asst
{
    class RoguelikeBattleTaskPlugin : public AbstractTaskPlugin
    {
        using Loc = asst::TilePack::BuildableType;
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeBattleTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

        void set_stage_name(std::string stage);

    protected:
        virtual bool _run() override;

        void wait_for_start();
        bool get_stage_info();
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

        struct DeployInfo
        {
            Point placed;
            Point direction;
        };

        DeployInfo calc_best_plan(const BattleRealTimeOper& oper);

        // 计算摆放干员的朝向
        // 返回滑动的方向、得分
        std::pair<Point, int> calc_best_direction_and_score(Point loc, const BattleRealTimeOper& oper);

        bool m_opers_used = false;
        int m_pre_hp = 0;
        int m_kills = 0;
        int m_total_kills = 0;

        std::unordered_map<Point, TilePack::TileInfo> m_side_tile_info;
        std::unordered_map<Point, TilePack::TileInfo> m_normal_tile_info;
        std::vector<Point> m_homes;
        std::queue<int> m_key_kills;
        size_t m_cur_home_index = 0;
        std::unordered_map<Point, std::string> m_used_tiles;
        std::unordered_map<std::string, int64_t> m_restore_status;

        std::string m_stage_name;
    };
}
