#pragma once

#include "Vision/OnnxRuntimeImageAnalyzer.h"

namespace asst
{
    class BattleDeployDirectionImageAnalyzer final : public OnnxRuntimeImageAnalyzer
    {
    public:
        using OnnxRuntimeImageAnalyzer::OnnxRuntimeImageAnalyzer;
        virtual ~BattleDeployDirectionImageAnalyzer() override = default;

        virtual bool analyze() override;

        size_t get_class_id() const { return m_class_id; }

        void set_base_point(const Point& pt);

    private:
        size_t m_class_id = 0;
    };
}
