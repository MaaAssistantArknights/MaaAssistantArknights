#include "PenguinUploader.h"

#include <Windows.h>

#include <json.h>

#include "Configer.h"
#include "Version.h"
#include "Logger.hpp"

bool asst::PenguinUploader::upload(const std::string& rec_res)
{
    std::string body = cvt_json(rec_res);
    return request_penguin(body);
}

std::string asst::PenguinUploader::cvt_json(const std::string& rec_res)
{
    auto parse_ret = json::parse(rec_res);
    if (!parse_ret) {
        return std::string();
    }
    json::value rec = std::move(parse_ret.value());

    // Doc: https://developer.penguin-stats.io/public-api/api-v2-instruction/report-api
    json::value body;
    body["server"] = Configer::get_instance().m_options.penguin_server;
    body["stageId"] = rec["stage"]["stageId"];
    body["drops"] = rec["drops"];
    body["source"] = "MeoAssistance";
    body["version"] = Version;

    return body.to_string();
}

bool asst::PenguinUploader::request_penguin(const std::string& body)
{
    std::string curl_cmd = R"(curl -H "Content-Type: application/json" -X POST -d )";
    curl_cmd += '\'' + body + "' \"" + Configer::get_instance().m_options.penguin_api + '"';

    DebugTrace("request_penguin |", curl_cmd);

    return !system(curl_cmd.c_str());
}