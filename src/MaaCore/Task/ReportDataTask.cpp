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

    switch (m_report_type) {
    case ReportType::PenguinStats:
        report_to_penguin();
        break;
    case ReportType::YituliuBigDataAutoRecruit:
    case ReportType::YituliuBigDataStageDrops:
        report_to_yituliu(m_report_type);
        break;
    case ReportType::Invalid:
    default:
        return false;
    }

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
    LogTraceFunction;

    using namespace std::chrono;
    auto epoch = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch();
    long long tick = duration_cast<nanoseconds>(epoch).count();

    static std::default_random_engine rand_engine(std::random_device {}());
    static std::uniform_int_distribution<uint64_t> rand_uni(0, UINT64_MAX);
    uint64_t rand = rand_uni(rand_engine);

    std::stringstream key_ss;
    key_ss << "MAA" << std::hex << tick << rand;
    std::string idempotency_key = key_ss.str();

    std::string url = Config.get_options().penguin_report.url;

    std::unordered_map<std::string, std::string> headers;
    for (const auto& field : m_extra_headers) {
        headers.emplace(field);
    }
    headers.emplace("X-Penguin-Idempotency-Key", std::move(idempotency_key));

    ReportRequest req{
        .url = std::move(url),
        .extra_headers = std::move(headers),
        .body = m_body,
        .subtask_name = "ReportToPenguinStats"
    };

    json::value req_json = json::object {
        {"url", req.url},
        {"headers", json::object{}},
        {"body", req.body},
        {"subtask", req.subtask_name}
    };
    for (auto& [k,v] : req.extra_headers) {
        req_json["headers"][k] = v;
    }
    callback(AsstMsg::ReportRequest, req_json);
}

void asst::ReportDataTask::report_to_yituliu(ReportType reportType)
{
    LogTraceFunction;

    std::string url;
    switch (reportType) {
    case ReportType::YituliuBigDataAutoRecruit:
        url = Config.get_options().yituliu_report.recruit_url;
        break;
    case ReportType::YituliuBigDataStageDrops:
        url = Config.get_options().yituliu_report.drop_url;
        break;
    case ReportType::Invalid:
    default:
        return;
    }

    std::unordered_map<std::string, std::string> headers;
    for (const auto& field : m_extra_headers) {
        headers.emplace(field);
    }

    ReportRequest req{
        .url = std::move(url),
        .extra_headers = std::move(headers),
        .body = m_body,
        .subtask_name = "ReportToYituliu"
    };

    json::value req_json = json::object {
        {"url", req.url},
        {"headers", json::object{}},
        {"body", req.body},
        {"subtask", req.subtask_name}
    };
    for (auto& [k,v] : req.extra_headers) {
        req_json["headers"][k] = v;
    }
    callback(AsstMsg::ReportRequest, req_json);
}
