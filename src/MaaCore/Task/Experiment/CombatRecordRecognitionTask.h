#pragma once

#include "Task/AbstractTask.h"

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/TilePack.h"

#include <meojson/json.hpp>

namespace cv
{
    class VideoCapture;
}

namespace asst
{
    class AbstractImageAnalyzer;

    class CombatRecordRecognitionTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~CombatRecordRecognitionTask() override = default;

        bool set_video_path(const std::filesystem::path& path);
        bool set_stage_name(const std::string& stage_name);

    protected:
        virtual bool _run() override;

    private:
        struct BattlefiledOper
        {
            bool new_here = false;
            battle::DeployDirection direction = battle::DeployDirection::None;
            std::string name;
        };

        struct ClipInfo
        {
            size_t start_frame = 0;
            size_t end_frame = 0;
            std::vector<battle::DeploymentOper> deployment;
            size_t cooling = 0;
            std::unordered_map<Point, BattlefiledOper> battlefield;
        };

        bool analyze_formation();
        bool analyze_stage();
        bool analyze_deployment();
        bool slice_video();
        bool analyze_all_clips();
        bool analyze_clip(ClipInfo& clip, ClipInfo* pre_clip_ptr = nullptr);
        bool detect_operators(ClipInfo& clip, ClipInfo* pre_clip_ptr = nullptr);
        bool classify_direction(ClipInfo& clip, ClipInfo* pre_clip_ptr = nullptr);
        bool process_changes(ClipInfo& clip, ClipInfo* pre_clip_ptr = nullptr);
        void ananlyze_deployment_names(ClipInfo& clip);
        size_t skip_frames(size_t count);

        std::filesystem::path m_video_path;
        std::shared_ptr<cv::VideoCapture> m_video_ptr = nullptr;
        std::string m_stage_name;
        double m_video_fps = 0;
        size_t m_video_frame_count = 0;
        double m_scale = 0;

        int m_formation_fps = 2;
        int m_stage_ocr_fps = 2;
        int m_deployment_fps = 5;

        size_t m_formation_end_frame = 0;
        size_t m_stage_ocr_end_frame = 0;
        size_t m_battle_start_frame = 0;
        size_t m_battle_end_frame = 0;

        std::vector<ClipInfo> m_clips;

        std::unordered_map<Point, TilePack::TileInfo> m_normal_tile_info;
        std::unordered_map<std::string, cv::Mat> m_formation;
        std::unordered_map<std::string, cv::Mat> m_all_avatars;

        json::value m_copilot_json;

        void show_img(const AbstractImageAnalyzer& analyzer);
        void show_img(const cv::Mat& img);

#ifdef ASST_DEBUG
        inline static const std::string DrawWindow = "CombatRecord";
#endif
    };
} // namespace asst
