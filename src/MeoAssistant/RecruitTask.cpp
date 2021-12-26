#include "RecruitTask.h"

#include <algorithm>
#include <vector>

#include "Controller.h"
#include "RecruitImageAnalyzer.h"
#include "Resource.h"
#include "ProcessTask.h"

using namespace asst;

bool RecruitTask::_run()
{
    json::value task_start_json = json::object{
        { "task_type", "RecruitTask_run" },
        { "task_chain", m_task_chain },
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    m_maybe_level = 0;
    m_has_special_tag = false;
    m_has_refresh = false;

    const cv::Mat image = Ctrler.get_image();
    if (image.empty()) {
        m_callback(AsstMsg::ImageIsEmpty, task_start_json, m_callback_arg);
        return false;
    }

    RecruitImageAnalyzer analyzer(image);

    if (!analyzer.analyze()) {
        return false;
    }
    m_has_refresh = !analyzer.get_refresh_rect().empty();

    if (m_set_time) {
        for (const auto& rect : analyzer.get_set_time_rect()) {
            Ctrler.click(rect, false);
        }
    }
    const std::vector<TextRect>& all_tags = analyzer.get_tags_result();

    std::unordered_set<std::string> all_tags_name;
    std::vector<json::value> all_tags_json_vector;
    for (const TextRect& text_area : all_tags) {
        all_tags_name.emplace(text_area.text);
        all_tags_json_vector.emplace_back(text_area.text);
    }
    json::value all_tags_json;
    all_tags_json["tags"] = json::array(all_tags_json_vector);
    all_tags_json["task_chain"] = m_task_chain;

    /* 过滤tags数量不足的情况（可能是识别漏了） */
    if (all_tags.size() != RecruitConfiger::CorrectNumberOfTags) {
        //all_tags_json["type"] = "OpenRecruit";
        //m_callback(AsstMsg::OcrResultError, all_tags_json, m_callback_arg);
        return false;
    }

    m_callback(AsstMsg::RecruitTagsDetected, all_tags_json, m_callback_arg);

    /* 针对特殊Tag的额外回调消息 */
    static const std::string SeniorOper = "高级资深干员";
    static const std::string SupportMachine = "支援机械";
    const std::vector<std::string> SpecialTags = { SeniorOper, SupportMachine };

    auto special_iter = std::find_first_of(SpecialTags.cbegin(), SpecialTags.cend(), all_tags_name.cbegin(), all_tags_name.cend());
    if (special_iter != SpecialTags.cend()) {
        json::value special_tag_json;
        special_tag_json["tag"] = *special_iter;
        m_callback(AsstMsg::RecruitSpecialTag, special_tag_json, m_callback_arg);
        m_has_special_tag = true;
    }

    // 识别到的5个Tags，全组合排列
    std::vector<std::vector<std::string>> all_combs;
    size_t len = all_tags.size();
    int count = (std::pow)(2, len);
    for (int i = 0; i < count; ++i) {
        std::vector<std::string> temp;
        for (int j = 0, mask = 1; j < len; ++j) {
            if ((i & mask) != 0) { // What the fuck???
                temp.emplace_back(all_tags.at(j).text);
            }
            mask = mask * 2;
        }
        // 游戏里最多选择3个tag
        if (!temp.empty() && temp.size() <= 3) {
            all_combs.emplace_back(std::move(temp));
        }
    }

    std::vector<RecruitCombs> result_vec;
    for (const std::vector<std::string>& comb : all_combs) {
        RecruitCombs oper_combs;
        oper_combs.tags = comb;

        for (const RecruitOperInfo& cur_oper : Resrc.recruit().get_all_opers()) {
            int matched_count = 0;
            for (const std::string& tag : comb) {
                if (cur_oper.tags.find(tag) != cur_oper.tags.cend()) {
                    ++matched_count;
                }
                else {
                    break;
                }
            }

            // 单个tags comb中的每一个tag，这个干员都有，才算该干员符合这个tags comb
            if (matched_count != comb.size()) {
                continue;
            }

            if (cur_oper.level == 6) {
                // 高资tag和六星强绑定，如果没有高资tag，即使其他tag匹配上了也不可能出六星
                if (std::find(comb.cbegin(), comb.cend(), SeniorOper) == comb.cend()) {
                    continue;
                }
            }

            oper_combs.opers.emplace_back(cur_oper);

            if (cur_oper.level == 1 || cur_oper.level == 2) {
                if (oper_combs.min_level == 0)
                    oper_combs.min_level = cur_oper.level;
                if (oper_combs.max_level == 0)
                    oper_combs.max_level = cur_oper.level;
                // 一星、二星干员不计入最低等级，因为拉满9小时之后不可能出1、2星
                continue;
            }
            if (oper_combs.min_level == 0 || oper_combs.min_level > cur_oper.level) {
                oper_combs.min_level = cur_oper.level;
            }
            if (oper_combs.max_level == 0 || oper_combs.max_level < cur_oper.level) {
                oper_combs.max_level = cur_oper.level;
            }
            oper_combs.avg_level += cur_oper.level;
        }
        if (!oper_combs.opers.empty()) {
            oper_combs.avg_level /= oper_combs.opers.size();
            result_vec.emplace_back(std::move(oper_combs));
        }
    }

    std::sort(result_vec.begin(), result_vec.end(),
        [](const RecruitCombs& lhs, const RecruitCombs& rhs) -> bool {
            // 最小等级大的，排前面
            if (lhs.min_level != rhs.min_level) {
                return lhs.min_level > rhs.min_level;
            }
            // 最大等级大的，排前面
            else if (lhs.max_level != rhs.max_level) {
                return lhs.max_level > rhs.max_level;
            }
            // 平均等级高的，排前面
            else if (std::fabs(lhs.avg_level - rhs.avg_level) > DoubleDiff) {
                return lhs.avg_level > rhs.avg_level;
            }
            // Tag数量少的，排前面
            else {
                return lhs.tags.size() < rhs.tags.size();
            }
        });

    if (!result_vec.empty()) {
        m_maybe_level = result_vec.at(0).min_level;
    }
    else {
        m_maybe_level = 0;
    }

    /* 整理识别结果Json */
    json::value results_json;
    results_json["result"] = json::array();
    results_json["maybe_level"] = m_maybe_level;
    results_json["task_chain"] = m_task_chain;

    std::vector<json::value> result_json_vector;
    for (const auto& comb : result_vec) {
        auto& tags = comb.tags;
        json::value comb_json;

        std::vector<json::value> tags_json_vector;
        for (const std::string& tag : tags) {
            tags_json_vector.emplace_back(tag);
        }
        comb_json["tags"] = json::array(std::move(tags_json_vector));

        std::vector<json::value> opers_json_vector;
        for (const RecruitOperInfo& oper_info : comb.opers) {
            json::value oper_json;
            oper_json["name"] = oper_info.name;
            oper_json["level"] = oper_info.level;
            opers_json_vector.emplace_back(std::move(oper_json));
        }
        comb_json["opers"] = json::array(std::move(opers_json_vector));
        comb_json["tag_level"] = comb.min_level;
        results_json["result"].as_array().emplace_back(std::move(comb_json));
    }
    m_callback(AsstMsg::RecruitResult, results_json, m_callback_arg);

    if (!result_vec.empty()) {
        /* 点击最优解的tags */
        if (std::find(m_select_level.cbegin(), m_select_level.cend(), m_maybe_level) != m_select_level.cend()) {
            const std::vector<std::string>& final_tags_name = result_vec.at(0).tags;

            for (const TextRect& text_area : all_tags) {
                if (std::find(final_tags_name.cbegin(), final_tags_name.cend(), text_area.text) != final_tags_name.cend()) {
                    Ctrler.click(text_area.rect, true);
                }
            }

            json::value selected_json;
            selected_json["tags"] = json::array(final_tags_name);
            m_callback(AsstMsg::RecruitSelected, selected_json, m_callback_arg);
        }
    }

    return true;
}

void RecruitTask::set_param(std::vector<int> select_level, bool set_time) noexcept
{
    m_select_level = std::move(select_level);
    m_set_time = set_time;
}
