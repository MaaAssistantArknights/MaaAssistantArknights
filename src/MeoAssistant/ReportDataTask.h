#pragma once

#include "AbstractTask.h"

#include <future>

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
        struct Response
        {
            bool success = false;
            std::string response;
        };

    protected:
        virtual bool _run() override;

        void report_to_penguin();
        void report_to_yituliu();
        Response escape_and_request(const std::string& subtask, const std::string& format);

        ReportType m_report_type = ReportType::Invaild;
        std::string m_body;
        std::string m_extra_param;
        std::vector<std::future<void>> m_upload_pending;
    };
}
