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
    using HttpResponsePred = std::function<bool(const cpr::Response&)>;

    virtual bool _run() override;
    virtual void callback(AsstMsg msg, const json::value& detail) override;

    void report_to_penguin();
    void report_to_yituliu(ReportType reportType);
    cpr::Response report(
        std::string_view subtask,
        const std::string& url,
        const cpr::Header& headers,
        const int timeout,
        HttpResponsePred success_cond = [](const cpr::Response& response) -> bool {
            return response.status_code == 200;
        },
        HttpResponsePred retry_cond = [](const cpr::Response& response) -> bool {
            return !response.status_code || (response.status_code >= 500 && response.status_code < 600);
        },
        bool callback_on_failure = true);

    ReportType m_report_type = ReportType::Invalid;
    std::string m_body;
    std::unordered_map<std::string, std::string> m_extra_headers;
    std::vector<std::future<void>> m_upload_pending;

    TaskCallback m_task_callback = nullptr;
    AbstractTask* m_upper_task = nullptr;
};
} // namespace asst
