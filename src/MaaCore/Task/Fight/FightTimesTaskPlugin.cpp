#include "FightTimesTaskPlugin.h"

#include "Config/GeneralConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/NoWarningCV.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::FightTimesTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    return details.get("details", "task", "").ends_with("StartButton1");
}

bool asst::FightTimesTaskPlugin::_run()
{
    LogTraceFunction;
    json::value info = basic_info_with_what("FightInfoBeforeStage");
    // {"sanity_current": 100, "sanity_max": 135, "report_time": "2023-09-01 09:31:53.527"}
    auto image = ctrler()->get_image();
    auto sanity = analyze_sanity_remain(image);
    if (!sanity) {
        Log.error(__FUNCTION__, "unable to analyze sanity");
        callback(AsstMsg::SubTaskExtraInfo, info);
        return false;
    }

    info["details"]["report_time"] = utils::get_format_time();
    info["details"]["sanity_current"] = sanity->current;
    info["details"]["sanity_max"] = sanity->max;
    info["details"]["fight_times_finished"] = m_fight_times;

    if (m_fight_times >= m_fight_times_max) {
        m_task_ptr->set_enable(false); // 战斗次数已达上限
        Log.info(__FUNCTION__, "fight times reached max");
        callback(AsstMsg::SubTaskExtraInfo, info);
        return true;
    }

    auto sanity_cost = analyze_sanity_cost(image);
    auto series = analyze_stage_series(image);
    if (sanity_cost.value_or(-1) < 0 || (series && (*series < 1 || *series > 6))) [[unlikely]] {
        Log.error(__FUNCTION__, "unable to analyze sanity cost or series");
        callback(AsstMsg::SubTaskExtraInfo, info);
        return false;
    }
    if (!series) { // 默认连续战斗次数为1, 部分关卡不支持连战
        info["details"]["series"] = 1;
        info["details"]["sanity_cost"] = *sanity_cost;
        Log.info(__FUNCTION__, "series not support");
        callback(AsstMsg::SubTaskExtraInfo, info);
        return true;
    }

    if (change_series(sanity->current, *sanity_cost, *series)) {
        image = ctrler()->get_image();
        sanity_cost = analyze_sanity_cost(image);
        series = analyze_stage_series(image);
        if (sanity_cost.value_or(-1) < 0 || !series || *series < 1 || *series > 6) [[unlikely]] {
            Log.error(__FUNCTION__, "unable to analyze sanity cost or series");
            callback(AsstMsg::SubTaskExtraInfo, info);
            return false;
        }
    }

    // 连续战斗次数+当前战斗次数 <= 最大战斗次数
    info["details"]["series"] = *series;
    info["details"]["sanity_cost"] = *sanity_cost;
    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}

bool asst::FightTimesTaskPlugin::open_series_list(const cv::Mat& image)
{
    return ProcessTask(*this, { "FightSeries-Opened", "FightSeries-Open" }).set_reusable_image(image).run();
}

bool asst::FightTimesTaskPlugin::change_series(int sanity_current, int sanity_cost, int series)
{
    int fight_times_remain = std::min(m_fight_times_max - m_fight_times, 6);
    if (!m_has_used_medicine) {
        if (fight_times_remain != series) {
            // 调整到剩余次数
            set_series(false);
            return true;
        }
        return false;
    }

    // 用过药品, 选择最大可用次数
    if (sanity_cost <= sanity_current) { // 吃药前一般选择最大可用次数, 吃完药已经够理智了
        return false;
    }

    set_series(true);
    return true;
}

bool asst::FightTimesTaskPlugin::set_series(bool available_only)
{
    if (open_series_list()) {
        Log.error(__FUNCTION__, "unable to open series list");
        return false;
    }
    int fight_times_remain = std::min(m_fight_times_max - m_fight_times, 6);
    auto image = ctrler()->get_image();
    auto list = analyze_series_list(image);
    if (list.empty()) {
        Log.error(__FUNCTION__, "unable to analyze series list");
        return false;
    }
    for (const auto& item : list) {
        if (item.times > fight_times_remain) {
            continue;
        }

        if (!available_only || item.status == asst::FightSeriesListItem::Status::Available) {
            ctrler()->click(item.rect);
            sleep(Config.get_options().task_delay);
            return true;
        }
    }
    Log.error(__FUNCTION__, "no available series found");
    return false;
}

bool asst::FightTimesTaskPlugin::modify_series(int series, const cv::Mat& image)
{
    return ProcessTask(*this, { "FightSeries-List-" + std::to_string(series) }).set_reusable_image(image).run();
}

std::vector<asst::FightSeriesListItem> asst::FightTimesTaskPlugin::analyze_series_list(const cv::Mat& image)
{
    std::vector<asst::FightSeriesListItem> list;
    const auto& task = Task.get("FightSeries-List-Multiply");
    MultiMatcher ocr(image);
    ocr.set_task_info(task);
    if (!ocr.analyze()) {
        Log.error(__FUNCTION__, "unable to analyze series list");
        return list;
    }
    auto result = ocr.get_result();
    if (result.size() != 6) {
        Log.error(__FUNCTION__, "no series found");
        return list;
    }
    sort_by_vertical_(result);

    double min_val = 0.0, max_val = 0.0;
    cv::Mat number_img, number_img_gray;
    cv::Point min_loc, max_loc;
    int index = 7;
    for (const auto& item : result) {
        --index;
        auto rect = item.rect.move(task->specific_rect);
        number_img = make_roi(image, item.rect.move(task->rect_move));
        cv::cvtColor(number_img, number_img_gray, cv::COLOR_BGR2GRAY);
        cv::minMaxLoc(number_img_gray, &min_val, &max_val, &min_loc, &max_loc);
        if (max_val < task->special_params[0]) {
            list.emplace_back(
                asst::FightSeriesListItem { .times = index,
                                            .rect = rect,
                                            .status = asst::FightSeriesListItem::Status::NeedStone });
            continue;
        }

        Matcher match(image);
        match.set_task_info("FightSeries-List-Exceeded");
        match.set_roi(rect);
        list.emplace_back(
            asst::FightSeriesListItem { .times = index,
                                        .rect = rect,
                                        .status = match.analyze() ? asst::FightSeriesListItem::Status::NeedMedicine
                                                                  : asst::FightSeriesListItem::Status::Available });
        /* 假设 连战列表 为6-1
        RegionOCRer analyzer(image);
        analyzer.set_roi(item.rect.move(task->rect_move));
        analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);
        analyzer.set_use_char_model(true);
        analyzer.set_bin_threshold(0);
        if (!analyzer.analyze()) {
            Log.error(__FUNCTION__, "unable to analyze series");
        }
        else {
            int times = 0;
            if (!utils::chars_to_number(analyzer.get_result().text, times)) [[unlikely]] {
                LogWarn << __FUNCTION__ << "Series ocr result could not convert to int:" << analyzer.get_result().text
                        << "has analyzed:" << list.size();
            }
        }*/
    }
    // ranges::reverse(list);
    return list;
}

std::optional<asst::SanityResult> asst::FightTimesTaskPlugin::analyze_sanity_remain(const cv::Mat& image)
{
    RegionOCRer analyzer(image);
    analyzer.set_task_info("SanityMatch");
    analyzer.set_bin_threshold(0, 255);
    auto res_opt = analyzer.analyze();

    if (!res_opt) [[unlikely]] {
        Log.warn(__FUNCTION__, "Sanity ocr failed");
        analyzer.save_img(utils::path("debug") / utils::path("sanity"));
        return std::nullopt;
    }

    std::string_view text = res_opt->text;
    auto slash_pos = text.find('/');
    if (slash_pos == std::string_view::npos) [[unlikely]] {
        Log.warn(__FUNCTION__, "Sanity ocr result without '/':", text);
        analyzer.save_img(utils::path("debug") / utils::path("sanity"));
        return std::nullopt;
    }

    int sanity_cur = 0, sanity_max = 0;
    if (!utils::chars_to_number(text.substr(0, slash_pos), sanity_cur) ||
        !utils::chars_to_number(text.substr(slash_pos + 1), sanity_max)) [[unlikely]] {
        Log.warn(__FUNCTION__, "Sanity ocr result could not convert to int:", text);
        analyzer.save_img(utils::path("debug") / utils::path("sanity"));
        return std::nullopt;
    }

    Log.info(__FUNCTION__, "Current Sanity:", sanity_cur, ", Max Sanity:", sanity_max);
    if (sanity_cur < 0 || sanity_max > 180 || sanity_max < 82) [[unlikely]] {
        // 理智上限[82,135] 2024.11.01 上限增加45点 [127, 180]
        Log.warn(__FUNCTION__, "Sanity out of limit");
        analyzer.save_img(utils::path("debug") / utils::path("sanity"));
        return std::nullopt;
    }

    return asst::SanityResult { .current = sanity_cur, .max = sanity_max };
}

std::optional<int> asst::FightTimesTaskPlugin::analyze_stage_series(const cv::Mat& image)
{
    LogTraceFunction;
    const auto& task = Task.get("FightSeries-Icon");

    Matcher match(image);
    match.set_task_info(task);
    if (!match.analyze()) {
        Log.error(__FUNCTION__, "unable to match series icon");
        return std::nullopt;
    }

    RegionOCRer analyzer(image);
    analyzer.set_roi(match.get_result().rect.move(task->rect_move));
    analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);
    analyzer.set_bin_threshold(0, 255);
    analyzer.set_use_char_model(true);
    if (!analyzer.analyze()) {
        Log.error(__FUNCTION__, "unable to analyze series");
        return std::nullopt;
    }

    int sanity = 0;
    if (!utils::chars_to_number(analyzer.get_result().text, sanity)) [[unlikely]] {
        Log.warn(__FUNCTION__, "Sanity ocr result could not convert to int:", analyzer.get_result().text);
        analyzer.save_img(utils::path("debug") / utils::path("sanity"));
        return std::nullopt;
    }

    return sanity;
}

std::optional<int> asst::FightTimesTaskPlugin::analyze_sanity_cost(const cv::Mat& image)
{
    LogTraceFunction;

    const auto& number_replace = Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map;
    auto task_replace = Task.get<OcrTaskInfo>("StageSanityCost")->replace_map;
    auto merge_map = std::vector(number_replace);
    ranges::copy(task_replace, std::back_inserter(merge_map));

    RegionOCRer analyzer(image);
    analyzer.set_task_info("StageSanityCost");
    analyzer.set_bin_threshold(0, 255);
    analyzer.set_replace(merge_map);

    if (!analyzer.analyze()) [[unlikely]] {
        Log.warn(__FUNCTION__, "Sanity ocr failed");
        analyzer.save_img(utils::path("debug") / utils::path("sanity"));
        return std::nullopt;
    }

    int sanity = 0;
    if (!utils::chars_to_number(analyzer.get_result().text, sanity)) [[unlikely]] {
        Log.warn(__FUNCTION__, "Sanity ocr result could not convert to int:", analyzer.get_result().text);
        analyzer.save_img(utils::path("debug") / utils::path("sanity"));
        return std::nullopt;
    }

    return sanity;
}
