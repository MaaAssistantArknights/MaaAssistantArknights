#pragma once
#include "Task/AbstractTask.h"

namespace asst
{
    enum class ReclamationTaskMode
    {
        GiveupUponFight,
        SmeltGold
    };

    class ReclamationControlTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~ReclamationControlTask() override = default;

        void set_task_mode(const ReclamationTaskMode& mode) { m_task_mode = mode; }

    private:
        virtual bool _run() override;
        bool run_giveup_upon_fight_procedure();
        bool run_smelt_gold_procedure();

        bool navigate_to_reclamation_home();   // 导航至生息演算主界面
        bool give_up_last_algorithm_if();      // 如果有，放弃上次演算
        bool start_with_default_formation();   // 开始演算，至第一天
        bool skip_announce_report();           // 跳过每日公告
        bool start_action_enter();             // 区域交互确认
        bool battle_default_formation_start(); // 默认战斗编队并开始战斗
        bool level_complete_confirm();         // 场景退出结算，至地图界面
        bool enter_next_day_if_useup();        // 如果决断点耗尽，进入下一天
        bool wait_between_day();               // 等待日期转换动画结束
        bool reset_scope();                    // 重置地图大小
        bool enter_command_center();           // 进入侧边栏的命令中心
        bool do_manufacture();                 // 点击制作按钮

        bool check_next_day();          // 决断点消耗完进入下一天的标志
        bool check_emergency();         // 敌袭到达
        int check_manufacture_status(); // 制造界面是否材料充足，0不足，1充足，-1非建造页面

        bool click_center_base();         // 点击中心基地图标
        bool click_corner_black_market(); // 点击左上角黑市图标
        bool click_any_zone();            // 点击奇遇外任意区
        bool click_black_market();        // 点击黑市

        bool swipe_right();
        bool swipe_left();

        void procedure_start_callback(int times);
        void smelt_gold_callback(int times);

        ReclamationTaskMode m_task_mode = ReclamationTaskMode::GiveupUponFight;
        int m_total_gold = 0; // 制作赤金的次数
    };
}
