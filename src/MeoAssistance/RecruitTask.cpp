#include "RecruitTask.h"

#include <algorithm>
#include <future>
#include <map>
#include <vector>

#include "Controller.h"
#include "RecruitImageAnalyzer.h"
#include "Resource.h"
#include "ProcessTask.h"
#include "OcrImageAnalyzer.h"

using namespace asst;

bool RecruitTask::_run()
{
    json::value task_start_json = json::object{
        { "task_type", "RecruitTask_run" },
        { "task_chain", m_task_chain },
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    m_last_error = ErrorT::Ok;

    const cv::Mat& image = ctrler.get_image();
    if (image.empty()) {
        m_callback(AsstMsg::ImageIsEmpty, task_start_json, m_callback_arg);
        m_last_error = ErrorT::OtherError;
        return false;
    }

    RecruitImageAnalyzer analyzer(image);
    if (!analyzer.analyze()) {
        m_last_error = ErrorT::NotInTagsPage;
        return false;
    }

    if (m_set_time) {
        for (const auto& rect : analyzer.get_set_time_rect()) {
            ctrler.click(rect, false);
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

    /* 过滤tags数量不足的情况（可能是识别漏了） */
    if (all_tags.size() != RecruitConfiger::CorrectNumberOfTags) {
        all_tags_json["type"] = "OpenRecruit";
        m_callback(AsstMsg::OcrResultError, all_tags_json, m_callback_arg);
        m_last_error = ErrorT::TagsError;
        return false;
    }

    m_callback(AsstMsg::RecruitTagsDetected, all_tags_json, m_callback_arg);

    bool not_confirm = false;
    /* 针对一星干员的额外回调消息 */
    static const std::string SupportMachine = "支援机械";
    if (std::find(all_tags_name.cbegin(), all_tags_name.cend(), SupportMachine) != all_tags_name.cend()) {
        json::value special_tag_json;
        special_tag_json["tag"] = SupportMachine;
        m_callback(AsstMsg::RecruitSpecialTag, special_tag_json, m_callback_arg);
        not_confirm = true;
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

    // key: tags comb, value: 干员组合
    // 例如 key: { "狙击"、"群攻" }，value: RecruitOperCombs.opers{ "陨星", "白雪", "空爆" }
    std::map<std::vector<std::string>, RecruitOperCombs> result_map;
    for (const std::vector<std::string>& comb : all_combs) {
        for (const RecruitOperInfo& cur_oper : resource.recruit().get_all_opers()) {
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
                static const std::string SeniorOper = "高级资深干员";
                if (std::find(comb.cbegin(), comb.cend(), SeniorOper) == comb.cend()) {
                    continue;
                }
            }

            RecruitOperCombs& oper_combs = result_map[comb];
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
        if (result_map.find(comb) != result_map.cend()) {
            RecruitOperCombs& oper_combs = result_map[comb];
            oper_combs.avg_level /= oper_combs.opers.size();
        }
    }

    // map没法按值排序，转个vector再排序
    std::vector<std::pair<std::vector<std::string>, RecruitOperCombs>> result_vector;
    for (auto&& pair : result_map) {
        result_vector.emplace_back(std::move(pair));
    }
    std::sort(result_vector.begin(), result_vector.end(), [](const auto& lhs, const auto& rhs) -> bool {
        // 最小等级大的，排前面
        if (lhs.second.min_level != rhs.second.min_level) {
            return lhs.second.min_level > rhs.second.min_level;
        }
        // 最大等级大的，排前面
        else if (lhs.second.max_level != rhs.second.max_level) {
            return lhs.second.max_level > rhs.second.max_level;
        }
        // 平均等级高的，排前面
        else if (std::fabs(lhs.second.avg_level - rhs.second.avg_level) > DoubleDiff) {
            return lhs.second.avg_level > rhs.second.avg_level;
        }
        // Tag数量少的，排前面
        else {
            return lhs.first.size() < rhs.first.size();
        }
        });

    /* 整理识别结果 */
    std::vector<json::value> result_json_vector;
    for (const auto& [tags_comb, oper_comb] : result_vector) {
        json::value comb_json;

        std::vector<json::value> tags_json_vector;
        for (const std::string& tag : tags_comb) {
            tags_json_vector.emplace_back(tag);
        }
        comb_json["tags"] = json::array(std::move(tags_json_vector));

        std::vector<json::value> opers_json_vector;
        for (const RecruitOperInfo& oper_info : oper_comb.opers) {
            json::value oper_json;
            oper_json["name"] = oper_info.name;
            oper_json["level"] = oper_info.level;
            opers_json_vector.emplace_back(std::move(oper_json));
        }
        comb_json["opers"] = json::array(std::move(opers_json_vector));
        comb_json["tag_level"] = oper_comb.min_level;
        result_json_vector.emplace_back(std::move(comb_json));
    }
    json::value results_json;
    results_json["result"] = json::array(std::move(result_json_vector));
    m_callback(AsstMsg::RecruitResult, results_json, m_callback_arg);

    if (!result_vector.empty()) {
        int maybe_level = result_vector[0].second.min_level;
        /* 只有3星干员，且可以刷新时，则刷新 */
        if (maybe_level == 3 && m_need_refresh) {
            Rect refresh = analyzer.get_refresh_rect();
            if (!not_confirm && !refresh.empty()) {
                ProcessTask task(*this, { "RecruitRefresh" });
                task.set_retry_times(20);
                if (task.run()) {
                    // 刷新按钮不是无限的，所以这里应该不会无限递归
                    return _run();
                }
            }
        }

        /* 点击最优解的tags */
        if (std::find(m_required_level.cbegin(), m_required_level.cend(), maybe_level) != m_required_level.cend()) {
            const std::vector<std::string>& final_tags_name = result_vector[0].first;

            for (const TextRect& text_area : all_tags) {
                if (std::find(final_tags_name.cbegin(), final_tags_name.cend(), text_area.text) != final_tags_name.cend()) {
                    ctrler.click(text_area.rect, true);
                }
            }
        }
        /* 点击确认按钮 */
        if (!not_confirm &&
            std::find(m_confirm_level.cbegin(), m_confirm_level.cend(), maybe_level) != m_confirm_level.cend()) {
            ctrler.click(analyzer.get_confirm_rect(), true);
        }
        else {
            m_last_error = ErrorT::NotInConfirm;
            return true;
        }
    }

    return true;
}

void RecruitTask::set_param(std::vector<int> required_level, bool set_time) noexcept
{
    m_required_level = std::move(required_level);
    m_set_time = set_time;
}

void asst::RecruitTask::set_confirm_level(std::vector<int> confirm_level) noexcept
{
    m_confirm_level = std::move(confirm_level);
}

void asst::RecruitTask::set_need_refresh(bool need_refresh) noexcept
{
    m_need_refresh = need_refresh;
}
