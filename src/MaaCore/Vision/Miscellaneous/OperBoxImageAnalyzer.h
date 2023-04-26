#pragma once

#include "Vision/AbstractImageAnalyzer.h"

namespace asst
{
    struct OperBoxInfo
    {
        std::string id;
        std::string name;
        int level = 0;     // 等级
        int elite = 0;     // 精英度
        int potential = 0; // 潜能
        int rarity = 0;    // 稀有度

        Rect rect;
        bool own = false;
    };
    class OperBoxImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~OperBoxImageAnalyzer() override = default;
        virtual bool analyze() override;
        const auto& get_result() const noexcept { return m_result; }

    private:
        int level_num(const std::string& level);
        bool analyzer_oper_box();
        // 获取lv rect
        bool opers_analyze();
        bool level_analyze();
        bool elite_analyze();
        bool potential_analyze();
        // 按位置排序
        // 1 - 2 - 3
        // 4 - 5 - 6
        void sort_();

        std::vector<asst::OperBoxInfo> m_result;
    };
}
