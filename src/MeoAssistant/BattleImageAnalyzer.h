#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class BattleImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        enum class Role
        {
            Invaild,
            Caster,
            Medic,
            Pioneer,
            Sniper,
            Special,
            Support,
            Tank,
            Warrior
        };
        struct Oper
        {
            int cost = 0;
            Role role = Role::Invaild;
            bool available = false;
            Rect rect;
        };
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~BattleImageAnalyzer() = default;

        virtual bool analyze() override;

        const std::vector<Oper>& get_opers() const noexcept;
    protected:

        bool opers_analyze();   // 识别干员
        Role oper_role_analyze(const Rect& roi);
        int oper_cost_analyze(const Rect& roi);
        bool oper_available_analyze(const Rect& roi);
        bool home_analyze();    // 识别蓝色的家门
        bool placed_analyze();  // 识别可放置干员的位置

        std::vector<Oper> m_opers;

    private:
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
