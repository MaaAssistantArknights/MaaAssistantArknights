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
class VisionHelper;

class CombatRecordRecognitionTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~CombatRecordRecognitionTask() override = default;

    bool set_video_path(const std::filesystem::path& path);

protected:
    virtual bool _run() override;

private:
    struct BattlefieldOper
    {
        bool new_here = false;
        battle::DeployDirection direction = battle::DeployDirection::None;
        std::string name;
    };

    struct ClipInfo
    {
        size_t start_frame_index = 0;
        size_t end_frame_index = 0;
        cv::Mat start_frame;
        cv::Mat end_frame;
        std::vector<battle::DeploymentOper> deployment;
        bool deployment_changed = true;
        std::unordered_map<Point, BattlefieldOper> battlefield;
        std::vector<cv::Mat> random_frames;
        std::string ends_oper_name;
    };

    bool analyze_formation();
    bool analyze_stage();
    bool analyze_deployment();
    bool slice_video();
    bool compare_skill(ClipInfo& clip, ClipInfo& pre_clip);

    bool analyze_clip(ClipInfo& clip, ClipInfo* pre_clip_ptr);
    bool detect_operators(ClipInfo& clip, ClipInfo* pre_clip_ptr);
    bool classify_direction(ClipInfo& clip, ClipInfo* pre_clip_ptr);
    bool process_changes(ClipInfo& clip, ClipInfo* pre_clip_ptr);
    void ananlyze_deployment_names(ClipInfo& clip);

    json::object analyze_action_condition(ClipInfo& clip, ClipInfo* pre_clip_ptr);
    size_t skip_frames(size_t count);

    static std::string analyze_detail_page_oper_name(const cv::Mat& frame);

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
    std::vector<std::pair<size_t /*frame*/, int /*kills*/>> m_frame_kills;

    std::unordered_map<Point, TilePack::TileInfo> m_normal_tile_info;
    std::unordered_map<std::string, cv::Mat> m_formation;
    std::unordered_map<std::string, cv::Mat> m_all_avatars;

    std::unordered_map<std::string, Point> m_operator_locations;
    std::unordered_map<Point, std::string> m_location_operators;

    json::value m_copilot_json;
    int m_pre_action_costs = -1;

    void show_img(const VisionHelper& analyzer);
    void show_img(const cv::Mat& img);

#ifdef ASST_DEBUG
    inline static const std::string DrawWindow = "CombatRecord";
#endif
};
} // namespace asst
