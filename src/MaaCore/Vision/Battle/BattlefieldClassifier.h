#pragma once

#include <cstddef>
#include <deque>
#include <filesystem>
#include <string>

#include "Common/AsstBattleDef.h"
#include "Vision/OnnxHelper.h"
#include "Vision/VisionHelper.h"

namespace asst
{
class BattlefieldClassifier final : public VisionHelper, public OnnxHelper
{
public:
    struct ObjectOfInterest
    {
        bool skill_ready = false;
        bool deploy_direction = false;
    };

public:
    struct SkillReadyResult
    {
        static constexpr size_t ClsSize = 3;
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

        SkillReadyResult skill_ready;
        DeployDirectionResult deploy_direction;
    };

    using ResultOpt = std::optional<Result>;

public:
    using VisionHelper::VisionHelper;
    virtual ~BattlefieldClassifier() override = default;

    void set_object_of_interest(ObjectOfInterest obj) { m_object_of_interest = obj; }

    void set_base_point(const Point& pt) { m_base_point = pt; }

    ResultOpt analyze() const;

protected:
    SkillReadyResult skill_ready_analyze() const;
    DeployDirectionResult deploy_direction_analyze() const;

    static constexpr std::size_t SkillReadyAutoCleanLimit = 50;

    struct SkillReadyFileQueue
    {
        std::deque<std::filesystem::path> files;
        bool initialized = false;
    };

    static void init_skill_ready_file_queue_locked(const std::filesystem::path& dir, SkillReadyFileQueue& file_queue);

    static bool save_skill_ready_debug_image(
        const cv::Mat& image,
        const std::string& subfolder,
        const std::string& filename,
        bool auto_clean);

    ObjectOfInterest m_object_of_interest; // 待识别的目标
    Point m_base_point;
};
} // namespace asst
