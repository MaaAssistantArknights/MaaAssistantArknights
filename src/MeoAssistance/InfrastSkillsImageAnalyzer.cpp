#include "InfrastSkillsImageAnalyzer.h"

#include "Resource.h"
#include "InfrastSmileyImageAnalyzer.h"
#include "AsstUtils.hpp"

bool asst::InfrastSkillsImageAnalyzer::analyze()
{
    m_skills_detected.clear();
    m_skills_splited.clear();

    auto raw_roi = m_roi;
    const auto upper_task_ptr = resource.task().task_ptr("InfrastSkillsUpper");
    m_roi = upper_task_ptr->roi;
    bool upper_ret = skills_detect();

    const auto lower_task_ptr = resource.task().task_ptr("InfrastSkillsLower");
    m_roi = lower_task_ptr->roi;
    bool lower_ret = skills_detect();

    m_roi = raw_roi;

    skills_split();

    return true;
}

bool asst::InfrastSkillsImageAnalyzer::skills_detect()
{
    InfrastSmileyImageAnalyzer smiley_analyzer(m_image, m_roi);

    if (!smiley_analyzer.analyze()) {
        return false;
    }

    const auto& smiley_res = smiley_analyzer.get_result();

    const auto task_ptr = resource.task().task_ptr("InfrastSkills");
    int skills_x_offset = task_ptr->result_move.x;
    int skills_y_offset = task_ptr->result_move.y;
    int skills_width = task_ptr->result_move.width;
    int skills_height = task_ptr->result_move.height;

    for (const auto& [_type, smiley_rect] : smiley_res) {
        Rect skills_rect;
        skills_rect.x = smiley_rect.x + skills_x_offset;
        skills_rect.y = smiley_rect.y + skills_y_offset;
        skills_rect.width = skills_width;
        skills_rect.height = skills_height;

        // 超过ROI边界了
        if (skills_rect.x + skills_rect.width > m_roi.x + m_roi.width
            || skills_rect.x < m_roi.x) {
            continue;
        }
#ifdef LOG_TRACE
        //cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(skills_rect), cv::Scalar(0, 0, 255), 2);
#endif // LOG_TRACE
        m_skills_detected.emplace_back(std::move(skills_rect));
    }

    if (!m_skills_detected.empty()) {
        return true;
    }
    return false;
}

bool asst::InfrastSkillsImageAnalyzer::skills_split()
{
    const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("InfrastSkills"));
    const auto thres = task_ptr->hist_threshold;

    for (const Rect& roi : m_skills_detected) {
        static int skill_width = roi.height;
        static int spacing = (roi.width - roi.height * MaxNumOfSkills) / (MaxNumOfSkills - 1);
        static cv::Mat mask;
        if (mask.empty()) {
            mask = cv::Mat(skill_width, skill_width, CV_8UC1, cv::Scalar(0));
            int radius = skill_width / 2;
            cv::circle(mask, cv::Point(radius, radius), radius, cv::Scalar(255, 255, 255), -1);
        }

        cv::Mat image_roi = m_image(utils::make_rect<cv::Rect>(roi));
        std::vector<Rect> skills_vec;   // 单个干员的所有技能
        for (int i = 0; i != MaxNumOfSkills; ++i) {
            int x = i * skill_width + spacing * i;
            Rect skill_rect(x, 0, skill_width, roi.height);
            cv::Mat skill_image = image_roi(utils::make_rect<cv::Rect>(skill_rect));

            cv::Mat skill_gray;
            cv::cvtColor(skill_image, skill_gray, cv::COLOR_BGR2GRAY);
            cv::Scalar avg = cv::mean(skill_gray, mask);
            if (avg[0] < thres) {
                continue;
            }
            Rect skill_rect_in_org = skill_rect;
            skill_rect_in_org.x += roi.x;
            skill_rect_in_org.y += roi.y;

#ifdef LOG_TRACE
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(skill_rect_in_org), cv::Scalar(0, 255, 0), 2);
#endif
            skills_vec.emplace_back(skill_rect_in_org);
        }
        m_skills_splited.emplace_back(skills_vec);
    }

    return false;
}

bool asst::InfrastSkillsImageAnalyzer::skill_analyze()
{
    return false;
}