#pragma once
#include "Vision/VisionHelper.h"

#include "Common/AsstBattleDef.h"

namespace asst
{
class BattlefieldMatcher : public VisionHelper
{
public:
    struct ObjectOfInterest
    {
        bool flag = true;
        bool deployment = false;
        bool kills = false;
        bool costs = false;
        // bool in_detail = false;
        bool speed_button = false;
        bool oper_cost = false;
    };

    enum MatchStatus
    {
        Invalid = 0,  // 识别失败
        Success = 1,  // 识别成功
        HitCache = 2, // 图像命中缓存, 不进行识别
    };

    template <typename T>
    struct MatchResult
    {
        T value {};
        MatchStatus status = MatchStatus::Invalid;
    };

    struct Result
    {
        ObjectOfInterest object_of_interest;
        std::vector<battle::DeploymentOper> deployment;
        MatchResult<std::pair<int, int>> kills; // kills / total_kills
        MatchResult<int> costs;

        // bool in_detail = false;
        bool speed_button = false;
        bool pause_button = false;
    };

    using ResultOpt = std::optional<Result>;

public:
    using VisionHelper::VisionHelper;
    virtual ~BattlefieldMatcher() override = default;

    void set_object_of_interest(ObjectOfInterest obj);
    void set_total_kills_prompt(int prompt);
    void set_image_prev(const cv::Mat& image);
    ResultOpt analyze() const;

protected:
    bool hp_flag_analyze() const;
    bool kills_flag_analyze() const;
    bool pause_button_analyze() const;

    std::vector<battle::DeploymentOper> deployment_analyze() const; // 识别干员
    battle::Role oper_role_analyze(const Rect& roi) const;
    bool oper_cooling_analyze(const Rect& roi) const;
    int oper_cost_analyze(const Rect& roi) const;
    bool oper_available_analyze(const Rect& roi) const;

    MatchResult<std::pair<int, int>> kills_analyze() const; // 识别击杀数
    // 识别击杀数是否命中缓存
    bool hit_kills_cache(Rect flag_rect) const;
    bool cost_symbol_analyze() const;       // 识别费用左侧图标
    MatchResult<int> costs_analyze() const; // 识别费用
    // 识别持有费用是否命中缓存
    bool hit_costs_cache() const;
    bool in_detail_analyze() const;        // 识别是否在详情页
    bool speed_button_analyze() const;     // 识别是否有加速按钮（在详情页就没有）

    ObjectOfInterest m_object_of_interest; // 待识别的目标
    int m_total_kills_prompt = 0;          // 之前的击杀总数，因为击杀数经常识别不准所以依赖外部传入作为参考
    cv::Mat m_image_prev;                  // 缓存图像, 用于判断费用, 击杀数是否变化. 无变化则不重新识别
};
} // namespace asst
