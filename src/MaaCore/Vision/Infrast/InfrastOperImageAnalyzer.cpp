#include "InfrastOperImageAnalyzer.h"

#include "Utils/NoWarningCV.h"
#include "Utils/Ranges.hpp"

#include "Config/Miscellaneous/InfrastConfig.h"
#include "Config/TaskData.h"
#include "InfrastSmileyImageAnalyzer.h"
#include "Utils/Logger.hpp"
#include "Vision/Hasher.h"
#include "Vision/Matcher.h"

bool asst::InfrastOperImageAnalyzer::analyze()
{
    m_result.clear();
    m_num_of_opers_with_skills = 0;

    if (m_to_be_calced & None) {
        return true;
    }

    if (m_to_be_calced & Smiley) {
        oper_detect();
    }
    if (m_to_be_calced & Mood) {
        mood_analyze();
    }
    if (m_to_be_calced & FaceHash) {
        face_hash_analyze();
    }
    if (m_to_be_calced & Skill) {
        skill_analyze();
    }
    if (m_to_be_calced & Selected) {
        selected_analyze();
    }
    if (m_to_be_calced & Doing) {
        doing_analyze();
    }

    return !m_result.empty();
}

void asst::InfrastOperImageAnalyzer::sort_by_loc()
{
    LogTraceFunction;

    ranges::sort(m_result, [](const infrast::Oper& lhs, const infrast::Oper& rhs) -> bool {
        if (std::abs(lhs.rect.x - rhs.rect.x) < 5) {
            // x差距较小则理解为是同一排的，按y排序
            return lhs.rect.y < rhs.rect.y;
        }
        else {
            return lhs.rect.x < rhs.rect.x;
        }
    });
}

void asst::InfrastOperImageAnalyzer::sort_by_mood()
{
    LogTraceFunction;

    ranges::sort(m_result, [](const infrast::Oper& lhs, const infrast::Oper& rhs) -> bool {
        // 先按心情排序，心情低的放前面
        if (std::fabs(lhs.mood_ratio - rhs.mood_ratio) > DoubleDiff) {
            return lhs.mood_ratio < rhs.mood_ratio;
        }
        // 心情一样的就按位置排序，左边的放前面
        if (std::abs(lhs.rect.x - rhs.rect.x) > 5) {
            return lhs.rect.x < rhs.rect.x;
        }
        else {
            return lhs.rect.y < rhs.rect.y;
        }
    });
}

void asst::InfrastOperImageAnalyzer::oper_detect()
{
    LogTraceFunction;

    const Rect upper_roi = Task.get("InfrastSkillsUpper")->roi;
    const Rect lower_roi = Task.get("InfrastSkillsLower")->roi;
    const std::vector<Rect> all_roi = { upper_roi, lower_roi };

    const Rect skill_rect_move = Task.get("InfrastSkills")->rect_move;
    const Rect name_rect_move = Task.get("InfrastOperNameOcr")->rect_move;
    const Rect facility_rect_move = Task.get("InfrastOperFacilityOcr")->rect_move;
    const Rect prg_rect_move = Task.get("InfrastOperMoodProgressBar")->rect_move;
    const std::vector<Rect> all_rect_move = { skill_rect_move, name_rect_move, facility_rect_move, prg_rect_move };

    InfrastSmileyImageAnalyzer smiley_analyzer(m_image);

    for (auto&& roi : all_roi) {
        smiley_analyzer.set_roi(roi);
        if (!smiley_analyzer.analyze()) {
            continue;
        }
        for (const auto& smiley : smiley_analyzer.get_result()) {
            auto&& [_type, smiley_rect] = smiley;
            bool available = true;

            for (const Rect& rect_move : all_rect_move) {
                Rect cor_rect = rect_move;
                cor_rect.x += smiley_rect.x;
                cor_rect.y += smiley_rect.y;
                // 超过ROI边界了
                if (cor_rect.x + cor_rect.width > roi.x + roi.width || cor_rect.x < roi.x) {
                    available = false;
                    break;
                }
            }
            if (!available) {
                continue;
            }

#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(smiley_rect), cv::Scalar(0, 0, 255), 2);
#endif // ASST_DEBUG

            infrast::Oper oper;
            oper.smiley = smiley;
            oper.name_img = m_image(make_rect<cv::Rect>(smiley_rect.move(name_rect_move)));
            oper.facility_img = m_image(make_rect<cv::Rect>(smiley_rect.move(facility_rect_move)));
            m_result.emplace_back(std::move(oper));
        }
    }
}

void asst::InfrastOperImageAnalyzer::mood_analyze()
{
    LogTraceFunction;

    const auto prg_task_ptr = Task.get<MatchTaskInfo>("InfrastOperMoodProgressBar");
    uint8_t prg_lower_limit = static_cast<uint8_t>(prg_task_ptr->templ_thresholds.front());
    int prg_diff_thres = prg_task_ptr->special_params.front();
    Rect rect_move = prg_task_ptr->rect_move;

    for (auto&& oper : m_result) {
        bool not_analyze = false;
        switch (oper.smiley.type) {
        case infrast::SmileyType::Distract:
            oper.mood_ratio = 0;
            not_analyze = true;
            break;
        case infrast::SmileyType::Rest:
            oper.mood_ratio = 1.0;
            not_analyze = true;
            break;
        case infrast::SmileyType::Work:
            not_analyze = false;
            break;
        default:
            // TODO 报错
            break;
        }

        if (not_analyze) {
            continue;
        }

        Rect roi = rect_move;
        roi.x += oper.smiley.rect.x;
        roi.y += oper.smiley.rect.y;
        cv::Mat prg_image = m_image(make_rect<cv::Rect>(roi));
        cv::Mat prg_gray;
        cv::cvtColor(prg_image, prg_gray, cv::COLOR_BGR2GRAY);

        int max_white_length = 0; // 最长横扫的白色长度，即作为进度条长度
        for (int i = 0; i != prg_gray.rows; ++i) {
            int cur_white_length = 0;
            cv::uint8_t left_value = prg_lower_limit;
            for (int j = 0; j != prg_gray.cols; ++j) {
                auto value = prg_gray.at<cv::uint8_t>(i, j);
                // 当前点的颜色，需要大于最低阈值；且与相邻点的差值不能过大，否则就认为当前点不是进度条
                if (value >= prg_lower_limit && left_value < value + prg_diff_thres) {
                    left_value = value;
                    ++cur_white_length;
                    if (max_white_length < cur_white_length) {
                        max_white_length = cur_white_length;
                    }
                }
                else {
                    if (max_white_length < cur_white_length) {
                        max_white_length = cur_white_length;
                    }
                    left_value = prg_lower_limit;
                    cur_white_length = 0;
                    break;
                }
            }
        }

        // TODO：这里的进度条长度算的并不是特别准，属于能跑就行。有空再优化下
        double ratio = static_cast<double>(max_white_length) / roi.width;
        oper.mood_ratio = ratio;
#ifdef ASST_DEBUG
        cv::Point p1(roi.x, roi.y);
        cv::Point p2(roi.x + max_white_length, roi.y);
        cv::line(m_image_draw, p1, p2, cv::Scalar(0, 255, 0), 1);
        cv::putText(m_image_draw, std::to_string(ratio), p1, 1, 1.0, cv::Scalar(0, 255, 0));
#endif // ASST_DEBUG
    }
}

void asst::InfrastOperImageAnalyzer::face_hash_analyze()
{
    LogTraceFunction;

    const Rect hash_rect_move = Task.get("InfrastOperFace")->rect_move;

    Hasher hash_analyzer(m_image);

    for (auto&& oper : m_result) {
        Rect roi = oper.smiley.rect.move(hash_rect_move);
        hash_analyzer.set_roi(roi);
        hash_analyzer.analyze();
        oper.face_hash = hash_analyzer.get_hash().front();
    }
}

void asst::InfrastOperImageAnalyzer::skill_analyze()
{
    LogTraceFunction;

    const auto task_ptr = Task.get<MatchTaskInfo>("InfrastSkills");
    const int bright_thres = task_ptr->special_params.front();

    Matcher skill_analyzer(m_image);

    skill_analyzer.set_mask_range(task_ptr->mask_range);
    skill_analyzer.set_threshold(task_ptr->templ_thresholds.front());
    skill_analyzer.set_method(task_ptr->methods.front());

    for (auto&& oper : m_result) {
        Rect roi = task_ptr->rect_move;
        roi.x += oper.smiley.rect.x;
        roi.y += oper.smiley.rect.y;

        // roi里面是干员的所有技能（两个技能），这里先分别裁剪出来
        static int skill_width = roi.height;
        static int spacing = (roi.width - roi.height * MaxNumOfSkills) / (MaxNumOfSkills - 1);
        static cv::Mat mask;
        if (mask.empty()) {
            mask = cv::Mat(skill_width, skill_width, CV_8UC1, cv::Scalar(0));
            int radius = skill_width / 2;
            cv::circle(mask, cv::Point(radius, radius), radius, cv::Scalar(255, 255, 255), -1);
        }

        cv::Mat all_skills_img = m_image(make_rect<cv::Rect>(roi));
        std::string log_str = "[ ";
        for (int i = 0; i != MaxNumOfSkills; ++i) {
            int x = i * skill_width + spacing * i;
            Rect skill_rect_in_roi(x, 0, skill_width, roi.height);
            cv::Mat skill_image = all_skills_img(make_rect<cv::Rect>(skill_rect_in_roi));

            // 过滤掉亮度阈值不够的，说明是暗的技能（不是当前设施的技能）
            cv::Mat skill_gray;
            cv::cvtColor(skill_image, skill_gray, cv::COLOR_BGR2GRAY);
            cv::Scalar avg = cv::mean(skill_gray, mask);
            if (avg[0] < bright_thres) {
                continue;
            }
            Rect skill_rect = skill_rect_in_roi;
            skill_rect.x += roi.x;
            skill_rect.y += roi.y;

#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(skill_rect), cv::Scalar(0, 255, 0), 2);
#endif

            // 针对裁剪出来的每个技能进行识别
            skill_analyzer.set_roi(skill_rect);

            std::vector<std::pair<infrast::Skill, MatchRect>> possible_skills;
            // 逐个该设施内所有可能的技能，取得分最高的
            for (const auto& skill : InfrastData.get_skills(m_facility) | views::values) {
                skill_analyzer.set_templ(skill.templ_name);

                if (!skill_analyzer.analyze()) {
                    continue;
                }
                possible_skills.emplace_back(std::make_pair(skill, skill_analyzer.get_result()));
            }
            if (possible_skills.empty()) {
                Log.error("skill has no recognition result");
                continue;
            }
            // 可能的结果多于1个，只可能是同一个技能不同等级的结果
            // 例如：标准化a、标准化b，这两个模板非常像，然后分数都超过了阈值
            // 如果原图是标准化a，是不可能匹配上标准化b的模板的，因为b的模板左半边多了半个环
            // 相反如果原图是标准化b，却有可能匹配上标准化a的模板，因为a的模板右半边的环，b的原图中也有
            // 所以如果结果是同类型的，只需要取里面等级最高的那个即可
            infrast::Skill most_confident_skills;
            if (possible_skills.size() == 1) {
                most_confident_skills = possible_skills.front().first;
            }
            else if (possible_skills.size() > 1) {
                // 匹配得分最高的id作为基准，排除有识别错误，其他的技能混进来了的情况
                // 即排除容器中，除了有同一个技能的不同等级，还有别的技能的情况
                auto max_iter = ranges::max_element(possible_skills, std::less {},
                                                    [](const auto& pair) { return pair.second.score; });
                double base_score = max_iter->second.score;
                std::string base_id = max_iter->first.id;
                size_t level_pos = 0;
                // 倒着找，第一个不是数字的。前面就是技能基础id名字，后面的数字就是技能等级
                for (size_t j = base_id.size() - 1; j != 0; --j) {
                    if (!std::isdigit(base_id.at(j))) {
                        level_pos = j + 1;
                        break;
                    }
                }
                base_id = base_id.substr(0, level_pos);
                std::string max_level;
                for (const auto& [skill, skill_mr] : possible_skills) {
                    // 得分差距过大的，直接忽略
                    if (base_score - skill_mr.score > 0.05) {
                        continue;
                    }
                    if (size_t find_pos = skill.id.find(base_id); find_pos != std::string::npos) {
                        std::string cur_skill_level = skill.id.substr(base_id.size());
                        if (max_level.empty() || cur_skill_level > max_level) {
                            max_level = cur_skill_level;
                            most_confident_skills = skill;
                        }
                    } // 这里对应的else就是上述的其他技能混进来了的情况
                }
            }
            Log.trace(most_confident_skills.id, most_confident_skills.names.front());
            std::string skill_id = most_confident_skills.id;
            log_str += skill_id + " - " + most_confident_skills.names.front() + "; ";
#ifdef ASST_DEBUG
            cv::Mat skill_mat = m_image(make_rect<cv::Rect>(skill_rect));
#endif
            oper.skills.emplace(std::move(most_confident_skills));
        }
        if (!oper.skills.empty()) {
            ++m_num_of_opers_with_skills;
        }
        Log.trace(log_str, "]");
    }
}

void asst::InfrastOperImageAnalyzer::selected_analyze()
{
    LogTraceFunction;

    const auto selected_task_ptr = Task.get<MatchTaskInfo>("InfrastOperSelected");
    Rect rect_move = selected_task_ptr->rect_move;

    for (auto&& oper : m_result) {
        Rect selected_rect = rect_move;
        selected_rect.x += oper.smiley.rect.x;
        selected_rect.y += oper.smiley.rect.y;

        cv::Mat roi = m_image(make_rect<cv::Rect>(selected_rect));
        cv::Mat hsv, bin;
        cv::cvtColor(roi, hsv, cv::COLOR_BGR2HSV);
        std::vector<cv::Mat> channels;
        cv::split(hsv, channels);
        int mask_lowb = selected_task_ptr->mask_range[0].first[0];
        int mask_uppb = selected_task_ptr->mask_range[0].second[0];

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
        Log.trace("selected_analyze |", count);
        oper.selected = count >= selected_task_ptr->templ_thresholds.front();
        oper.rect = selected_rect.move({ 18, 0, 10, 160 }); // 先凑合用（
    }
}

void asst::InfrastOperImageAnalyzer::doing_analyze()
{
    LogTraceFunction;

    const auto working_task_ptr = Task.get("InfrastOperOnShift");
    Rect rect_move = working_task_ptr->rect_move;

    Matcher working_analyzer(m_image);

    working_analyzer.set_task_info(working_task_ptr);

    for (auto&& oper : m_result) {
        Rect working_rect = rect_move;
        working_rect.x += oper.smiley.rect.x;
        working_rect.y += oper.smiley.rect.y;
        working_analyzer.set_roi(working_rect);
        if (working_analyzer.analyze()) {
            oper.doing = infrast::Doing::Working;
#ifdef ASST_DEBUG
            cv::putText(m_image_draw, "Working", cv::Point(working_rect.x, working_rect.y), 1, 1, cv::Scalar(0, 0, 255),
                        2);
#endif
        }
        // TODO: infrast::Doing::Resting的识别
    }
}
