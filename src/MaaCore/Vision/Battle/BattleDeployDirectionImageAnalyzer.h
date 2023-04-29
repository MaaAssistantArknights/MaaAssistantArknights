#pragma once

#include "Vision/OnnxHelper.h"

namespace asst
{
    class BattleDeployDirectionImageAnalyzer final : public OnnxHelper
    {
    public:
        static constexpr size_t ClassificationSize = 4;
        using RawResults = std::array<float, ClassificationSize>;

    public:
        using OnnxHelper::OnnxHelper;
        virtual ~BattleDeployDirectionImageAnalyzer() override = default;

        void set_base_point(const Point& pt);
        bool analyze();

        size_t get_class_id() const { return m_class_id; }
        RawResults get_raw_results() const { return m_raw_results; }

    private:
        std::array<float, ClassificationSize> m_raw_results;
        size_t m_class_id = 0;
    };
}
