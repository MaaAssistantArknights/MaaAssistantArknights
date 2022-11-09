#pragma once

#include "AbstractTask.h"
#include "Utils/Http.hpp"

#include <cctype>
#include <future>
#include <unordered_set>

namespace asst
{
    enum class ReportType
    {
        Invalid,
        PenguinStats,
        YituliuBigData,
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
        ReportDataTask& set_extra_param(std::string extra_param);

    protected:
        using HttpResponsePred = std::function<bool(const http::Response&)>;

        virtual bool _run() override;
        virtual void callback(AsstMsg msg, const json::value& detail) override;

        void report_to_penguin();
        void report_to_yituliu();
        http::Response escape_and_request(const std::string& format);
        http::Response report(
            const std::string& subtask, const std::string& format,
            HttpResponsePred success_cond = [](const http::Response& response) -> bool { return response.success(); },
            HttpResponsePred retry_cond = [](const http::Response& response) -> bool {
                return !response.status_code() || response.status_5xx();
            },
            bool callback_on_failure = true);

        ReportType m_report_type = ReportType::Invalid;
        std::string m_body;
        std::string m_extra_param;
        std::vector<std::future<void>> m_upload_pending;

        TaskCallback m_task_callback = nullptr;
        AbstractTask* m_upper_task = nullptr;
    };
} // namespace asst
