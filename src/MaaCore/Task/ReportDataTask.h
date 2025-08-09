#pragma once

#include "AbstractTask.h"

#include <cctype>
#include <future>
#include <unordered_set>

#include "Utils/NoWarningCPR.h"

namespace asst
{
enum class ReportType
{
    Invalid,
    PenguinStats,
    YituliuBigDataAutoRecruit,
    YituliuBigDataStageDrops,
};

using TaskCallback = std::function<void(AsstMsg, const json::value&, AbstractTask*)>;

class ReportDataTask : public AbstractTask
{
public:
    ReportDataTask(const TaskCallback& task_callback, AbstractTask* upper_task);
    ReportDataTask(const ReportDataTask&) = default;
    ReportDataTask(ReportDataTask&&) = default;

    virtual ~ReportDataTask() override = default;

    ReportDataTask& set_report_type(ReportType type);

    ReportDataTask& set_body(std::string body);
    ReportDataTask& set_extra_headers(std::unordered_map<std::string, std::string> headers);

protected:
    struct ReportRequest
    {
        std::string url;
        std::unordered_map<std::string, std::string> extra_headers;
        std::string body;
        std::string subtask_name;
    };

    virtual bool _run() override;
    virtual void callback(AsstMsg msg, const json::value& detail) override;

    void report_to_penguin();
    void report_to_yituliu(ReportType reportType);

    ReportType m_report_type = ReportType::Invalid;
    std::string m_body;
    std::unordered_map<std::string, std::string> m_extra_headers;
    std::vector<std::future<void>> m_upload_pending;

    TaskCallback m_task_callback = nullptr;
    AbstractTask* m_upper_task = nullptr;
};
} // namespace asst
