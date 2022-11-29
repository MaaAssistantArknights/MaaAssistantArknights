#pragma once
#include "Vision/AbstractImageAnalyzer.h"

#include "Common/AsstInfrastDef.h"

namespace asst
{
    class InfrastSmileyImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastSmileyImageAnalyzer() override = default;

        virtual bool analyze() override;

        auto get_result() const noexcept -> const std::vector<infrast::Smiley>& { return m_result; }

    protected:
        std::vector<infrast::Smiley> m_result;
    };
}
