#include "PenguinUploader.h"

// #include <Windows.h>

#include <meojson/json.h>

#include "AsstUtils.hpp"
#include "Logger.hpp"
#include "Resource.h"
#include "Version.h"

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
    auto& opt = Resrc.cfg().get_options();
    body["server"] = opt.penguin_report.server;
    body["stageId"] = rec["stage"]["stageId"];
    // To fix: https://github.com/MistEO/MeoAssistantArknights/issues/40
    body["drops"] = json::array();
    for (auto&& drop : rec["drops"].as_array()) {
        if (!drop["itemId"].as_string().empty()) {
            body["drops"].as_array().emplace_back(drop);
        }
    }
    body["source"] = "MeoAssistant";
    body["version"] = Version;

    return body.to_string();
}

bool asst::PenguinUploader::request_penguin(const std::string& body)
{
    auto& opt = Resrc.cfg().get_options();
    std::string body_escape = utils::string_replace_all(body, "\"", "\\\"");
    std::string cmd_line = utils::string_replace_all(opt.penguin_report.cmd_format, "[body]", body_escape);
    cmd_line = utils::string_replace_all(cmd_line, "[extra]", opt.penguin_report.extra_param);

    Log.trace("request_penguin |", cmd_line);

    std::string response = utils::callcmd(cmd_line);

    Log.trace("response:\n", response);

    return true;
}
