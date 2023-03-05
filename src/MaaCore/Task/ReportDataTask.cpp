#include "ReportDataTask.h"

#include "Config/GeneralConfig.h"
#include "Utils/Locale.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

#include <meojson/json.hpp>

#include <chrono>
#include <functional>
#include <random>
#include <sstream>

asst::ReportDataTask::ReportDataTask(const TaskCallback& task_callback, AbstractTask* upper_task)
    : AbstractTask(nullptr, nullptr, upper_task->get_task_chain()), m_task_callback(task_callback),
      m_upper_task(upper_task)
{}

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

void asst::ReportDataTask::callback(AsstMsg msg, const json::value& detail)
{
    if (m_task_callback) {
        m_task_callback(msg, detail, m_upper_task);
    }
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

    constexpr int DefaultBackoff = 10 * 1000; // 10s
    int backoff = DefaultBackoff;

    auto penguin_success_cond = [](const http::Response& response) -> bool { return response.success(); };
    auto penguin_retry_cond = [&](const http::Response& response) -> bool {
        if (!response.status_code()) {
            return true;
        }
        if (response.status_5xx()) {
            backoff = static_cast<int>(backoff * 1.5);
            sleep(backoff);
            return true;
        }
        return false;
    };

    constexpr std::string_view PenguinSubtaskName = "ReportToPenguinStats";
    const std::string& cmd_format = Config.get_options().penguin_report.cmd_format;
    http::Response response = report(PenguinSubtaskName, cmd_format, penguin_success_cond, penguin_retry_cond, false);

    auto proc_response_id = [&]() {
        if (auto penguinid_opt = response.find_header("x-penguin-set-penguinid")) [[unlikely]] {
            json::value id_info = basic_info_with_what("PenguinId");
            id_info["details"]["id"] = std::string(penguinid_opt.value());
            callback(AsstMsg::SubTaskExtraInfo, id_info);
        }
    };

    if (response.success()) {
        proc_response_id();
        return;
    }

    // 重新向企鹅物流统计的 CN 域名发送数据
    constexpr std::string_view Penguin_IO = "https://penguin-stats.io";
    constexpr std::string_view Penguin_CN = "https://penguin-stats.cn";
    if (cmd_format.find(Penguin_IO) == std::string::npos) {
        return;
    }
    Log.info("Re-report to penguin-stats.cn", Penguin_CN);
    std::string new_cmd_format = utils::string_replace_all(cmd_format, Penguin_IO, Penguin_CN);

    backoff = DefaultBackoff;
    response = report(PenguinSubtaskName, new_cmd_format, penguin_success_cond, penguin_retry_cond);

    if (response.success()) {
        proc_response_id();
        return;
    }
}

void asst::ReportDataTask::report_to_yituliu()
{
    LogTraceFunction;

    constexpr std::string_view YituliuSubtaskName = "ReportToYituliu";
    const std::string& cmd_format = Config.get_options().yituliu_report.cmd_format;
    report(YituliuSubtaskName, cmd_format);
}

asst::http::Response asst::ReportDataTask::report(std::string_view subtask, const std::string& format,
                                                  HttpResponsePred success_cond, HttpResponsePred retry_cond,
                                                  bool callback_on_failure)
{
    LogTraceFunction;

    json::value cb_info = basic_info();
    cb_info["subtask"] = std::string(subtask);
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
        else {
            Log.trace("retrying... | why:", response.get_last_error(), "status_code:", response.status_code());
        }
        if (need_exit()) {
            break;
        }
    }

    if (callback_on_failure) {
        cb_info["why"] = "上报失败";
        cb_info["details"] = json::object {
            { "error", response.get_last_error() },
            { "status_code", response.status_code() },
            { "status_code_info", std::string(response.status_code_info()) },
            { "response", std::string(response.body()) },
        };

        callback(AsstMsg::SubTaskError, cb_info);
    }

    return response;
}

asst::http::Response asst::ReportDataTask::escape_and_request(const std::string& format)
{
    std::string body_escape = utils::string_replace_all(m_body, "\"", "\\\"");

#ifdef _WIN32
    body_escape = utils::utf8_to_unicode_escape(body_escape);
#endif

    std::string cmd_line =
        utils::string_replace_all(format, { { "[body]", body_escape }, { "[extra]", m_extra_param } });

    Log.info("request:\n" + cmd_line);
    std::string response = utils::call_command(cmd_line);
    Log.info("response:\n" + response);

    return response;
}
