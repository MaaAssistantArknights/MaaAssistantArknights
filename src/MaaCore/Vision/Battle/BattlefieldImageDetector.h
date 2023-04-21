#pragma once

#include "Vision/OnnxRuntimeImageAnalyzer.h"

namespace asst
{
    class BattlefieldImageDetector final : public OnnxRuntimeImageAnalyzer
    {
    public:
        struct ObjectOfInterest
        {
            bool operators = true;
        };

    public:
        // 战斗中干员（血条）检测，yolov8 模型
        struct OperatorResult
        {
            enum class Cls
            {
                Operator = 0,
            };

            Cls cls = Cls::Operator;
            Rect rect;
            float score = .0f;

#ifdef ASST_DEBUG
            Rect draw_rect;
#endif
        };

        struct Result
        {
            ObjectOfInterest object_of_interest;

            std::vector<OperatorResult> operators;
        };

        using ResultOpt = std::optional<Result>;

    public:
        using OnnxRuntimeImageAnalyzer::OnnxRuntimeImageAnalyzer;
        virtual ~BattlefieldImageDetector() override = default;

        const ResultOpt& analyze();
        const ResultOpt& result() const noexcept { return m_result; }

    protected:
        std::vector<OperatorResult> operator_analyze();

        ObjectOfInterest m_object_of_interest;

        ResultOpt m_result;
    };
}
