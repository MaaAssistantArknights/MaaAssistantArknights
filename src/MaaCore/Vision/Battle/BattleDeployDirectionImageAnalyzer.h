#pragma once

#include "Vision/OnnxRuntimeImageAnalyzer.h"

namespace asst
{
    class BattleDeployDirectionImageAnalyzer final : public OnnxRuntimeImageAnalyzer
    {
    public:
        static constexpr size_t ClassificationSize = 4;
        using RawResults = std::array<float, ClassificationSize>;

    public:
        using OnnxRuntimeImageAnalyzer::OnnxRuntimeImageAnalyzer;
        virtual ~BattleDeployDirectionImageAnalyzer() override = default;

        void set_base_point(const Point& pt);
        virtual bool analyze() override;

        size_t get_class_id() const { return m_class_id; }
        RawResults get_raw_results() const { return m_raw_results; }

    private:
        std::array<float, ClassificationSize> m_raw_results;
        size_t m_class_id = 0;
    };
}
