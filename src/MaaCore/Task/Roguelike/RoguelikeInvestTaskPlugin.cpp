#include "RoguelikeInvestTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/RegionOCRer.h"

bool asst::RoguelikeInvestTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const auto task_name = details.get("details", "task", "");
    if (task_name.ends_with("Roguelike@StageTraderInvestConfirm")) {
        return m_invest_count < m_config->get_invest_maximum();
    }
    else {
        return false;
    }
}

bool asst::RoguelikeInvestTaskPlugin::_run()
{
    LogTraceFunction;

    auto image = ctrler()->get_image();

    int count = 0;                                                                // 当次已投资的个数
    int retry = 0;                                                                // 重试次数
    auto deposit = ocr_current_count(image, "Roguelike@StageTraderInvest-Count"); // 当前的存款
    int count_limit = m_config->get_invest_maximum() - m_invest_count;            // 可用投资次数
    // 投资确认按钮
    const auto& click_rect = Task.get("Roguelike@StageTraderInvest-Confirm")->specific_rect;
    Log.info(__FUNCTION__, "开始投资, 存款", deposit.value_or(-1), ", 可投资次数", count_limit);
    while (!need_exit() && deposit && *deposit < 999 && count_limit - count > 0 && retry < 3) {
        int times = std::min(15, count_limit - count);
        while (!need_exit() && times > 0) {
            ctrler()->click(click_rect);
            sleep(100);
            times--;
        }
        if (count_limit - count < 5) {
            sleep(500);
        }
        image = ctrler()->get_image();
        if (is_investment_available(image)) { // 检查是否处于可投资状态
            if (auto ocr = ocr_current_count(image, "Roguelike@StageTraderInvest-Count"); ocr) {
                // 可继续投资 / 到达投资上限999
                if (*ocr == *deposit || *ocr > 999 || *ocr < 1) {
                    retry++; // 可能是出错了，重试三次放弃
                }
                else {
                    count += *ocr - *deposit;
                    deposit = *ocr;
                    retry = 0;
                }
            }
            else {
                Log.error(__FUNCTION__, "无法获取可投资状态下的存款");
                save_img(utils::path("debug") / utils::path("roguelike") / utils::path("invest_system"));
                retry++;
            }
        }
        else if (is_investment_error(image)) {
            Log.info(__FUNCTION__, "投资系统错误, 退出投资");

            sleep(500); // 此处UI有一个从左往右的移动，等待后重新截图，防止UI错位
            if (auto ocr = ocr_current_count(ctrler()->get_image(), "Roguelike@StageTraderInvestError-Count"); ocr) {
                // 可继续投资 / 到达投资上限999
                count += *ocr - deposit.value_or(0);
                deposit = *ocr;
            }
            else {
                Log.error(__FUNCTION__, "无法获取错误状态下的存款");
                save_img(utils::path("debug") / utils::path("roguelike") / utils::path("invest_system"));
            }

            break;
        }
        else {
            Log.error(__FUNCTION__, "未知状态，投资中止");
            return false;
        }
    }

    const auto total = count + m_invest_count;

    auto info = basic_info_with_what("RoguelikeInvestment");
    info["details"]["count"] = count;
    info["details"]["total"] = total;
    info["details"]["deposit"] = deposit ? *deposit : -1;
    callback(AsstMsg::SubTaskExtraInfo, info);

    Log.info(__FUNCTION__, "本轮投资结束, 投资", count, ", 共投资", total, "; 系统余额", deposit ? *deposit : -1);
    m_invest_count = total;

    if (count_limit - count <= 0) {
        Log.info(__FUNCTION__, "投资达到设置上限,", m_config->get_invest_maximum());
        stop_roguelike();
        return true;
    }
    if (deposit.value_or(0) == 999 && m_config->get_invest_stop_when_full()) {
        Log.info(__FUNCTION__, "存款已满");
        stop_roguelike();
        return true;
    }

    return true;
}

std::optional<int> asst::RoguelikeInvestTaskPlugin::ocr_current_count(const auto& img, const auto& task_name)
{
    const auto& number_replace = Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map;
    auto task_replace = Task.get<OcrTaskInfo>(task_name)->replace_map;
    auto merge_map = std::vector(number_replace);
    ranges::copy(task_replace, std::back_inserter(merge_map));

    RegionOCRer ocr(img);
    ocr.set_task_info(task_name);
    ocr.set_use_raw(false);
    ocr.set_replace(merge_map);
    if (!ocr.analyze()) {
        Log.error(__FUNCTION__, "unable to analyze current investment count.");
        return std::nullopt;
    };
    int count = 0;
    if (!utils::chars_to_number(ocr.get_result().text, count)) {
        Log.error(__FUNCTION__, "unable to convert current investment count<", ocr.get_result().text, "> to number.");
        return std::nullopt;
    }
    return count;
}

bool asst::RoguelikeInvestTaskPlugin::is_investment_available(const cv::Mat& image) const
{
    auto task = ProcessTask(*this, { "Roguelike@StageTraderInvest-Arrow" });
    task.set_reusable_image(image).set_retry_times(0);
    return task.run();
}

bool asst::RoguelikeInvestTaskPlugin::is_investment_error(const cv::Mat& image) const
{
    auto task = ProcessTask(*this, { "Roguelike@StageTraderInvestSystemError" });
    task.set_reusable_image(image).set_retry_times(0);
    task.set_times_limit("Roguelike@StageTraderInvestSystemError", 0);
    return task.run();
}

void asst::RoguelikeInvestTaskPlugin::stop_roguelike()
{
    ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ExitThenAbandon" })
        .set_times_limit("Roguelike@StartExplore", 0)
        //.set_times_limit("Roguelike@Abandon", 0)
        .set_retry_times(5)
        .run();
    m_task_ptr->set_enable(false);
}
