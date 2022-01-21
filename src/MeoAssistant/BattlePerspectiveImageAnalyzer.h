#pragma once
#include "BattleImageAnalyzer.h"

namespace asst
{
    // 战斗透视图片，一般是点击干员后的画面
    class BattlePerspectiveImageAnalyzer : public BattleImageAnalyzer
    {
    public:
        using BattleImageAnalyzer::BattleImageAnalyzer;
        virtual ~BattlePerspectiveImageAnalyzer() = default;

        virtual bool analyze() override;
        virtual const std::vector<Rect>& get_homes() const noexcept override;

        // 设置非透视状态下蓝色家门的位置
        void set_src_homes(std::vector<Rect> src_homes);
        Point get_nearest_point() const noexcept;
    protected:
        bool placed_analyze();  // 识别可放置干员的位置

        std::vector<Rect> m_src_homes;
        std::vector<Rect> m_available_placed;
        Point m_nearest_point;
    };
}
