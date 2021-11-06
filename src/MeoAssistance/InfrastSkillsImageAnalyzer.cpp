﻿/*
    MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
    Copyright (C) 2021 MistEO and Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "InfrastSkillsImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "InfrastSmileyImageAnalyzer.h"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "Resource.h"

bool asst::InfrastSkillsImageAnalyzer::analyze() {
    LogTraceFunction;

    m_skills_detected.clear();
    m_skills_splited.clear();
    m_result.clear();

    skills_detect();

    skills_split();

    skill_analyze();

    for (auto&& info : m_result) {
        std::string skill_str;
        for (auto&& skill : info.skills_comb.skills) {
            skill_str += skill.names.front() + " ";
        }
        log.trace(info.hash, info.rect.to_string(), skill_str);
    }

    return true;
}

void asst::InfrastSkillsImageAnalyzer::sort_result() {
    LogTraceFunction;
    // 按位置排个序
    std::sort(m_result.begin(), m_result.end(),
              [](const auto& lhs, const auto& rhs) -> bool {
                  if (std::abs(lhs.rect.x - rhs.rect.x) < 5) { // x差距较小则理解为是同一排的，按y排序
                      return lhs.rect.y < rhs.rect.y;
                  }
                  else {
                      return lhs.rect.x < rhs.rect.x;
                  }
              });
}

bool asst::InfrastSkillsImageAnalyzer::skills_detect() {
    LogTraceFunction;
    const auto upper_task_ptr = resource.task().task_ptr("InfrastSkillsUpper");
    const auto lower_task_ptr = resource.task().task_ptr("InfrastSkillsLower");
    const auto hash_task_ptr = resource.task().task_ptr("InfrastSkillsHash");

    Rect hash_rect_move = hash_task_ptr->rect_move;

    std::vector<Rect> roi_vec = {
        upper_task_ptr->roi,
        lower_task_ptr->roi
    };

    InfrastSmileyImageAnalyzer smiley_analyzer(m_image);

    for (auto&& roi : roi_vec) {
        smiley_analyzer.set_roi(roi);
        if (!smiley_analyzer.analyze()) {
            return false;
        }

        const auto& smiley_res = smiley_analyzer.get_result();

        const auto task_ptr = resource.task().task_ptr("InfrastSkills");
        int skills_x_offset = task_ptr->rect_move.x;
        int skills_y_offset = task_ptr->rect_move.y;
        int skills_width = task_ptr->rect_move.width;
        int skills_height = task_ptr->rect_move.height;

        for (const auto& [_type, smiley_rect] : smiley_res) {
            Rect skills_rect;
            skills_rect.x = smiley_rect.x + skills_x_offset;
            skills_rect.y = smiley_rect.y + skills_y_offset;
            skills_rect.width = skills_width;
            skills_rect.height = skills_height;

            // 超过ROI边界了
            if (skills_rect.x + skills_rect.width > roi.x + roi.width || skills_rect.x < roi.x) {
                continue;
            }
#ifdef LOG_TRACE
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(skills_rect), cv::Scalar(0, 0, 255), 2);
#endif // LOG_TRACE

            Rect hash_rect = hash_rect_move;
            hash_rect.x += smiley_rect.x;
            hash_rect.y += smiley_rect.y;
            std::string hash = calc_hash(hash_rect);

            m_skills_detected.emplace(std::move(hash), std::move(skills_rect));
        }
    }

    if (!m_skills_detected.empty()) {
        return true;
    }
    return false;
}

bool asst::InfrastSkillsImageAnalyzer::skills_split() {
    LogTraceFunction;
    const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("InfrastSkills"));
    const auto thres = task_ptr->hist_threshold;

    for (const auto& [hash, roi] : m_skills_detected) {
        static int skill_width = roi.height;
        static int spacing = (roi.width - roi.height * MaxNumOfSkills) / (MaxNumOfSkills - 1);
        static cv::Mat mask;
        if (mask.empty()) {
            mask = cv::Mat(skill_width, skill_width, CV_8UC1, cv::Scalar(0));
            int radius = skill_width / 2;
            cv::circle(mask, cv::Point(radius, radius), radius, cv::Scalar(255, 255, 255), -1);
        }

        cv::Mat image_roi = m_image(utils::make_rect<cv::Rect>(roi));
        std::vector<Rect> skills_vec; // 单个干员的所有技能
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
        if (!skills_vec.empty()) {
            m_skills_splited.emplace(hash, skills_vec);
        }
    }

    return false;
}

bool asst::InfrastSkillsImageAnalyzer::skill_analyze() {
    LogTraceFunction;
    const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("InfrastSkills"));

    MatchImageAnalyzer skill_analyzer(m_image);
    skill_analyzer.set_mask_range(task_ptr->mask_range);
    skill_analyzer.set_threshold(task_ptr->templ_threshold);

    for (const auto& [hash, skills_rect_vec] : m_skills_splited) {
        std::unordered_set<InfrastSkill> skills_set; // 单个干员的全部技能
        std::string log_str = "[ ";
        for (const Rect& skill_rect : skills_rect_vec) {
            skill_analyzer.set_roi(skill_rect);

            std::vector<std::pair<InfrastSkill, MatchRect>> possible_skills;
            // 逐个该设施内所有可能的技能，取得分最高的
            for (const auto& [id, skill] : resource.infrast().get_skills(m_facility)) {
                skill_analyzer.set_templ_name(skill.templ_name);

                if (!skill_analyzer.analyze()) {
                    continue;
                }
                possible_skills.emplace_back(std::make_pair(skill, skill_analyzer.get_result()));
            }
            if (possible_skills.empty()) {
                log.error("skill has no recognition result");
                continue;
            }
            // 可能的结果多于1个，只可能是同一个技能不同等级的结果
            // 例如：标准化a、标准化b，这两个模板非常像，然后分数都超过了阈值
            // 如果原图是标准化a，是不可能匹配上标准化b的模板的，因为b的模板左半边多了半个环
            // 相反如果原图是标准化b，却有可能匹配上标准化a的模板，因为a的模板右半边的环，b的原图中也有
            // 所以如果结果是同类型的，只需要取里面等级最高的那个即可
            InfrastSkill most_confident_skills;
            if (possible_skills.size() == 1) {
                most_confident_skills = possible_skills.front().first;
            }
            else if (possible_skills.size() > 1) {
                // 匹配得分最高的id作为基准，排除有识别错误，其他的技能混进来了的情况
                // 即排除容器中，除了有同一个技能的不同等级，还有别的技能的情况
                auto max_iter = std::max_element(possible_skills.begin(), possible_skills.end(),
                                                 [](const auto& lhs, const auto& rhs) -> bool {
                                                     return lhs.second.score < rhs.second.score;
                                                 });
                std::string base_id = max_iter->first.id;
                size_t level_pos = 0;
                // 倒着找，第一个不是数字的。前面就是技能基础id名字，后面的数字就是技能等级
                for (size_t i = base_id.size() - 1; i != 0; --i) {
                    if (!std::isdigit(base_id.at(i))) {
                        level_pos = i + 1;
                        break;
                    }
                }
                base_id = base_id.substr(0, level_pos);
                std::string max_level;
                for (const auto& [skill, _] : possible_skills) {
                    if (size_t find_pos = skill.id.find(base_id);
                        find_pos != std::string::npos) {
                        std::string cur_skill_level = skill.id.substr(base_id.size());
                        if (max_level.empty() || cur_skill_level > max_level) {
                            max_level = cur_skill_level;
                            most_confident_skills = skill;
                        }
                    } // 这里对应的else就是上述的其他技能混进来了的情况
                }
            }
            log.trace(most_confident_skills.id);
            std::string skill_id = most_confident_skills.id;
            log_str += skill_id + " - " + most_confident_skills.names.front() + "; ";
#ifdef LOG_TRACE
            cv::Mat skill_mat = m_image(utils::make_rect<cv::Rect>(skill_rect));
#endif
            skills_set.emplace(std::move(most_confident_skills));
        }
        log.trace(log_str, "]");

        InfrastOperSkillInfo info;
        info.hash = hash;
        info.skills_comb = InfrastSkillsComb(std::move(skills_set));
        info.rect = m_skills_detected.at(hash);

        // 识别该干员是否被选中。数据结构设计的不太合理，先这么凑合用了
        int smiley_x = skills_rect_vec.front().x - task_ptr->rect_move.x;
        int smiley_y = skills_rect_vec.front().y - task_ptr->rect_move.y;
        info.selected = selected_analyze(smiley_x, smiley_y);

        m_result.emplace_back(std::move(info));
    }
    if (!m_result.empty()) {
        return true;
    }
    return false;
}

bool asst::InfrastSkillsImageAnalyzer::selected_analyze(int smiley_x, int smiley_y) {
    LogTraceFunction;
    const auto selected_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("InfrastOperSelected"));
    cv::Rect selected_rect = utils::make_rect<cv::Rect>(selected_task_ptr->rect_move);
    selected_rect.x += smiley_x;
    selected_rect.y += smiley_y;

#ifdef LOG_TRACE
    cv::Mat draw = m_image_draw.clone();
    cv::rectangle(draw, selected_rect, cv::Scalar(0, 0, 255), 2);
#endif // LOG_TRACE
    cv::Mat roi = m_image(selected_rect);
    cv::Mat hsv, bin;
    cv::cvtColor(roi, hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);
    int mask_lowb = selected_task_ptr->mask_range.first;
    int mask_uppb = selected_task_ptr->mask_range.second;

    int count = 0;
    auto& h_channel = channels.at(0);
    for (int i = 0; i != h_channel.rows; ++i) {
        for (int j = 0; j != h_channel.cols; ++j) {
            cv::uint8_t value = h_channel.at<cv::uint8_t>(i, j);
            if (mask_lowb < value && value < mask_uppb) {
                ++count;
            }
        }
    }
    log.trace("selected_analyze |", count);
    return count >= selected_task_ptr->templ_threshold;
}