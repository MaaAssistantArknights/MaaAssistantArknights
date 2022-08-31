#include "ReportDataTask.h"

#include "AsstUtils.hpp"
#include "Logger.hpp"
#include "Resource.h"

#include <meojson/json.hpp>

#include <chrono>
#include <functional>
#include <random>
#include <sstream>

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

    for (int i = 0; i != m_retry_times; ++i) {
        const auto& response =
            escape_and_request("ReportToPenguinStats", Resrc.cfg().get_options().penguin_report.cmd_format);

        if (response.success()) [[likely]] {
            if (response.contains_header("x-penguin-set-penguinid")) {
                json::value id_info = basic_info_with_what("PenguinId");
                std::string penguin_id(response.at_header("x-penguin-set-penguinid"));
                id_info["details"]["id"] = penguin_id;
                callback(AsstMsg::SubTaskExtraInfo, id_info);
            }
            break;
        }
        else if (!response.code_5xx()) [[unlikely]] {
            json::value cb_info = basic_info();
            cb_info["why"] = std::string(response.status_code_info());
            cb_info["details"]["status_code"] = response.status_code();
            callback(AsstMsg::SubTaskError, cb_info);
        }
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
    Response response = utils::callcmd(cmd_line);
    Log.info("response:\n", response);

    cb_info["details"]["response"] = static_cast<std::string&>(response);

    if (response.status_code()) [[likely]] {
        callback(AsstMsg::SubTaskCompleted, cb_info);
    }
    else { // 连状态码都没有，一般是网络错误
        cb_info["why"] = "上报失败";
        callback(AsstMsg::SubTaskError, cb_info);
    }

    return response;
}
