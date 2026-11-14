#pragma once
#include "Task/AbstractTask.h"

#include <chrono>
#include <vector>

namespace asst
{
class AccountSwitchTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~AccountSwitchTask() noexcept override = default;

    void set_account(std::string account) { m_account = std::move(account); }

    void set_client_type(std::string client_type) { m_client_type = std::move(client_type); }

    void set_recognition_timeout(std::chrono::milliseconds timeout) { m_recognition_timeout = timeout; }

private:
    virtual bool _run() override;

    // 导航至账号管理页
    bool navigate_to_start_page();
    bool equal_current_account();
    bool equal_current_account_b();
    // 账号列表里面点登录
    bool click_manager_login_button();
    // 打开账号列表
    bool show_account_list();
    bool swipe_and_select(bool to_top = false);
    // 往下滑账号列表
    void swipe_account_list(bool to_top = false);
    // 识别并选择m_account
    bool select_account();

    std::string m_account;
    std::string m_target_account;
    std::string m_client_type; // 客户端类型
    std::chrono::milliseconds m_recognition_timeout { 0 };
    const std::vector<std::string> SupportedClientType = { "Official",
                                                           "Bilibili",
                                                           "txwy",
                                                           "YoStarKR" /*, "YoStarEN", "YoStarJP" */ };
};
}
