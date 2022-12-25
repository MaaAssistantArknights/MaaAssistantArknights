#pragma once

#include "AbstractTask.h"

#include "Common/AsstBattleDef.h"
#include "Config/Miscellaneous/TilePack.h"
#include "InstHelper.h"
#include "Utils/NoWarningCVMat.h"
#include "Utils/Platform.hpp"
#include "Utils/WorkingDir.hpp"

#include <map>

namespace asst
{
    class BattleHelper
    {
        inline static const auto AvatarCacheDir = UserDir.get() / utils::path("cache") / utils::path("avatars");
        inline static const std::string CacheExtension = ".png";

    public:
        ~BattleHelper() = default;

    protected:
        BattleHelper(Assistant* inst);

        virtual bool set_stage_name(const std::string& name);
        virtual void clear();

        bool calc_tiles_info(const std::string& stage_name);
        bool load_avatar_cache(const std::string& name, bool with_token = false);
        void save_avatar_cache(const std::string& name, const cv::Mat& avatar);

        bool pause();
        bool speed_up();

        bool update_deployment(bool init = false);

        bool deploy_oper(const std::string& name, const Point& loc, battle::DeployDirection direction);
        bool retreat_oper(const std::string& name);
        bool retreat_oper(const Point& loc, bool manually = true);
        bool use_skill(const std::string& name);
        bool use_skill(const Point& loc);
        bool check_pause_button();
        bool wait_for_start();
        bool wait_for_end();
        bool use_all_ready_skill();
        bool check_and_use_skill(const std::string& name);
        bool check_and_use_skill(const Point& loc);
        void save_map(const cv::Mat& image);

        bool click_oper_on_deployment(const std::string& name);
        bool click_oper_on_deployment(const Rect& rect);
        bool click_oper_on_battlefiled(const std::string& name);
        bool click_oper_on_battlefiled(const Point& loc);
        bool click_retreat(); // 这个是不带识别的，直接点
        bool click_skill();   // 这个是带识别的，转好了才点
        bool cancel_oper_selection();

        bool is_name_invaild(const std::string& name);

        std::optional<Rect> get_oper_rect_on_deployment(const std::string& name) const;

        std::string m_stage_name;
        std::unordered_map<Point, TilePack::TileInfo> m_side_tile_info;
        std::unordered_map<Point, TilePack::TileInfo> m_normal_tile_info;
        std::unordered_map<std::string, battle::SkillUsage> m_skill_usage;

        /* 实时更新的数据 */
        int m_kills = 0;
        int m_total_kills = 0;

        std::map<std::string, cv::Mat> m_all_deployment_avatars;
        std::map<std::string, battle::DeploymentOper> m_cur_deployment_opers;

        std::map<std::string, Point> m_battlefield_opers;
        std::map<Point, std::string> m_used_tiles;

    private:
        virtual AbstractTask& this_task() = 0;

        InstHelper m_inst_helper;
    };
} // namespace asst
