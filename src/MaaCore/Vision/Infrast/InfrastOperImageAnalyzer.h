#pragma once
#include "Vision/VisionHelper.h"

#include "Common/AsstInfrastDef.h"

namespace asst
{
class InfrastOperImageAnalyzer : public VisionHelper
{
public:
    enum ToBeCalced
    {
        None = 0,
        Smiley = 1,
        Mood = 2,
        FaceHash = 4,
        // NameHash = 8,
        Selected = 16,
        Doing = 32,
        Skill = 64,
        All = 127
    };

    using VisionHelper::VisionHelper;
    virtual ~InfrastOperImageAnalyzer() override = default;
    InfrastOperImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;

    bool analyze();

    void sort_by_loc();
    void sort_by_mood();

    auto get_result() const noexcept -> const std::vector<infrast::Oper>& { return m_result; }

    int get_num_of_opers_with_skills() const noexcept { return m_num_of_opers_with_skills; }

    void set_facility(std::string facility) noexcept { m_facility = std::move(facility); }

    void set_to_be_calced(int to_be_calced) noexcept { m_to_be_calced = to_be_calced | Smiley; }

    static constexpr int MaxNumOfSkills = 2; // 单个干员最多有几个基建技能

private:
    // 该分析器不支持外部设置ROI
    using VisionHelper::set_roi;

    void oper_detect();
    void mood_analyze();
    void face_hash_analyze();
    void skill_analyze();
    void selected_analyze();
    void doing_analyze();

    std::string m_facility;
    std::vector<infrast::Oper> m_result;
    int m_to_be_calced = Smiley;
    int m_num_of_opers_with_skills = 0;
};
} // namespace asst
