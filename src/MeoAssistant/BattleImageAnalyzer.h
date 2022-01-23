#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class BattleImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        enum class Role
        {
            Unknown,
            Caster,
            Medic,
            Pioneer,
            Sniper,
            Special,
            Support,
            Tank,
            Warrior,
            Drone
        };
        struct Oper
        {
            int cost = 0;
            Role role = Role::Unknown;
            bool available = false;
            Rect rect;
        };
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~BattleImageAnalyzer() = default;

        virtual bool analyze() override;

        virtual const std::vector<Oper>& get_opers() const noexcept;
        virtual const std::vector<Rect>& get_homes() const noexcept;

        const std::vector<Rect>& get_ready_skills() const noexcept;
        int get_hp() const noexcept;
    protected:

        bool opers_analyze();   // 识别干员
        Role oper_role_analyze(const Rect& roi);
        int oper_cost_analyze(const Rect& roi);
        bool oper_available_analyze(const Rect& roi);

        bool home_analyze();    // 识别蓝色的家门
        bool skill_analyze();   // 识别技能是否可用
        bool hp_analyze();      // 识别剩余生命值

        std::vector<Oper> m_opers;   // 下方干员信息
        std::vector<Rect> m_homes;   // 蓝色的家门位置
        std::vector<Rect> m_ready_skills;   // 可以释放的技能（Rect实际是干员的位置）
        int m_hp;                    // 剩余生命值

    protected:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override
        {
            AbstractImageAnalyzer::set_roi(roi);
        }
        virtual void set_image(const cv::Mat image, const Rect& roi)
        {
            AbstractImageAnalyzer::set_image(image, roi);
        }
    };
}
