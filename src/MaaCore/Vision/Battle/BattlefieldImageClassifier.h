#pragma once

#include "Common/AsstBattleDef.h"
#include "Vision/OnnxRuntimeImageAnalyzer.h"

namespace asst
{
    class BattlefieldImageClassifier final : public OnnxRuntimeImageAnalyzer
    {
    public:
        struct ObjectOfInterest
        {
            bool skill_ready = true;
            bool deploy_direction = false;
        };

    public:
        struct SkillReadyResult
        {
            static constexpr size_t ClsSize = 2;
            using Raw = std::array<float, ClsSize>;
            using Prob = Raw;

            bool ready = false;
            Rect rect;
            float score = .0f;
            Raw raw;
            Prob prob; // after softmax

            Point base_point;
        };

        struct DeployDirectionResult
        {
            static constexpr size_t ClsSize = 4;
            using Raw = std::array<float, ClsSize>;
            using Prob = Raw;

            battle::DeployDirection direction = battle::DeployDirection::None;
            Rect rect;
            float score = .0f;
            Raw raw;
            Prob prob; // after softmax

            Point base_point;
        };

        struct Result
        {
            ObjectOfInterest object_of_interest;

            std::optional<SkillReadyResult> skill_ready;
            std::optional<DeployDirectionResult> deploy_direction;
        };

        using ResultOpt = std::optional<Result>;

    public:
        using OnnxRuntimeImageAnalyzer::OnnxRuntimeImageAnalyzer;
        virtual ~BattlefieldImageClassifier() override = default;

        void set_object_to_analyze(ObjectOfInterest obj) { m_object_of_interest = obj; }
        void set_base_point(const Point& pt) { m_base_point = pt; }

        const ResultOpt& analyze();
        const ResultOpt& result() const noexcept { return m_result; }

    protected:
        SkillReadyResult skill_ready_analyze();
        DeployDirectionResult deploy_direction_analyze();

        ObjectOfInterest m_object_of_interest; // 待识别的目标
        Point m_base_point;
    };
} // namespace asst
