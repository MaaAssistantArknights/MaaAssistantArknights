#include "ReportDataTask.h"

#include "AsstUtils.hpp"
#include "Logger.hpp"
#include "Resource.h"

#include <meojson/json.hpp>

#include <functional>
#include <regex>

asst::ReportDataTask& asst::ReportDataTask::set_report_type(ReportType type)
{
    m_report_type = type;
    return *this;
}

asst::ReportDataTask& asst::ReportDataTask::set_body(std::string body)
{
    m_body = std::move(body);
    return *this;
}

asst::ReportDataTask& asst::ReportDataTask::set_extra_param(std::string extra_param)
{
    m_extra_param = std::move(extra_param);
    return *this;
}

bool asst::ReportDataTask::_run()
{
    LogTraceFunction;

    std::function<void(void)> report_func;

    switch (m_report_type) {
    case ReportType::PenguinStats:
        report_func = std::bind(&ReportDataTask::report_to_penguin, this);
        break;
    case ReportType::YituliuBigData:
        report_func = std::bind(&ReportDataTask::report_to_yituliu, this);
        break;
    default:
        return false;
    }

    auto upload_future = std::async(std::launch::async, report_func);
    m_upload_pending.emplace_back(std::move(upload_future));

    return true;
}

void asst::ReportDataTask::report_to_penguin()
{
    LogTraceFunction;

    const auto& [success, response] =
        escape_and_request("ReportToPenguinStats", Resrc.cfg().get_options().penguin_report.cmd_format);

    static const std::regex penguinid_regex(R"(X-Penguin-Set-Penguinid: (\d+))");
    std::smatch penguinid_sm;
    if (std::regex_search(response, penguinid_sm, penguinid_regex)) {
        json::value id_info = basic_info_with_what("PenguinId");
        std::string penguin_id = penguinid_sm[1];
        id_info["details"]["id"] = penguin_id;
        callback(AsstMsg::SubTaskExtraInfo, id_info);
    }
}

void asst::ReportDataTask::report_to_yituliu()
{
    LogTraceFunction;

    escape_and_request("ReportToYituliu", Resrc.cfg().get_options().yituliu_report.cmd_format);
}

asst::ReportDataTask::Response asst::ReportDataTask::escape_and_request(const std::string& subtask,
                                                                        const std::string& format)
{
    LogTraceFunction;

    json::value cb_info = basic_info();
    cb_info["subtask"] = subtask;
    callback(AsstMsg::SubTaskStart, cb_info);

    std::string body_escape = utils::string_replace_all_batch(m_body, { { "\"", "\\\"" } });

#ifdef _WIN32
    std::string body_escapes = utils::utf8_to_unicode_escape(body_escape);
#else
    std::string body_escapes = body_escape;
#endif

    std::string cmd_line =
        utils::string_replace_all_batch(format, { { "[body]", body_escapes }, { "[extra]", m_extra_param } });

    Log.info("request:\n", cmd_line);
    std::string response = utils::callcmd(cmd_line);
    Log.info("response:\n", response);

    cb_info["details"]["response"] = response;

    bool success = false;
    static const std::regex http_ok_regex(R"(HTTP/.+ 200)");
    if (std::regex_search(response, http_ok_regex)) {
        success = true;
        callback(AsstMsg::SubTaskCompleted, cb_info);
    }
    else {
        success = false;
        cb_info["why"] = "上报失败";
        callback(AsstMsg::SubTaskError, cb_info);
    }

    return { success, response };
}
