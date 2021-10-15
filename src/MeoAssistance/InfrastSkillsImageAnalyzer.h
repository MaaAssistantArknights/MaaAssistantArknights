#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst {
    class InfrastSkillsImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastSkillsImageAnalyzer() = default;
        virtual bool analyze() override;
    protected:
        bool skills_roi_analyze(const Rect& roi);
    };
}