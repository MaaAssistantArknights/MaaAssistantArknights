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

asst::ReportDataTask::ReportDataTask(const TaskCallback& task_callback, AbstractTask* upper_task) :
    AbstractTask(nullptr, upper_task->inst(), upper_task->get_task_chain()),
    m_task_callback(task_callback),
    m_upper_task(upper_task)
{
}

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

asst::ReportDataTask& asst::ReportDataTask::set_extra_headers(std::unordered_map<std::string, std::string> headers)
{
    m_extra_headers = std::move(headers);
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
    case ReportType::YituliuBigDataAutoRecruit:
    case ReportType::YituliuBigDataStageDrops:
        report_func = std::bind(&ReportDataTask::report_to_yituliu, this, m_report_type);
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

    constexpr int DefaultBackoff = 3 * 1000; // 3s
    int backoff = DefaultBackoff;

    auto penguin_success_cond = [](const cpr::Response& response) -> bool {
        return response.status_code == 200;
    };
    auto penguin_retry_cond = [&](const cpr::Response& response) -> bool {
        if (!response.status_code) {
            return true;
        }
        if (response.status_code >= 500 && response.status_code < 600) {
            backoff = static_cast<int>(backoff * 1.5);
            sleep(backoff);
            return true;
        }
        return false;
    };

    std::string url = Config.get_options().penguin_report.url;
    int timeout = Config.get_options().penguin_report.timeout;
    cpr::Header headers {};

    for (const auto& [header_key, value] : Config.get_options().penguin_report.headers) {
        headers.emplace(header_key, value);
    }

    for (const auto& field : m_extra_headers) {
        headers.emplace(field);
    }

    headers.emplace("X-Penguin-Idempotency-Key", std::move(key));

    constexpr std::string_view PenguinSubtaskName = "ReportToPenguinStats";
    cpr::Response response =
        report(PenguinSubtaskName, url, headers, timeout, penguin_success_cond, penguin_retry_cond, false);

    auto proc_response_id = [&]() {
        if (response.header.contains("x-penguin-set-penguinid")) [[unlikely]] {
            json::value id_info = basic_info_with_what("PenguinId");
            id_info["details"]["id"] = response.header["x-penguin-set-penguinid"];
            callback(AsstMsg::SubTaskExtraInfo, id_info);
        }
    };

    if (response.status_code == 200) {
        proc_response_id();
        return;
    }

    // 重新向企鹅物流统计的 CN 域名发送数据
    constexpr std::string_view Penguin_IO = "https://penguin-stats.io";
    // https://penguin-stats.alvorna.com: G找朋友开的代理，仅可以代理 API 请求。
    // 实际的网站访问请前往 https://penguin-stats.cn
    // constexpr std::string_view Penguin_CN = "https://penguin-stats.cn";
    constexpr std::string_view Penguin_CN_PROXY = "https://penguin-stats.alvorna.com";
    if (url.find(Penguin_IO) == std::string::npos) {
        return;
    }
    Log.info("Re-report to penguin-stats.alvorna.com", Penguin_CN_PROXY);
    std::string new_url = utils::string_replace_all(url, Penguin_IO, Penguin_CN_PROXY);

    backoff = DefaultBackoff;
    response = report(PenguinSubtaskName, new_url, headers, timeout, penguin_success_cond, penguin_retry_cond);

    if (response.status_code == 200) {
        proc_response_id();
        return;
    }
}

void asst::ReportDataTask::report_to_yituliu(ReportType reportType)
{
    LogTraceFunction;

    constexpr std::string_view yituliu_subtask_name = "ReportToYituliu";
    std::string url;
    switch (reportType) {
    case ReportType::YituliuBigDataAutoRecruit:
        url = Config.get_options().yituliu_report.recruit_url;
        break;
    case ReportType::YituliuBigDataStageDrops:
        url = Config.get_options().yituliu_report.drop_url;
        break;
    default:
        return;
    }
    const int timeout = Config.get_options().yituliu_report.timeout;
    cpr::Header headers = {};

    for (const auto& [header_key, value] : Config.get_options().yituliu_report.headers) {
        headers.emplace(header_key, value);
    }

    for (const auto& field : m_extra_headers) {
        headers.emplace(field);
    }

    report(yituliu_subtask_name, url, headers, timeout);
}

cpr::Response asst::ReportDataTask::report(
    std::string_view subtask,
    const std::string& url,
    const cpr::Header& headers,
    const int timeout,
    HttpResponsePred success_cond,
    HttpResponsePred retry_cond,
    bool callback_on_failure)
{
    LogTraceFunction;

    json::value cb_info = basic_info();
    cb_info["subtask"] = std::string(subtask);
    callback(AsstMsg::SubTaskStart, cb_info);

    Log.info("Report url: ", url);
    for (const auto& [key, value] : headers) {
        Log.info("Report header: ", key, ":", value);
    }
    Log.info("Report body: ", m_body);

    cpr::Response response;
    for (int i = 0; i <= m_retry_times; ++i) {
        response = cpr::Post(
            cpr::Url { url },
            cpr::Body { m_body },
            headers,
            cpr::Timeout { timeout },
            cpr::HttpVersion { cpr::HttpVersionCode::VERSION_2_0 });
        Log.info("Report response status code: ", response.status_code);
        for (const auto& [key, value] : response.header) {
            Log.info("Report response header: ", key, ":", value);
        }
        Log.info("Report response: ", response.text);
        if (success_cond(response)) {
            callback(AsstMsg::SubTaskCompleted, cb_info);
            return response;
        }
        else if (!retry_cond(response)) {
            break;
        }
        else {
            Log.trace("retrying... | why:", response.error.message, "status_code:", response.status_code);
        }
        if (need_exit()) {
            break;
        }
    }

    if (callback_on_failure) {
        cb_info["why"] = "上报失败";
        cb_info["details"] = json::object {
            { "error", response.error.message },
            { "status_code", response.status_code },
            { "status_code_info", response.reason },
            { "response", response.text },
        };

        callback(AsstMsg::SubTaskError, cb_info);
    }

    return response;
}
