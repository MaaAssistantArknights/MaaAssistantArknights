#pragma once

#include "Vision/OnnxRuntimeImageAnalyzer.h"

namespace asst
{
    class BattleSkillReadyImageAnalyzer final : public OnnxRuntimeImageAnalyzer
    {
    public:
        using OnnxRuntimeImageAnalyzer::OnnxRuntimeImageAnalyzer;
        virtual ~BattleSkillReadyImageAnalyzer() override = default;

        bool analyze();

        void set_base_point(const Point& pt);
    };
}
