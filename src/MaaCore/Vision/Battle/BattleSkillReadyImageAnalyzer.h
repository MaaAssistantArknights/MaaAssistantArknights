#pragma once

#include "Vision/OnnxHelper.h"

namespace asst
{
    class BattleSkillReadyImageAnalyzer final : public OnnxHelper
    {
    public:
        using OnnxHelper::OnnxHelper;
        virtual ~BattleSkillReadyImageAnalyzer() override = default;

        bool analyze();

        void set_base_point(const Point& pt);
    };
}
