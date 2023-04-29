#pragma once

#include "Vision/VisionHelper.h"

namespace asst
{
    class BattleFormationImageAnalyzer : public VisionHelper
    {
    public:
        using VisionHelper::VisionHelper;
        virtual ~BattleFormationImageAnalyzer() override = default;

        bool analyze();

        const auto& get_result() const { return m_result; }

    private:
        std::unordered_map<std::string, cv::Mat> m_result;
    };
}
