#pragma once

#include "AbstractTask.h"
#include "AsstHttp.hpp"
#include "AsstUtils.hpp"

#include <cctype>
#include <future>
#include <unordered_set>

namespace asst
{
    enum class ReportType
    {
        Invaild,
        PenguinStats,
        YituliuBigData,
    };

    class ReportDataTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~ReportDataTask() noexcept override = default;

        ReportDataTask& set_report_type(ReportType type);

        ReportDataTask& set_body(std::string body);
        ReportDataTask& set_extra_param(std::string extra_param);

    protected:
        virtual bool _run() override;

        void report_to_penguin();
        void report_to_yituliu();
        Response escape_and_request(const std::string& format);
        Response report_and_retry(
            const std::string& subtask, const std::string& format, int report_retry_times = 1,
            std::function<bool(const Response&)> retry_condition = [](const Response& response) {
                return response.status_5xx();
            });

        ReportType m_report_type = ReportType::Invaild;
        std::string m_body;
        std::string m_extra_param;
        static inline std::vector<std::future<void>> m_upload_pending;
    };
} // namespace asst
