#pragma once
#include "Task/AbstractTaskPlugin.h"

#include <memory>
#include <vector>

#include <meojson/json.hpp>

#include "Config/Miscellaneous/StageDropsConfig.h"
#include "Status.h"
#include "Utils/NoWarningCVMat.h"

namespace asst
{
    class ProcessTask;

    class ReclamationConclusionReportPlugin final : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~ReclamationConclusionReportPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    private:
        virtual bool _run() override;

        bool check_page_valid(const cv::Mat& image);

        void analyze(const cv::Mat& image);
        int analyze_badges(const cv::Mat& image);
        int analyze_construction_points(const cv::Mat& image);

        void info_callback();

        int m_badges = 0;
        int m_construction_points = 0;
        int m_total_badges = 0;
        int m_total_cons_points = 0;
    };
}
