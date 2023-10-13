#pragma once
#include "Task/AbstractTask.h"

namespace asst
{
    class SideStoryReopenTask final : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        // SideStoryReopenTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
        virtual ~SideStoryReopenTask() noexcept override = default;

        void set_sidestory_name(std::string sidestory_name);
        void set_medicine(int medicine);
        void set_expiring_medicine(int expiring_medicine);
        void set_stone(int stone);

        bool set_enable_penguid(bool enable);
        bool set_penguin_id(std::string id);
        bool set_server(std::string server);

    private:
        virtual bool _run() override;
        // 判断如果处于普通关页面，直接短路导航
        bool at_normal_page();
        bool navigate_to_normal_page();
        bool select_stage(int stage_index);
        bool activate_prts();
        bool fight(bool use_medicine, bool use_stone);
        void clear();

        // 活动名简称：IC、CW等
        std::string m_sidestory_name;
        int m_medicine = 0;
        int m_expiring_medicine = 0;
        int m_stone = 0;
        // 已使用的理智药数量，不计算临期药品。在每次任务开始前，通过clear()清空
        int m_cur_medicine;
        // 已碎石数量。在每次任务开始前，通过clear()清空
        int m_cur_stone;
        // 理智不够，需要吃药 / 碎石
        bool m_sanity_not_enough = false;
        std::unordered_map<std::string, int> m_drop_stats;
        // 企鹅物流上报用
        bool m_enable_penguid = false;
        std::string m_penguin_id;
        std::string m_server = "CN";
    };

}
