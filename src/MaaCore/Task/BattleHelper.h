#pragma once

#include "AbstractTask.h"

#include "Common/AsstBattleDef.h"
#include "Config/Miscellaneous/TilePack.h"
#include "InstHelper.h"
#include "Utils/NoWarningCVMat.h"
#include "Utils/Platform.hpp"
#include "Utils/WorkingDir.hpp"

#include <filesystem>
#include <map>

namespace asst
{
    class BattleHelper
    {
        inline static const std::string CacheExtension = ".png";

    public:
        ~BattleHelper() = default;

    protected:
        BattleHelper(Assistant* inst);

        virtual AbstractTask& this_task() = 0;

        virtual bool set_stage_name(const std::string& name);
        virtual void clear();
        virtual const std::string oper_name_ocr_task_name() const noexcept { return "BattleOperName"; }
        virtual bool do_strategic_action(const cv::Mat& reusable = cv::Mat());

        bool calc_tiles_info(const std::string& stage_name);
        bool load_avatar_cache(const std::string& name, bool with_token = false);
        void save_avatar_cache(const std::string& name, const cv::Mat& avatar);

        bool pause();
        bool speed_up();
        bool abandon();

        bool update_deployment(bool init = false, const cv::Mat& reusable = cv::Mat());
        bool update_kills(const cv::Mat& reusable = cv::Mat());
        bool update_cost(const cv::Mat& reusable = cv::Mat());

        bool deploy_oper(const std::string& name, const Point& loc, battle::DeployDirection direction);
        bool retreat_oper(const std::string& name);
        bool retreat_oper(const Point& loc, bool manually = true);
        bool use_skill(const std::string& name, bool keep_waiting = true);
        bool use_skill(const Point& loc, bool keep_waiting = true);
        bool check_pause_button(const cv::Mat& reusable = cv::Mat());
        virtual bool wait_until_start();
        bool wait_until_end();
        bool use_all_ready_skill(const cv::Mat& reusable = cv::Mat());
        bool check_and_use_skill(const std::string& name, const cv::Mat& reusable = cv::Mat());
        bool check_and_use_skill(const Point& loc, const cv::Mat& reusable = cv::Mat());
        void save_map(const cv::Mat& image);

        bool click_oper_on_deployment(const std::string& name);
        bool click_oper_on_deployment(const Rect& rect);
        bool click_oper_on_battlefield(const std::string& name);
        bool click_oper_on_battlefield(const Point& loc);
        bool click_retreat();                       // 这个是不带识别的，直接点
        bool click_skill(bool keep_waiting = true); // 这个是带识别的，转好了才点
        bool cancel_oper_selection();
        bool move_camera(const std::pair<double, double>& move_loc, bool clear_kills = false);

        bool is_name_invalid(const std::string& name);
        std::optional<Rect> get_oper_rect_on_deployment(const std::string& name) const;

        std::string m_stage_name;
        std::unordered_map<Point, TilePack::TileInfo> m_side_tile_info;
        std::unordered_map<Point, TilePack::TileInfo> m_normal_tile_info;
        std::unordered_map<std::string, battle::SkillUsage> m_skill_usage;

        /* 实时更新的数据 */
        bool m_in_battle = false;
        int m_kills = 0;
        int m_total_kills = 0;
        int m_cost = 0;

        std::map<std::string, cv::Mat> m_all_deployment_avatars;
        std::map<std::string, battle::DeploymentOper> m_cur_deployment_opers;

        std::map<std::string, Point> m_battlefield_opers;
        std::map<Point, std::string> m_used_tiles;

    private:
        static const std::filesystem::path& avatar_cache_dir();

        InstHelper m_inst_helper;
    };
} // namespace asst
