#pragma once

#include "Task/AbstractTaskPlugin.h"

namespace cv
{
class Mat;
}

namespace asst
{
class StartUpEventPagePlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~StartUpEventPagePlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

protected:
    virtual bool _run() override;

private:
    static bool is_pc_event_page(const cv::Mat& image);

    static constexpr std::string_view EscTaskName = "ClosePCEventPageEsc";
};
}
