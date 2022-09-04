#include "ReportDataTask.h"

#include "AsstUtils.hpp"
#include "GeneralConfiger.h"
#include "Logger.hpp"

#include <meojson/json.hpp>

#include <chrono>
#include <functional>
#include <random>
#include <sstream>

std::vector<std::future<void>> asst::ReportDataTask::m_upload_pending;

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
    using namespace std::chrono;

    LogTraceFunction;

    auto epoch = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch();
    long long tick = duration_cast<nanoseconds>(epoch).count();

    static std::default_random_engine rand_engine(std::random_device {}());
    static std::uniform_int_distribution<uint64_t> rand_uni(0, UINT64_MAX);

    uint64_t rand = rand_uni(rand_engine);

    std::stringstream key_ss;
    key_ss << "MAA" << std::hex << tick << rand;
    std::string key = key_ss.str();
    Log.info("X-Penguin-Idempotency-Key:", key);

    m_extra_param += " -H \"X-Penguin-Idempotency-Key: " + std::move(key) + "\"";

    http::Response response = report("ReportToPenguinStats", Configer.get_options().penguin_report.cmd_format);

    if (response.success()) {
        if (auto penguinid_opt = response.find_header("x-penguin-set-penguinid")) [[unlikely]] {
            json::value id_info = basic_info_with_what("PenguinId");
            id_info["details"]["id"] = std::string(penguinid_opt.value());
            callback(AsstMsg::SubTaskExtraInfo, id_info);
        }
    }
}

void asst::ReportDataTask::report_to_yituliu()
{
    LogTraceFunction;

    report("ReportToYituliu", Configer.get_options().yituliu_report.cmd_format);
}

asst::http::Response asst::ReportDataTask::report(const std::string& subtask, const std::string& format,
                                                  HttpResponsePred success_cond, HttpResponsePred retry_cond)
{
    LogTraceFunction;

    json::value cb_info = basic_info();
    cb_info["subtask"] = subtask;
    callback(AsstMsg::SubTaskStart, cb_info);

    http::Response response;
    for (int i = 0; i <= m_retry_times; ++i) {
        response = escape_and_request(format);
        if (success_cond(response)) {
            callback(AsstMsg::SubTaskCompleted, cb_info);
            return response;
        }
        else if (!retry_cond(response)) {
            break;
        }
    }

    cb_info["why"] = "上报失败";
    cb_info["details"] = json::object { { "error", response.get_last_error() },
                                        { "status_code", response.status_code() },
                                        { "status_code_info", std::string(response.status_code_info()) },
                                        { "response", std::string(response.data()) } };

    callback(AsstMsg::SubTaskError, cb_info);

    return response;
}

asst::http::Response asst::ReportDataTask::escape_and_request(const std::string& format)
{
    LogTraceFunction;

    std::string body_escape = utils::string_replace_all(m_body, { { "\"", "\\\"" } });

#ifdef _WIN32
    std::string body_escapes = utils::utf8_to_unicode_escape(body_escape);
#else
    std::string body_escapes = body_escape;
#endif

    std::string cmd_line =
        utils::string_replace_all(format, { { "[body]", body_escapes }, { "[extra]", m_extra_param } });

    Log.info("request:\n", cmd_line);
    http::Response response = utils::callcmd(cmd_line);
    Log.info("response:\n", response);

    return response;
}
