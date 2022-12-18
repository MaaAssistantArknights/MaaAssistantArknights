#include "AutoRecruitTask.h"

#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/RecruitConfig.h"
#include "Config/TaskData.h"
#include "Controller.h"
#include "Task/ProcessTask.h"
#include "Task/ReportDataTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/RecruitImageAnalyzer.h"
#include "Vision/MultiMatchImageAnalyzer.h"
#include "Vision/OcrImageAnalyzer.h"

#include "Utils/Ranges.hpp"
#include <algorithm>
#include <regex>

namespace asst::recruit_calc
{
    // all combinations and their operator list, excluding empty set and 6-star operators while there is no senior tag
    auto get_all_combs(const std::vector<RecruitConfig::TagId>& tags,
                       const std::vector<Recruitment>& all_ops = RecruitData.get_all_opers())
    {
        std::vector<RecruitCombs> rcs_with_single_tag;

        {
            rcs_with_single_tag.reserve(tags.size());
            ranges::transform(tags, std::back_inserter(rcs_with_single_tag), [](const RecruitConfig::TagId& t) {
                RecruitCombs result;
                result.tags = { t };
                result.min_level = 6;
                result.max_level = 0;
                result.avg_level = 0;
                return result;
            });

            for (const auto& op : all_ops) {
                for (auto& rc : rcs_with_single_tag) {
                    if (!op.has_tag(rc.tags.front())) continue;
                    rc.opers.push_back(op);
                    rc.min_level = (std::min)(rc.min_level, op.level);
                    rc.max_level = (std::max)(rc.max_level, op.level);
                    rc.avg_level += op.level;
                }
            }

            for (auto& rc : rcs_with_single_tag) {
                rc.avg_level /= static_cast<double>(rc.opers.size());
                // intersection and union are based on sorted container
                ranges::sort(rc.tags);
                ranges::sort(rc.opers);
            }
        }

        std::vector<RecruitCombs> result;
        const size_t tag_size = tags.size();
        result.reserve(tag_size * (tag_size * tag_size + 5) / 6); // C(size, 3) + C(size, 2) + C(size, 1)

        // select one tag first
        for (size_t i = 0; i < tag_size; ++i) {
            RecruitCombs temp1 = rcs_with_single_tag[i];
            if (temp1.opers.empty()) [[unlikely]] {
                continue; // this is not possible
            }
            result.emplace_back(temp1); // that is it

            // but what if another tag is also selected
            for (size_t j = i + 1; j < tag_size; ++j) {
                RecruitCombs temp2 = temp1 * rcs_with_single_tag[j];
                if (temp2.opers.empty()) [[unlikely]] {
                    continue;
                }
                result.emplace_back(temp2); // two tags only

                // select a third one
                for (size_t k = j + 1; k < tag_size; ++k) {
                    RecruitCombs temp3 = temp2 * rcs_with_single_tag[k];
                    if (temp3.opers.empty()) [[unlikely]] {
                        continue;
                    }
                    result.emplace_back(temp3);
                }
            }
        }

        static constexpr std::string_view SeniorOper = "高级资深干员";

        for (auto comb_iter = result.begin(); comb_iter != result.end();) {
            if (ranges::find(comb_iter->tags, RecruitConfig::TagId(SeniorOper)) != comb_iter->tags.end()) {
                ++comb_iter;
                continue;
            }
            // no senior tag, remove 6-star operators
            // assuming sorted by level
            auto iter = ranges::find_if(comb_iter->opers, [](const Recruitment& op) { return op.level >= 6; });
            if (iter == comb_iter->opers.end()) {
                ++comb_iter;
                continue;
            }
            comb_iter->opers.erase(iter, comb_iter->opers.end());
            if (comb_iter->opers.empty()) {
                comb_iter = result.erase(comb_iter);
                continue;
            }
            comb_iter->update_attributes();
            ++comb_iter;
        }

        return result;
    }
} // namespace asst::recruit_calc

asst::AutoRecruitTask& asst::AutoRecruitTask::set_select_level(std::vector<int> select_level) noexcept
{
    m_select_level = std::move(select_level);
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_confirm_level(std::vector<int> confirm_level) noexcept
{
    m_confirm_level = std::move(confirm_level);
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_need_refresh(bool need_refresh) noexcept
{
    m_need_refresh = need_refresh;
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_max_times(int max_times) noexcept
{
    m_max_times = max_times;
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_use_expedited(bool use_or_not) noexcept
{
    m_use_expedited = use_or_not;
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_skip_robot(bool skip_robot) noexcept
{
    m_skip_robot = skip_robot;
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_set_time(bool set_time) noexcept
{
    m_set_time = set_time;
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_recruitment_time(std::unordered_map<int, int> time_map) noexcept
{
    m_desired_time_map = std::move(time_map);
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_penguin_enabled(bool enable, std::string penguin_id) noexcept
{
    m_upload_to_penguin = enable;
    if (!penguin_id.empty()) {
        m_penguin_id = std::move(penguin_id);
    }
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_yituliu_enabled(bool enable, std::string yituliu_id) noexcept
{
    m_upload_to_yituliu = enable;
    if (!yituliu_id.empty()) {
        m_yituliu_id = std::move(yituliu_id);
    }
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_server(std::string server) noexcept
{
    m_server = std::move(server);
    return *this;
}

bool asst::AutoRecruitTask::_run()
{
    if (m_force_discard_flag) return false;

    if (is_calc_only_task()) {
        return recruit_calc_task().success;
    }

    if (!recruit_begin()) return false;

    {
        const auto image = ctrler()->get_image();
        // initialize_dirty_slot_info(image);
        m_dirty_slots = { 0, 1, 2, 3 };
        if (!hire_all(image)) return false;
    }

    static constexpr int slot_retry_limit = 3;

    bool try_use_expedited = m_use_expedited;

    // m_cur_times means how many times has the confirm button been pressed, NOT expedited plans used
    while (m_cur_times != m_max_times) {
        if (m_force_discard_flag) {
            return false;
        }

        auto start_rect = try_get_start_button(ctrler()->get_image());
        if (start_rect) {
            if (need_exit()) return false;
            if (recruit_one(start_rect.value()))
                ++m_cur_times;
            else
                ++m_slot_fail;
            if (m_slot_fail >= slot_retry_limit) {
                return false;
            }
        }
        else {
            if (!check_recruit_home_page()) return false;
            Log.info("There is no available start button.");
            if (!try_use_expedited) return true;
        }

        if (try_use_expedited) {
            if (need_exit()) return false;
            Log.info("ready to use expedited plan");
            if (recruit_now()) {
                hire_all();
            }
            else {
                Log.info("Failed to use expedited plan");
                // There is a small chance that confirm button were clicked twice and got stuck into the bottom-right
                // slot. ref: #1491
                if (check_recruit_home_page()) {
                    // ran out of expedited plan? stop trying
                    try_use_expedited = false;
                }
                else {
                    Log.info("Not in home page after failing to use expedited plan.");
                    return false;
                }
            }
        }
    }
    return true;
}

std::vector<asst::TextRect> asst::AutoRecruitTask::start_recruit_analyze(const cv::Mat& image)
{
    OcrImageAnalyzer start_analyzer;
    start_analyzer.set_task_info("StartRecruit");
    start_analyzer.set_image(image);
    if (!start_analyzer.analyze()) return {};
    return start_analyzer.get_result();
}

std::optional<asst::Rect> asst::AutoRecruitTask::try_get_start_button(const cv::Mat& image)
{
    const auto result = start_recruit_analyze(image);
    if (result.empty()) return std::nullopt;
    auto iter = ranges::find_if(
        result, [&](const TextRect& r) -> bool { return !m_force_skipped.contains(slot_index_from_rect(r.rect)); });
    if (iter == result.cend()) return std::nullopt;
    Log.info("Found slot index", slot_index_from_rect(iter->rect), ".");
    return iter->rect;
}

/// open a pending recruit slot, set timer and tags then confirm, or leave the slot doing nothing
/// return false if:
/// - recognition failed
/// - timer or tags corrupted
/// - failed to confirm
bool asst::AutoRecruitTask::recruit_one(const Rect& button)
{
    LogTraceFunction;

    int delay = Config.get_options().task_delay;

    ctrler()->click(button);
    sleep(delay);

    auto calc_result = recruit_calc_task(slot_index_from_rect(button));
    sleep(delay);

    if (!calc_result.success) {
        // recognition failed, perhaps opening the slot again would not help
        {
            json::value info = basic_info();
            info["what"] = "RecruitError";
            info["why"] = "识别错误";
            callback(AsstMsg::SubTaskError, info);
        }
        m_force_skipped.emplace(slot_index_from_rect(button));
        click_return_button();
        return false;
    }

    if (calc_result.force_skip) {
        // mark the slot as completed and return
        m_force_skipped.emplace(slot_index_from_rect(button));
        click_return_button();
        return true;
    }

    if (need_exit()) return false;

    if (m_set_time && !check_timer(calc_result.recruitment_time)) {
        // timer was not set to 09:00:00 properly, likely the tag selection was also corrupted
        // see https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/300#issuecomment-1073287984
        // return and try later
        Log.info("Timer of this slot has not been reduced as expected.");
        click_return_button();
        return false;
    }

    // TODO: count blue pixels and compare with number of selected tags desired

    if (need_exit()) return false;

    if (!confirm()) { // ran out of recruit permit?
        Log.info("Failed to confirm current recruit config.");
        m_force_discard_flag = true;
        click_return_button();
        return false;
    }

    return true;
}

// set recruit timer and tags only
asst::AutoRecruitTask::calc_task_result_type asst::AutoRecruitTask::recruit_calc_task(slot_index index)
{
    LogTraceFunction;

    static constexpr size_t refresh_limit = 3;
    static constexpr size_t analyze_limit = 5;

    size_t refresh_count = 0;
    for (size_t image_analyzer_retry = 0; image_analyzer_retry < analyze_limit;) {
        ++image_analyzer_retry;

        RecruitImageAnalyzer image_analyzer(ctrler()->get_image());
        if (!image_analyzer.analyze()) continue;
        if (image_analyzer.get_tags_result().size() != RecruitConfig::CorrectNumberOfTags) continue;

        const std::vector<TextRect>& tags = image_analyzer.get_tags_result();
        bool has_refresh = !image_analyzer.get_refresh_rect().empty();

        std::vector<RecruitConfig::TagId> tag_ids;
        ranges::transform(tags, std::back_inserter(tag_ids), std::mem_fn(&TextRect::text));

        bool has_special_tag = false;
        bool has_robot_tag = false;

        json::value info = basic_info();
        info["details"]["tags"] = json::array(get_tag_names(tag_ids));

        // tags result
        {
            json::value cb_info = info;
            cb_info["what"] = "RecruitTagsDetected";
            callback(AsstMsg::SubTaskExtraInfo, cb_info);
        }

        // special tags
        const std::vector<RecruitConfig::TagId> SpecialTags = { "高级资深干员", "资深干员" };
        if (auto special_iter = ranges::find_first_of(SpecialTags, tag_ids); special_iter != SpecialTags.cend())
            [[unlikely]] {
            has_special_tag = true;
            json::value cb_info = info;
            cb_info["what"] = "RecruitSpecialTag";
            cb_info["details"]["tag"] = RecruitData.get_tag_name(*special_iter);
            callback(AsstMsg::SubTaskExtraInfo, cb_info);
        }

        // robot tags
        const std::vector<RecruitConfig::TagId> RobotTags = { "支援机械" };
        if (auto robot_iter = ranges::find_first_of(RobotTags, tag_ids); robot_iter != RobotTags.cend()) [[unlikely]] {
            has_robot_tag = true;
            json::value cb_info = info;
            cb_info["what"] = "RecruitSpecialTag";
            cb_info["details"]["tag"] = RecruitData.get_tag_name(*robot_iter);
            callback(AsstMsg::SubTaskExtraInfo, cb_info);
        }

        std::vector<RecruitCombs> result_vec = recruit_calc::get_all_combs(tag_ids);

        // assuming timer would be set to 09:00:00
        for (RecruitCombs& rc : result_vec) {
            if (rc.min_level < 3) {
                // find another min level (assuming operator list sorted in increment order by level)
                auto sec = ranges::find_if(rc.opers, [](const Recruitment& op) { return op.level >= 3; });
                if (sec != rc.opers.end()) {
                    rc.min_level = sec->level;
                    rc.avg_level = std::transform_reduce(sec, rc.opers.end(), 0., std::plus<double> {},
                                                         std::mem_fn(&Recruitment::level)) /
                                   static_cast<double>(std::distance(sec, rc.opers.end()));
                }
            }
        }

        ranges::sort(result_vec, [&](const RecruitCombs& lhs, const RecruitCombs& rhs) -> bool {
            // prefer the one with special tag
            // workaround for https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/1336
            if (has_special_tag) {
                bool l_has = ranges::find_first_of(lhs.tags, SpecialTags) != lhs.tags.cend();
                bool r_has = ranges::find_first_of(rhs.tags, SpecialTags) != rhs.tags.cend();
                if (l_has != r_has) return l_has > r_has;
            }

            if (lhs.min_level != rhs.min_level)
                return lhs.min_level > rhs.min_level; // 最小等级大的，排前面
            else if (lhs.max_level != rhs.max_level)
                return lhs.max_level > rhs.max_level; // 最大等级大的，排前面
            else if (std::fabs(lhs.avg_level - rhs.avg_level) > DoubleDiff)
                return lhs.avg_level > rhs.avg_level; // 平均等级高的，排前面
            else
                return lhs.tags.size() < rhs.tags.size(); // Tag数量少的，排前面
        });

        if (result_vec.empty()) continue;

        const auto& final_combination = result_vec.front();

        {
            json::value results_json;
            results_json["result"] = json::array();
            results_json["level"] = final_combination.min_level;
            std::vector<json::value> result_json_vector;
            for (const auto& comb : result_vec) {
                json::value comb_json;
                comb_json["tags"] = json::array(get_tag_names(comb.tags));

                std::vector<json::value> opers_json_vector;
                for (const Recruitment& oper_info : ranges::reverse_view(comb.opers)) { // print reversely
                    json::value oper_json;
                    oper_json["name"] = oper_info.name;
                    oper_json["level"] = oper_info.level;
                    opers_json_vector.emplace_back(std::move(oper_json));
                }
                comb_json["opers"] = json::array(std::move(opers_json_vector));
                comb_json["level"] = comb.min_level;
                results_json["result"].as_array().emplace_back(std::move(comb_json));
            }
            info["details"] |= results_json.as_object();

            json::value cb_info = info;
            cb_info["what"] = "RecruitResult";
            callback(AsstMsg::SubTaskExtraInfo, cb_info);
        }

        bool to_report = false;
        if (!is_calc_only_task()) {
            // report if the slot is clean
            if (!m_dirty_slots.contains(index)) {
                to_report = true;
                m_dirty_slots.emplace(index); // mark as dirty
            }
            else
                Log.info("will not report, dirty slots are", m_dirty_slots);
        }

#ifdef ASST_DEBUG
        to_report = true;
#endif
        if (to_report) {
            upload_result(tag_ids, info["details"]);
        }

        if (need_exit()) return {};

        // refresh
        if (m_need_refresh && has_refresh && !has_special_tag && final_combination.min_level == 3 &&
            !(m_skip_robot && has_robot_tag)) {
            if (refresh_count > refresh_limit) [[unlikely]] {
                json::value cb_info = basic_info();
                cb_info["what"] = "RecruitError";
                cb_info["why"] = "刷新次数达到上限";
                cb_info["details"] = json::object { { "refresh_limit", refresh_limit } };
                callback(AsstMsg::SubTaskError, cb_info);
                return {};
            }

            refresh();

            ++refresh_count;

            // mark the slot clean after refreshed
            m_dirty_slots.erase(index);

            {
                json::value cb_info = basic_info();
                cb_info["what"] = "RecruitTagsRefreshed";
                cb_info["details"] = json::object {
                    { "count", refresh_count },
                    { "refresh_limit", refresh_limit },
                };
                callback(AsstMsg::SubTaskExtraInfo, cb_info);
                Log.trace("recruit tags refreshed", std::to_string(refresh_count), " times, rerunning recruit task");
            }

            // desired retry, not an error
            --image_analyzer_retry;
            continue;
        }

        if (need_exit()) return {};

        if (!is_calc_only_task()) {
            // do not confirm, force skip
            if (ranges::none_of(m_confirm_level, [&](const auto& i) { return i == final_combination.min_level; })) {
                calc_task_result_type result;
                result.success = true;
                result.force_skip = true;
                return result;
            }

            if (m_skip_robot && has_robot_tag) {
                calc_task_result_type result;
                result.success = true;
                result.force_skip = true;
                return result;
            }
        }

        int recruitment_time = m_desired_time_map[(std::max)(final_combination.min_level, 3)];
        if (recruitment_time == 0) recruitment_time = 9 * 60;

        // try to set the timer to desired value
        if (m_set_time) {
            Log.info("recruitment time:", recruitment_time, "min");
            const int desired_hour = recruitment_time / 60;
            const int desired_minute_div_10 = (recruitment_time % 60) / 10;
            const int temp = desired_hour + (desired_minute_div_10 != 0);
            const int hour_delta = (1 < temp) ? (1 + 9 - temp) : (temp - 1);
            const int minute_delta = (0 < desired_minute_div_10) ? (0 + 6 - desired_minute_div_10) : (0);
            for (int i = 0; i < hour_delta; ++i)
                ctrler()->click(image_analyzer.get_hour_decrement_rect());
            for (int i = 0; i < minute_delta; ++i)
                ctrler()->click(image_analyzer.get_minute_decrement_rect());
        }

        // nothing to select, leave the selection empty
        if (ranges::none_of(m_select_level, [&](const auto& i) { return i == final_combination.min_level; })) {
            calc_task_result_type result;
            result.success = true;
            result.force_skip = false;
            result.recruitment_time = recruitment_time;
            result.tags_selected = 0;
            return result;
        }

        // select tags
        for (const std::string& final_tag_name : final_combination.tags) {
            auto tag_rect_iter = ranges::find_if(tags, [&](const TextRect& r) { return r.text == final_tag_name; });
            if (tag_rect_iter != tags.cend()) {
                ctrler()->click(tag_rect_iter->rect);
            }
        }

        {
            json::value cb_info = basic_info();
            cb_info["what"] = "RecruitTagsSelected";
            cb_info["details"] = json::object { { "tags", json::array(get_tag_names(final_combination.tags)) } };
            callback(AsstMsg::SubTaskExtraInfo, cb_info);
        }

        calc_task_result_type result;
        result.success = true;
        result.force_skip = false;
        result.recruitment_time = recruitment_time;
        result.tags_selected = static_cast<int>(final_combination.tags.size());
        return result;
    }
    return {};
}

bool asst::AutoRecruitTask::recruit_begin()
{
    ProcessTask task(*this, { "RecruitBegin" });
    return task.run();
}

bool asst::AutoRecruitTask::check_timer(int minutes_expected)
{
    const auto image = ctrler()->get_image();
    const auto replace_map = Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map;

    {
        OcrImageAnalyzer hour_ocr(image);
        hour_ocr.set_task_info("RecruitTimerH");
        hour_ocr.set_replace(replace_map);
        if (!hour_ocr.analyze()) return false;
        std::string desired_hour_str = std::string("0") + std::to_string(minutes_expected / 60);
        if (hour_ocr.get_result().front().text != desired_hour_str) return false;
    }
    if (minutes_expected % 60 == 0) return true; // minute counter stays untouched

    {
        OcrImageAnalyzer minute_ocr(image);
        minute_ocr.set_task_info("RecruitTimerM");
        minute_ocr.set_replace(replace_map);
        if (!minute_ocr.analyze()) return false;
        std::string desired_minute_str = std::to_string((minutes_expected % 60) / 10) + "0";
        if (minute_ocr.get_result().front().text != desired_minute_str) return false;
    }
    return true;
}

bool asst::AutoRecruitTask::check_recruit_home_page()
{
    ProcessTask task(*this, { "RecruitFlag" });
    task.set_retry_times(2);
    return task.run();
}

bool asst::AutoRecruitTask::recruit_now()
{
    ProcessTask task(*this, { "RecruitNow" });
    return task.run();
}

bool asst::AutoRecruitTask::confirm()
{
    ProcessTask confirm_task(*this, { "RecruitConfirm" });
    return confirm_task.set_retry_times(5).run();
}

bool asst::AutoRecruitTask::refresh()
{
    ProcessTask refresh_task(*this, { "RecruitRefresh" });
    return refresh_task.run();
}

bool asst::AutoRecruitTask::hire_all(const cv::Mat& image)
{
    LogTraceFunction;
    // mark slots with *Hire* button clean (regardless of whether hiring will success)
    {
        MultiMatchImageAnalyzer hire_searcher(image);
        hire_searcher.set_task_info("RecruitFinish");
        hire_searcher.analyze();
        for (const MatchRect& r : hire_searcher.get_result()) {
            Log.info("Mark", slot_index_from_rect(r.rect), "clean");
            m_dirty_slots.erase(slot_index_from_rect(r.rect));
        }
        if (hire_searcher.get_result().empty()) return true;
    }
    // hire all
    return ProcessTask { *this, { "RecruitFinish" } }.run();
}

/// search for blue *Hire* buttons in the recruit home page, mark those slot clean and do hiring
bool asst::AutoRecruitTask::hire_all()
{
    return hire_all(ctrler()->get_image());
}

/// search for *RecruitNow* buttons before recruit and mark them as dirty
[[maybe_unused]] bool asst::AutoRecruitTask::initialize_dirty_slot_info(const cv::Mat& image)
{
    m_dirty_slots.clear();
    const auto result = start_recruit_analyze(image);
    for (const TextRect& r : result) {
        m_dirty_slots.emplace(slot_index_from_rect(r.rect));
    }
    Log.info("Dirty slots are", m_dirty_slots);
    return true;
}

std::vector<std::string> asst::AutoRecruitTask::get_tag_names(const std::vector<RecruitConfig::TagId>& ids) const
{
    std::vector<std::string> names;
    for (const RecruitConfig::TagId& id : ids) {
        names.emplace_back(RecruitData.get_tag_name(id));
    }
    return names;
}

template <typename Rng>
void asst::AutoRecruitTask::upload_result(const Rng& tag_ids, const json::value& yituliu_details)
{
    LogTraceFunction;
    if (m_upload_to_penguin) {
        upload_to_penguin(tag_ids);
    }
    if (m_upload_to_yituliu) {
        upload_to_yituliu(yituliu_details);
    }
}

template <typename Rng>
void asst::AutoRecruitTask::upload_to_penguin(Rng&& tags)
{
    LogTraceFunction;

    json::value body;
    body["server"] = m_server;
    body["stageId"] = "recruit";
    auto& all_drops = body["drops"];
    for (const auto& tag : tags) {
        all_drops.array_emplace(json::object {
            { "dropType", "NORMAL_DROP" },
            { "itemId", tag },
            { "quantity", 1 },
        });
    }
    body["source"] = UploadDataSource;
    body["version"] = Version;

    std::string extra_param;
    if (!m_penguin_id.empty()) {
        extra_param = "-H \"authorization: PenguinID " + m_penguin_id + "\"";
    }

    if (!m_report_penguin_task_ptr) {
        m_report_penguin_task_ptr = std::make_shared<ReportDataTask>(report_penguin_callback, this);
    }

    m_report_penguin_task_ptr->set_report_type(ReportType::PenguinStats)
        .set_body(body.to_string())
        .set_extra_param(extra_param)
        .set_retry_times(5)
        .run();
}

void asst::AutoRecruitTask::upload_to_yituliu(const json::value& details)
{
    LogTraceFunction;

    json::value body = details;
    body["server"] = m_server;
    body["source"] = UploadDataSource;
    body["version"] = Version;
    body["uuid"] = /* m_yituliu_id */ m_penguin_id;

    if (!m_report_yituliu_task_ptr) {
        m_report_yituliu_task_ptr = std::make_shared<ReportDataTask>(report_yituliu_callback, this);
    }

    m_report_yituliu_task_ptr->set_report_type(ReportType::YituliuBigData)
        .set_body(body.to_string())
        .set_retry_times(0)
        .run();
}

void asst::AutoRecruitTask::report_penguin_callback(AsstMsg msg, const json::value& detail, AbstractTask* task_ptr)
{
    LogTraceFunction;

    auto p_this = dynamic_cast<AutoRecruitTask*>(task_ptr);
    if (!p_this) {
        return;
    }

    if (msg == AsstMsg::SubTaskExtraInfo && detail.get("what", std::string()) == "PenguinId") {
        std::string id = detail.get("details", "id", std::string());
        p_this->m_penguin_id = id;
    }

    p_this->callback(msg, detail);
}

void asst::AutoRecruitTask::report_yituliu_callback(AsstMsg msg, const json::value& detail, AbstractTask* task_ptr)
{
    LogTraceFunction;

    auto p_this = dynamic_cast<AutoRecruitTask*>(task_ptr);
    if (!p_this) {
        return;
    }

    p_this->callback(msg, detail);
}
