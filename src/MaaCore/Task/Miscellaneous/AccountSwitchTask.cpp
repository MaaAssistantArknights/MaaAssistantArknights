#include "AccountSwitchTask.h"

#include <meojson/json.hpp>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::AccountSwitchTask::_run()
{
    LogTraceFunction;

    if (need_exit()) {
        return false;
    }

    if (m_account.empty()) {
        Log.error(__FUNCTION__, "account is empty");
        return false;
    }

    if (ranges::find(SupportedClientType, m_client_type) == SupportedClientType.end()) {
        Log.error(__FUNCTION__, "unsupported client");
        return false;
    }

    // 退出到选择账号界面
    if (!navigate_to_start_page()) {
        return false;
    }
    // 当前账号就是想要的
    std::string current_account;

    if (m_client_type == "Official") {
        current_account = get_current_account();
    }
    else if (m_client_type == "Bilibili") {
        current_account = get_current_account_b();
    }

    if (current_account == m_account) {
        return click_manager_login_button();
    }
    // 展开列表
    show_account_list();

    if (swipe_and_select() || swipe_and_select(true)) {
        json::value info = basic_info_with_what("AccountSwitch");
        info["details"]["current_account"] = current_account;
        info["details"]["account_name"] = m_target_account;
        callback(AsstMsg::SubTaskExtraInfo, info);

        return true;
    }
    else {
        return false;
    }
}

bool asst::AccountSwitchTask::navigate_to_start_page()
{
    auto task = ProcessTask(*this, { "SwitchAccount@StartUpBegin" });
    task.set_retry_times(30);
    task.run();
    std::string last_name = task.get_last_task_name();
    if (last_name == "LoginOther") {
        return true;
    }
    else if (last_name == "AccountManagerOfficial") {
        return true;
    }
    else if (last_name == "AccountManagerBili") {
        return true;
    }
    return false;
}

std::string asst::AccountSwitchTask::get_current_account()
{
    RegionOCRer ocr(ctrler()->get_image());
    ocr.set_task_info("AccountCurrentOCR");
    if (!ocr.analyze()) {
        Log.info(__FUNCTION__, "current account ocr analyze failed");
        return std::string();
    }
    return ocr.get_result().text;
}

std::string asst::AccountSwitchTask::get_current_account_b()
{
    RegionOCRer ocr(ctrler()->get_image());
    ocr.set_task_info("AccountCurrentOCRBili");
    if (!ocr.analyze()) {
        Log.info(__FUNCTION__, "current account ocr analyze failed");
        return std::string();
    }
    return ocr.get_result().text;
}

bool asst::AccountSwitchTask::click_manager_login_button()
{
    return ProcessTask(*this, { "AccountManagerLoginButton", "AccountManagerLoginButtonBili" })
        .set_retry_times(3)
        .run();
}

bool asst::AccountSwitchTask::show_account_list()
{
    return ProcessTask(*this, { "AccountManagerListAccount", "AccountManagerListAccountBili" }).run();
}

bool asst::AccountSwitchTask::swipe_and_select(bool to_top)
{
    if (need_exit()) {
        return false;
    }
    // 下滑寻找账号，垃圾桶定位
    std::vector<std::string> pre_result, result;
    int repeat = 0;
    bool click = false;
    while (!need_exit()) {
        if (m_client_type == "Official") {
            click = select_account(result);
        }
        else if (m_client_type == "Bilibili") {
            click = select_account_b(result);
        }
        if (click) {
            return click_manager_login_button();
        }
        if (pre_result == result) {
            if (repeat++ > 3) {
                // 没找到对应账号
                return false;
            }
        }
        else {
            repeat = 0;
        }
        pre_result = result;
        result.clear();
        swipe_account_list(to_top);
    }
    return false;
}

void asst::AccountSwitchTask::swipe_account_list(bool to_top)
{
    ProcessTask(*this, { to_top ? "AccountListSwipeToTop" : "AccountListSwipe" }).run();
}

bool asst::AccountSwitchTask::select_account(std::vector<std::string>& account_list)
{
    LogTraceFunction;

    sleep(200);
    auto raw_img = ctrler()->get_image();
    MultiMatcher matcher(raw_img);
    matcher.set_task_info("AccountManagerTrashButton");
    if (!matcher.analyze()) {
        Log.error(__FUNCTION__, "unable to match trash button in account list");
        return false;
    }
    for (const auto& res : matcher.get_result()) {
        asst::Rect roi = { res.rect.x - 308, res.rect.y - 14, 124, 25 };

        RegionOCRer ocr(raw_img);
        ocr.set_task_info("AccountListOcr");
        ocr.set_roi(roi);
        if (!ocr.analyze()) {
            Log.error(__FUNCTION__, "unable to analyze account");
            continue;
        }
        std::string text = ocr.get_result().text;
        if (text.find(m_account) != std::string::npos) {
            m_target_account = std::move(text);
            ctrler()->click(roi);
            return true;
        }
        account_list.emplace_back(std::move(text));
    }
    return false;
}

bool asst::AccountSwitchTask::select_account_b(std::vector<std::string>& account_list)
{
    LogTraceFunction;

    auto raw_img = ctrler()->get_image();
    MultiMatcher matcher(raw_img);
    matcher.set_task_info("AccountManagerTrashButtonBili");
    if (!matcher.analyze()) {
        Log.error(__FUNCTION__, "unable to match trash button in account list");
        return false;
    }
    for (const auto& res : matcher.get_result()) {
        asst::Rect roi = { res.rect.x - 238, res.rect.y - 18, 150, 30 };
        RegionOCRer ocr(raw_img);
        ocr.set_roi(roi);
        if (!ocr.analyze()) {
            Log.error(__FUNCTION__, "unable to analyze account");
            continue;
        }
        std::string text = ocr.get_result().text;
        if (text.find(m_account) != std::string::npos) {
            m_target_account = std::move(text);
            ctrler()->click(roi);
            return true;
        }
        account_list.emplace_back(std::move(text));
    }
    return false;
}
