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
    bool equal = false;

    Log.info(m_client_type);
    if (m_client_type == "Official") {
        equal = equal_current_account();
    }
    else if (m_client_type == "Bilibili") {
        equal = equal_current_account_b();
    }

    if (equal) {
        return click_manager_login_button();
    }
    // 展开列表
    show_account_list();

    if (swipe_and_select() || swipe_and_select(true)) {
        json::value info = basic_info_with_what("AccountSwitch");
        // info["details"]["current_account"] = current_account;
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

bool asst::AccountSwitchTask::equal_current_account()
{
    OCRer ocr(ctrler()->get_image());
    ocr.set_task_info("AccountCurrentOCR");
    ocr.set_required({ m_account });
    if (!ocr.analyze()) {
        return false;
    }
    return true;
}

bool asst::AccountSwitchTask::equal_current_account_b()
{
    OCRer ocr(ctrler()->get_image());
    ocr.set_task_info("AccountCurrentOCRBili");
    ocr.set_required({ m_account });
    if (!ocr.analyze()) {
        return false;
    }
    return true;
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
    // 下滑寻找账号
    int repeat = 0;
    bool click = false;
    while (!need_exit()) {
        click = select_account();
        if (click) {
            return click_manager_login_button();
        }
        if (repeat++ > 10) {
            // 没找到对应账号
            return false;
        }
        swipe_account_list(to_top);
    }
    return false;
}

void asst::AccountSwitchTask::swipe_account_list(bool to_top)
{
    ProcessTask(*this, { to_top ? "AccountListSwipeToTop" : "AccountListSwipe" }).run();
}

bool asst::AccountSwitchTask::select_account()
{
    LogTraceFunction;

    sleep(200);
    auto raw_img = ctrler()->get_image();
    OCRer ocr(ctrler()->get_image());
    ocr.set_task_info("AccountListOcr");
    ocr.set_required({ m_account });
    if (!ocr.analyze()) {
        return false;
    }

    asst::Rect roi = ocr.get_result().front().rect;
    m_target_account = ocr.get_result().front().text;
    ctrler()->click(roi);
    return true;
}
