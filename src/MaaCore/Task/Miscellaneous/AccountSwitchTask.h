#pragma once
#include "Task/AbstractTask.h"

namespace asst
{
    class AccountSwitchTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~AccountSwitchTask() noexcept override = default;
        void set_account(std::string account);

    private:
        virtual bool _run() override;

        // 导航至账号管理页
        bool navigate_to_start_page();
        std::string get_current_account();
        std::string get_current_account_b();
        // 判断游戏服务器
        bool verify_client_type();
        // 账号列表里面点登录
        bool click_manager_login_button();
        // 打开账号列表
        bool show_account_list();
        bool swipe_and_select(bool to_top = false);
        // 往下滑账号列表
        void swipe_account_list(bool to_top = false);
        // 识别并选择m_account
        bool select_account(std::vector<std::string>& account_list);
        // 识别并选择m_account
        bool select_account_b(std::vector<std::string>& account_list);

        bool is_official = false, is_bili = false;
        std::string m_account;
        std::string m_target_account;
    };
}
