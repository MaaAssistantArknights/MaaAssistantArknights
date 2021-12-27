#include "InfrastOperImageAnalyzer.h"

#include "InfrastSmileyImageAnalyzer.h"
#include "MatchImageAnalyzer.h"
#include "Logger.hpp"
#include "Resource.h"

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
    if (m_to_be_calced & NameHash) {
        name_hash_analyze();
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

    std::sort(
        m_result.begin(), m_result.end(),
        [](const infrast::Oper& lhs, const infrast::Oper& rhs) -> bool {
            if (std::abs(lhs.rect.x - rhs.rect.x) < 5) { // x差距较小则理解为是同一排的，按y排序
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

    std::sort(
        m_result.begin(), m_result.end(),
        [](const infrast::Oper& lhs, const infrast::Oper& rhs) -> bool {
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

    const Rect upper_roi = task.get("InfrastSkillsUpper")->roi;
    const Rect lower_roi = task.get("InfrastSkillsLower")->roi;
    const std::vector<Rect> all_roi = { upper_roi, lower_roi };

    const Rect skill_rect_move = task.get("InfrastSkills")->rect_move;
    const Rect hash_rect_move = task.get("InfrastOperNameHash")->rect_move;
    const Rect prg_rect_move = task.get("InfrastOperMoodProgressBar")->roi;
    const std::vector<Rect> all_rect_move = { skill_rect_move, hash_rect_move, prg_rect_move };

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

#ifdef LOG_TRACE
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(smiley_rect), cv::Scalar(0, 0, 255), 2);
#endif // LOG_TRACE

            infrast::Oper oper;
            oper.smiley = smiley;
            m_result.emplace_back(std::move(oper));
        }
    }
}

void asst::InfrastOperImageAnalyzer::mood_analyze()
{
    LogTraceFunction;

    const auto prg_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        task.get("InfrastOperMoodProgressBar"));
    int prg_lower_limit = prg_task_ptr->templ_threshold;
    int prg_diff_thres = prg_task_ptr->special_threshold;
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
        cv::Mat prg_image = m_image(utils::make_rect<cv::Rect>(roi));
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
#ifdef LOG_TRACE
        cv::Point p1(roi.x, roi.y);
        cv::Point p2(roi.x + max_white_length, roi.y);
        cv::line(m_image_draw, p1, p2, cv::Scalar(0, 255, 0), 1);
        cv::putText(m_image_draw, std::to_string(ratio), p1, 1, 1.0, cv::Scalar(0, 255, 0));
#endif // LOG_TRACE
    }
}

void asst::InfrastOperImageAnalyzer::face_hash_analyze()
{
    LogTraceFunction;

    const Rect hash_rect_move = task.get("InfrastOperFaceHash")->rect_move;

    for (auto&& oper : m_result) {
        Rect roi = hash_rect_move;
        roi.x += oper.smiley.rect.x;
        roi.y += oper.smiley.rect.y;

        cv::Mat image_roi = m_image(utils::make_rect<cv::Rect>(roi));
        oper.face_hash = hash_calc(image_roi);
    }
}

void asst::InfrastOperImageAnalyzer::name_hash_analyze()
{
    LogTraceFunction;

    const Rect hash_rect_move = task.get("InfrastOperNameHash")->rect_move;

    cv::Mat gray;
    cv::cvtColor(m_image, gray, cv::COLOR_BGR2GRAY);

    for (auto&& oper : m_result) {
        Rect roi = hash_rect_move;
        roi.x += oper.smiley.rect.x;
        roi.y += oper.smiley.rect.y;

        constexpr static int threshold = 100;
        auto check_point = [&](cv::Point point) -> bool {
            auto value = gray.at<uchar>(point);
            return value > threshold;
        };
        // 找到四个方向上最靠外的白色点，把ROI缩小裁出来
        int left = -1, right = -1, top = INT_MAX, bottom = -1;
        for (int i = 0; i != roi.width; ++i) {
            for (int j = 0; j != roi.height; ++j) {
                cv::Point point(roi.x + i, roi.y + j);
                if (check_point(point)) {
                    if (left < 0) {
                        left = i;
                    }
                    right = i;
                    top = (std::min)(top, j);
                    bottom = (std::max)(bottom, j);
                }
            }
        }
        roi.x += left;
        roi.width = right - left + 1;
        roi.y += top;
        roi.height = bottom - top + 1;

        cv::Mat hash_roi = m_image(utils::make_rect<cv::Rect>(roi));
        oper.name_hash = hash_calc(hash_roi);
    }
}

void asst::InfrastOperImageAnalyzer::skill_analyze()
{
    LogTraceFunction;

    const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        task.get("InfrastSkills"));
    const auto bright_thres = task_ptr->special_threshold;

    MatchImageAnalyzer skill_analyzer(m_image);
    skill_analyzer.set_mask_range(task_ptr->mask_range);
    skill_analyzer.set_threshold(task_ptr->templ_threshold);

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

        cv::Mat all_skills_img = m_image(utils::make_rect<cv::Rect>(roi));
        std::string log_str = "[ ";
        for (int i = 0; i != MaxNumOfSkills; ++i) {
            int x = i * skill_width + spacing * i;
            Rect skill_rect_in_roi(x, 0, skill_width, roi.height);
            cv::Mat skill_image = all_skills_img(utils::make_rect<cv::Rect>(skill_rect_in_roi));

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

#ifdef LOG_TRACE
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(skill_rect), cv::Scalar(0, 255, 0), 2);
#endif

            // 针对裁剪出来的每个技能进行识别
            skill_analyzer.set_roi(skill_rect);

            std::vector<std::pair<infrast::Skill, MatchRect>> possible_skills;
            // 逐个该设施内所有可能的技能，取得分最高的
            for (const auto& [id, skill] : Resrc.infrast().get_skills(m_facility)) {
                skill_analyzer.set_templ_name(skill.templ_name);

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
                auto max_iter = std::max_element(
                    possible_skills.begin(), possible_skills.end(),
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
            Log.trace(most_confident_skills.id, most_confident_skills.names.front());
            std::string skill_id = most_confident_skills.id;
            log_str += skill_id + " - " + most_confident_skills.names.front() + "; ";
#ifdef LOG_TRACE
            cv::Mat skill_mat = m_image(utils::make_rect<cv::Rect>(skill_rect));
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

    const auto selected_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        task.get("InfrastOperSelected"));
    Rect rect_move = selected_task_ptr->rect_move;

    for (auto&& oper : m_result) {
        Rect selected_rect = rect_move;
        selected_rect.x += oper.smiley.rect.x;
        selected_rect.y += oper.smiley.rect.y;

        cv::Mat roi = m_image(utils::make_rect<cv::Rect>(selected_rect));
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
        Log.trace("selected_analyze |", count);
        oper.selected = count >= selected_task_ptr->templ_threshold;
        oper.rect = selected_rect;  // 先凑合用（
    }
}

void asst::InfrastOperImageAnalyzer::doing_analyze()
{
    LogTraceFunction;

    const auto working_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        task.get("InfrastOperOnShift"));
    Rect rect_move = working_task_ptr->rect_move;

    MatchImageAnalyzer working_analyzer(m_image);
    working_analyzer.set_task_info(*working_task_ptr);

    for (auto&& oper : m_result) {
        Rect working_rect = rect_move;
        working_rect.x += oper.smiley.rect.x;
        working_rect.y += oper.smiley.rect.y;
        working_analyzer.set_roi(working_rect);
        if (working_analyzer.analyze()) {
            oper.doing = infrast::Doing::Working;
#ifdef LOG_TRACE
            cv::putText(m_image_draw, "Working", cv::Point(working_rect.x, working_rect.y), 1, 1, cv::Scalar(0, 0, 255), 2);
#endif
        }
        // TODO: infrast::Doing::Resting的识别
    }
}

std::string asst::InfrastOperImageAnalyzer::hash_calc(const cv::Mat image)
{
    //constexpr static int HashKernelSize = 16;
    const static cv::Size HashKernel(16, 16);

    cv::Mat hash_img;
    cv::resize(image, hash_img, HashKernel);
    cv::cvtColor(hash_img, hash_img, cv::COLOR_BGR2GRAY);

    std::stringstream hash_value;
    cv::uint8_t* pix = hash_img.data;
    int tmp_dec = 0;
    for (int ro = 0; ro < 256; ro++) {
        tmp_dec = tmp_dec << 1;
        if (*pix > 127)
            tmp_dec++;
        if (ro % 4 == 3) {
            hash_value << std::hex << tmp_dec;
            tmp_dec = 0;
        }
        pix++;
    }
    return hash_value.str();
}
