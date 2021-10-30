#pragma once
#include "AbstractImageAnalyzer.h"

#include "AsstDef.h"

namespace asst {
    class InfrastSmileyImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastSmileyImageAnalyzer() = default;

        virtual bool analyze() override;

        const std::vector<InfrastSmileyInfo>& get_result() const noexcept {
            return m_result;
        }
    protected:
        std::vector<InfrastSmileyInfo> m_result;
    };
}
