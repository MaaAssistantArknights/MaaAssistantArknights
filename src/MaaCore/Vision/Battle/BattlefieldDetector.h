#pragma once

#include "Vision/OnnxHelper.h"
#include "Vision/VisionHelper.h"

MAA_VISION_NS_BEGIN

class BattlefieldDetector final : public VisionHelper, public OnnxHelper
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
    using VisionHelper::VisionHelper;
    virtual ~BattlefieldDetector() override = default;

    ResultOpt analyze() const;

protected:
    std::vector<OperatorResult> operator_analyze() const;

    ObjectOfInterest m_object_of_interest;
};

MAA_VISION_NS_END
