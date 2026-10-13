#pragma once
#include "Task/AbstractTask.h"

#include <unordered_map>

#include "Vision/Miscellaneous/DepotImageAnalyzer.h"

namespace asst
{
class DepotRecognitionTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~DepotRecognitionTask() noexcept override = default;

    void set_strict_scan(bool value) noexcept { m_strict_scan = value; }

protected:
    virtual bool _run() override;

    bool swipe_and_analyze();
    bool strict_swipe_and_analyze();
    bool confirm_operator_token_boundary(const Rect& rect, const cv::Mat& grid_frame);
    bool read_lmd_detail(ItemInfo& item, const cv::Mat& grid_frame);
    bool analyze_basic_items();
    bool report_invalid_templates(const DepotImageAnalyzer& analyzer);
    void callback_analyze_result(bool done);
    void swipe();
    std::unordered_map<std::string, ItemInfo> m_all_items;
    bool m_invalid_templates = false;
    bool m_strict_scan = false;
    bool m_scan_verified = false;
};
}
