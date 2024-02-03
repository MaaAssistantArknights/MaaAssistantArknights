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

    // 进入投资页面成功
    // 999 >> 1000
    // 取消 | 确认投资

    int count = 0; // 当次已投资的个数
    int retry = 0; // 重试次数
    const auto ocr_current_count = [](const auto& img, const auto& task_name) -> std::optional<int> {
        RegionOCRer ocr(img);
        ocr.set_task_info(task_name);
        if (!ocr.analyze()) {
            Log.error(__FUNCTION__, "unable to analyze current investment count.");
            return std::nullopt;
        };
        int count = 0;
        if (!utils::chars_to_number(ocr.get_result().text, count)) {
            Log.error(__FUNCTION__, "unable to convert current investment count<", ocr.get_result().text,
                      "> to number.");
            return std::nullopt;
        }
        return count;
    };
    // 当前的存款
    auto deposit = ocr_current_count(image, "Roguelike@StageTraderInvest-Count");
    // 可用投资次数
    int count_limit = m_config->get_invest_maximum() - m_invest_count;
    // 投资确认按钮
    const auto& click_rect = Task.get("Roguelike@StageTraderInvest-Confirm")->specific_rect;
    Log.info(__FUNCTION__, "开始投资, 存款", deposit.value_or(-1), ", 可投资次数", count_limit);
    while (!need_exit() && deposit && *deposit < 999 && count_limit > 0 && retry < 3) {
        int times = std::min(15, count_limit - count);
        while (!need_exit() && times > 0) {
            ctrler()->click(click_rect);
            sleep(100);
            times--;
        }
        image = ctrler()->get_image();
        if (!is_investment_available(image)) { // 检查是否处于可投资状态
            break;                             // 投资系统错误 / 没钱了
        }
        else if (auto ocr = ocr_current_count(image, "Roguelike@StageTraderInvest-Count"); ocr) {
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
            Log.error(__FUNCTION__, "未知状态");
            return true;
        }
    }

    if (auto ocr = ocr_current_count(ctrler()->get_image(), "Roguelike@StageTrader-InvestCount"); ocr) {
        // 可继续投资 / 到达投资上限999
        count += *ocr - deposit.value_or(0);
        deposit = *ocr;
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
        m_task_ptr->set_enable(false);
        return true;
    }
    if (deposit.value_or(0) == 999 && m_config->get_invest_stop_when_full()) {
        Log.info(__FUNCTION__, "存款已满");
        stop_roguelike();
        m_task_ptr->set_enable(false);
    }

    return true;
}

bool asst::RoguelikeInvestTaskPlugin::is_investment_available(const cv::Mat& image) const
{
    auto task = ProcessTask(*this, { "Roguelike@StageTraderInvest-Arrow", "Roguelike@StageTraderInvestSystemError" });
    task.set_reusable_image(image).set_retry_times(3);
    task.set_times_limit("Roguelike@StageTraderInvestCancel", 0);
    return task.run() && task.get_last_task_name() == "Roguelike@StageTraderInvest-Arrow";
}

void asst::RoguelikeInvestTaskPlugin::stop_roguelike()
{
    dynamic_cast<ProcessTask*>(m_task_ptr)->set_times_limit("Roguelike@StageTraderInvestConfirm", 0);
}
