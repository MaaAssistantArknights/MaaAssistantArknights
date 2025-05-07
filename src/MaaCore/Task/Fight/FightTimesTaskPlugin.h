#pragma once
#include "Common/AsstTypes.h"
#include "Task/AbstractTaskPlugin.h"
#include "Utils/NoWarningCVMat.h"

namespace asst
{
struct SanityResult
{
    int current = 0; // 当前理智
    int max = 0;     // 最大理智
};

struct FightSeriesListItem
{
public:
    enum class Status
    {
        Unknown = 0,      // 0 - 未知
        Available = 1,    // 1 - 可用
        NeedMedicine = 2, // 2 - 需要药品
        NeedStone = 3,    // 3 - 需要源石
    };

    int times = 0;                   // 连战次数
    asst::Rect rect;                 // 点击区域
    Status status = Status::Unknown; // 是否可用
};

class FightTimesTaskPlugin : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~FightTimesTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    void set_series(int series) { m_series = series; }

    void set_fight_times(int times) { m_fight_times_max = times; }

    void finish_fight(int times) { m_fight_times += times; }

    // 获取 当前理智/最大理智
    static std::optional<asst::SanityResult> analyze_sanity_remain(const cv::Mat& image);
    // 获取 连战次数
    static std::optional<int> analyze_stage_series(const cv::Mat& image);
    // 获取 理智消耗
    static std::optional<int> analyze_sanity_cost(const cv::Mat& image);
    // 获取 连战列表
    static std::vector<asst::FightSeriesListItem> analyze_series_list(const cv::Mat& image);

protected:
    virtual bool _run() override;

private:
    bool open_series_list(const cv::Mat& image = cv::Mat());
    // 计算并调整连续战斗次数, 返回是否修改了次数
    bool change_series(int sanity_remain, int sanity_cost, int series);
    bool select_series(bool available_only);
    bool select_series(int times);

    int m_fight_times = 0;                    // 已战斗次数
    int m_fight_times_max = INT_MAX;          // 最大战斗次数
    int m_series = -1;                        // 连续战斗次数
    mutable bool m_has_used_medicine = false; // 是否使用过药品
};
}
