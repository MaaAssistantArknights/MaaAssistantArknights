#include "ReclamationConclusionReportPlugin.h"

#include <regex>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

bool asst::ReclamationConclusionReportPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }
    const std::string task = details.at("details").at("task").as_string();
    if (task.ends_with("Reclamation@GiveupSkipConfirm")) {
        return true;
    }
    return false;
}

bool asst::ReclamationConclusionReportPlugin::_run()
{
    const auto img = ctrler()->get_image();
    if (!check_page_valid(img)) {
        return false;
    }
    analyze(img);
    info_callback();
    return true;
}

bool asst::ReclamationConclusionReportPlugin::check_page_valid(const cv::Mat& image)
{
    Matcher pageAnalyzer(image);
    pageAnalyzer.set_task_info("Reclamation@GiveupSkipConfirm");
    return pageAnalyzer.analyze().has_value();
}

void asst::ReclamationConclusionReportPlugin::analyze(const cv::Mat& image)
{
    m_badges = analyze_badges(image);
    m_construction_points = analyze_construction_points(image);

    m_total_badges += (m_badges == -1 ? 0 : m_badges);
    m_total_cons_points += (m_construction_points == -1 ? 0 : m_construction_points);
}

int asst::ReclamationConclusionReportPlugin::analyze_badges(const cv::Mat& image)
{
    Matcher badgeIconAnalyzer(image);
    badgeIconAnalyzer.set_task_info("Reclamation@ConclusionReportBadgeIcon");
    if (!badgeIconAnalyzer.analyze()) return -1;
    const auto& iconRect = badgeIconAnalyzer.get_result();

    OCRer badgeCntAnalyzer(image);
    badgeCntAnalyzer.set_task_info("NumberOcrReplace");
    Rect roi = Task.get("Reclamation@ReportBadgesOcr")->roi;
    int newX = iconRect.rect.x + iconRect.rect.width;
    roi.width -= (newX - roi.x);
    roi.x = newX;
    badgeCntAnalyzer.set_roi(roi);
    if (!badgeCntAnalyzer.analyze()) return -1;
    std::string result = badgeCntAnalyzer.get_result().front().text;
    try {
        return std::stoi(result);
    }
    catch (const std::invalid_argument&) {
        return -1;
    }
}

int asst::ReclamationConclusionReportPlugin::analyze_construction_points(const cv::Mat& image)
{
    OCRer consAnalyzer(image);
    consAnalyzer.set_task_info("NumberOcrReplace");
    consAnalyzer.set_roi(Task.get("Reclamation@ReportConstructPointsOcr")->roi);
    if (!consAnalyzer.analyze()) return -1;
    std::string result = consAnalyzer.get_result().front().text;
    try {
        return std::stoi(result);
    }
    catch (const std::invalid_argument&) {
        return -1;
    }
}

void asst::ReclamationConclusionReportPlugin::info_callback()
{
    json::value info = basic_info_with_what("ReclamationReport");
    json::value& details = info["details"];
    details["badges"] = m_badges;
    details["construction_points"] = m_construction_points;
    details["total_badges"] = m_total_badges;
    details["total_construction_points"] = m_total_cons_points;
    callback(AsstMsg::SubTaskExtraInfo, info);
}
