#pragma once
#include "Vision/AbstractImageAnalyzer.h"

#include "Common/AsstBattleDef.h"

namespace asst
{
    class BattleImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        enum Target // 需要识别的目标
        {
            None = 0,
            Oper = 1,       // 下方的干员信息
            DetailPage = 2, // 是否点开了详情页
            Kills = 4,      // 击杀数
            Cost = 8,       // 费用
        };

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~BattleImageAnalyzer() override = default;

        bool set_target(int target);
        void set_pre_total_kills(int pre_total_kills);
        virtual bool analyze() override;

        virtual const std::vector<battle::DeploymentOper>& get_opers() const noexcept;
        virtual const std::vector<Rect>& get_homes() const noexcept;

        int get_hp() const noexcept;
        int get_kills() const noexcept;
        int get_total_kills() const noexcept;
        int get_cost() const noexcept;
        bool get_in_detail_page() const noexcept;
        bool get_pause_button() const noexcept;

        void clear() noexcept;
        void sort_opers_by_cost(); // 高费在前，费用降序

    protected:
        bool opers_analyze();       // 识别干员
        bool detail_page_analyze(); // 识别是否在详情页
        battle::Role oper_role_analyze(const Rect& roi);
        bool oper_cooling_analyze(const Rect& roi);
        int oper_cost_analyze(const Rect& roi);
        bool oper_available_analyze(const Rect& roi);

        bool home_analyze(); // 识别蓝色的家门
        // bool skill_analyze();     // 识别技能是否可用
        bool kills_analyze();     // 识别击杀数
        bool cost_analyze();      // 识别费用
        bool vacancies_analyze(); // 识别剩余可部署人数
        bool flag_analyze();
        bool hp_flag_analyze();
        bool kills_flag_analyze();
        bool pause_button_analyze();

        int m_target = 0; // 待识别的目标
        int m_pre_total_kills = 0; // 之前的击杀总数，因为击杀数经常识别不准所以依赖外部传入作为参考

        /* 识别结果缓存 */
        std::vector<battle::DeploymentOper> m_opers; // 下方干员信息
        std::vector<Rect> m_homes;                   // 蓝色的家门位置
        std::vector<Rect> m_ready_skills;            // 可以释放的技能（Rect实际是干员的位置）
        int m_hp = 0;                                // 剩余生命值
        int m_kills = 0;                             // 击杀数
        int m_total_kills = 0;                       // 击杀总数
        int m_cost = 0;                              // 部署费用
        bool m_in_detail_page = false;
        bool m_pause_button = false;
    };
} // namespace asst
