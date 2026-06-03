#include "MultiCopilotTaskPlugin.h"

#include <ranges>

#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/PipelineAnalyzer.h"

bool asst::MultiCopilotTaskPlugin::_run()
{
    LogTraceFunction;
    if (m_copilot_configs.size() < (size_t)m_index_current) {
        LogError << __FUNCTION__ << "configs size:" << m_copilot_configs.size() << ", current index:" << m_index_current
                 << ", out of range";
        return false;
    }

    const auto& config = m_copilot_configs[m_index_current++];

    std::string file_name;
    if (!Copilot.load(config.copilot_file)) {
        Log.error("CopilotConfig parse failed");
        return false;
    }
    file_name = utils::path_to_utf8_string(config.copilot_file);

    const auto& stage_name = Copilot.get_stage_name();
    if (!m_battle_task_ptr->set_stage_name(stage_name)) {
        Log.error("Not support stage");
        return false;
    }

    json::value info = basic_info_with_what("CopilotListLoadTaskFileSuccess");
    info["details"]["stage_name"] = Copilot.get_stage_name();
    info["details"]["file_name"] = std::move(file_name);
    callback(AsstMsg::SubTaskExtraInfo, info);

    bool ret = true;
    ret = ret && navigate_to_stage(config.nav_name);

    ProcessTask(*this, { "NotUsePrts" }).set_ignore_error(true).set_retry_times(0).run();
    if (config.is_raid) {
        // 选择突袭模式
        ret = ret && ProcessTask(*this, { "RaidConfirm", "ChangeToRaidDifficulty" }).set_retry_times(20).run();
    }

    return ret;
}

bool asst::MultiCopilotTaskPlugin::navigate_to_stage(const std::string& stage_name)
{
    m_status = NavigationStatus::Init;
    auto image = ctrler()->get_image();

    if (is_stage_detail_opened(image)) { // 关卡介绍已展开
        m_status = NavigationStatus::StageClicked;
        return confirm_stage_name(image, stage_name);
    }

    const auto& task = Task.get<OcrTaskInfo>(stage_name + "@ClickStageName");
    auto stages = find_stage(image, task->bin_threshold[0], task->bin_threshold[1]);
    auto it = std::ranges::find_if(stages, [&](const OcrPack::Result& r) { return r.text == stage_name; });
    if (it != stages.end()) {
        if (enter_stage(it->rect, stage_name)) {
            return true;
        }
    }

    ProcessTask(*this, { "Copilot@FullStageNavigation" }).set_retry_times(20).run();
    sleep(Config.get_options().task_delay);
    image = ctrler()->get_image();
    stages = find_stage(image, task->bin_threshold[0], task->bin_threshold[1]);
    it = std::ranges::find_if(stages, [&](const OcrPack::Result& r) { return r.text == stage_name; });
    if (it != stages.end()) {
        if (enter_stage(it->rect, stage_name)) {
            return true;
        }
    }

    for (int i = 0; i < m_max_retry; ++i) {
        ProcessTask(*this, { "Copilot@StageNavigationSlowlySwipeLeft" }).set_retry_times(20).run();
        sleep(Config.get_options().task_delay);
        image = ctrler()->get_image();
        stages = find_stage(image, task->bin_threshold[0], task->bin_threshold[1]);
        it = std::ranges::find_if(stages, [&](const OcrPack::Result& r) { return r.text == stage_name; });
        if (it != stages.end()) {
            if (enter_stage(it->rect, stage_name)) {
                return true;
            }
        }
    }

    // 划 10 次到最右，然后扫有无初见剧情
    auto plot_task = ProcessTask(*this, { "Copilot@ChapterSwipeToTheRightAndPlot" });
    plot_task.set_retry_times(20);
    if (plot_task.run()) {
        sleep(Config.get_options().task_delay);
        image = ctrler()->get_image();
        stages = find_stage(image, task->bin_threshold[0], task->bin_threshold[1]);
        it = std::ranges::find_if(stages, [&](const OcrPack::Result& r) { return r.text == stage_name; });
        if (it != stages.end()) {
            if (enter_stage(it->rect, stage_name)) {
                return true;
            }
        }
    }

    return false;
}

bool asst::MultiCopilotTaskPlugin::enter_stage(const Rect rect, const std::string& stage_name)
{
    ctrler()->click(rect);
    sleep(Config.get_options().task_delay);
    auto image_entered = ctrler()->get_image();
    if (is_stage_detail_opened(image_entered)) { // 关卡介绍已展开
        m_status = NavigationStatus::StageClicked;
        sleep(Config.get_options().task_delay);
        return confirm_stage_name(image_entered, stage_name);
    }

    return false;
}

asst::OCRer::ResultsVec
    asst::MultiCopilotTaskPlugin::find_stage(const cv::Mat& image, int threshold_low, int threshold_high)
{
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::inRange(gray, threshold_low, threshold_high, gray);
    cv::dilate(gray, gray, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(20, 8)), cv::Point(-1, -1), 1);
    std::vector<cv::Mat> channels = { gray, gray, gray };
    cv::Mat gray3;
    cv::merge(channels, gray3);
    cv::bitwise_and(image, gray3, image);
    OCRer ocr(image);
    ocr.set_task_info("ClickStageName");
    if (!ocr.analyze()) {
        return {};
    }
    auto result = ocr.get_result();
    std::erase_if(result, [](const OcrPack::Result& r) { return r.text.size() == 1 || r.score < 0.5; });
    LogInfo << __FUNCTION__ << "stage results:" << result;
    return result;
}

bool asst::MultiCopilotTaskPlugin::is_stage_detail_opened(const cv::Mat& image)
{
    PipelineAnalyzer match(image);
    match.set_tasks({ "ClickedCorrectStageOrSwipe" });
    return match.analyze().has_value();
}

bool asst::MultiCopilotTaskPlugin::confirm_stage_name(const cv::Mat& image, const std::string& stage_name)
{
    const auto ocr_check = [&](const OCRer::ResultsVecOpt& ret_opt) {
        return ret_opt.has_value() &&
               std::ranges::any_of(ret_opt.value(), [&](const OcrPack::Result& r) { return r.text == stage_name; });
    };
    OCRer ocr(image);
    ocr.set_task_info("ClickStageName");
    if (ocr_check(ocr.analyze())) {
        m_status = NavigationStatus::StageConfirmed;
        return true;
    }

    for (int i = 0; i < m_max_retry; ++i) {
        sleep(Config.get_options().task_delay);
        OCRer re_OCR(ctrler()->get_image());
        re_OCR.set_task_info("ClickStageName");
        if (ocr_check(re_OCR.analyze())) {
            m_status = NavigationStatus::StageConfirmed;
            return true;
        }
    }
    LogError << __FUNCTION__ << "confirm stage name failed after retrying " << m_max_retry
             << " times, stage name:" << stage_name;
    return false;
}

