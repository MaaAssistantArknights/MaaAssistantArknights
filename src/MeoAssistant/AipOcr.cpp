#include "AipOcr.h"

#include <string_view>

#include <opencv2/opencv.hpp>
#include <meojson/json.h>
#include <cpp-base64/base64.hpp>

#include "AsstUtils.hpp"
#include "Logger.hpp"
#include "Resource.h"

bool asst::AipOcr::request_access_token(const std::string& client_id, const std::string& client_secret)
{
    LogTraceFunction;

    m_access_token.clear();

    std::string_view cmd_fmt =
        R"(curl -s -k "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=%s&client_secret=%s")";

    constexpr int cmd_len = 256;
    char cmd[cmd_len] = { 0 };
#ifdef _MSC_VER
    sprintf_s(cmd, cmd_len, cmd_fmt.data(), client_id.c_str(), client_secret.c_str());
#else
    sprintf(cmd, cmd_fmt.data(), client_id.c_str(), client_secret.c_str());
#endif

    Log.trace("call cmd:", cmd);

    std::string response = utils::callcmd(cmd);

    Log.trace("response:", response);

    auto parse_res = json::parse(response);
    if (!parse_res) {
        return false;
    }
    try {
        auto json = parse_res.value();
        m_access_token = json.at("access_token").as_string();
    }
    catch (...) {
        return false;
    }
    return true;
}

bool asst::AipOcr::request_ocr_general(const cv::Mat image, std::vector<TextRect>& out_result, const TextRectProc& pred)
{
    LogTraceFunction;

    if (m_access_token.empty()) {
        auto& cfg = Resrc.cfg().get_options().aip_ocr;
        request_access_token(cfg.client_id, cfg.client_secret);
    }

    std::string_view cmd_fmt =
        R"(curl -s -k "https://aip.baidubce.com/rest/2.0/ocr/v1/general?access_token=%s" --data "image=%s" -H "Content-Type:application/x-www-form-urlencoded")";

    return request_ocr_and_parse(cmd_fmt, image, out_result, pred);
}

bool asst::AipOcr::request_ocr_accurate(const cv::Mat image, std::vector<TextRect>& out_result, const TextRectProc& pred)
{
    LogTraceFunction;

    if (m_access_token.empty()) {
        auto& cfg = Resrc.cfg().get_options().aip_ocr;
        request_access_token(cfg.client_id, cfg.client_secret);
    }

    std::string_view cmd_fmt =
        R"(curl -s -k "https://aip.baidubce.com/rest/2.0/ocr/v1/accurate?access_token=%s" --data "image=%s" -H "Content-Type:application/x-www-form-urlencoded")";

    return request_ocr_and_parse(cmd_fmt, image, out_result, pred);
}

bool asst::AipOcr::request_ocr_and_parse(std::string_view cmd_fmt, const cv::Mat image, std::vector<TextRect>& out_result, const TextRectProc& pred)
{
    if (m_access_token.empty()) {
        return false;
    }
    // CreateProcess 最大只能接受 32767 chars，一张图片随便就超过了
    // 这个功能暂时没法用，得集成下 libcurl 或者别的
    std::vector<uchar> buf;
    cv::imencode(".png", image, buf);
    auto* enc_msg = reinterpret_cast<unsigned char*>(buf.data());
    std::string encoded = base64_encode(enc_msg, buf.size());

    size_t cmd_len = encoded.size() + cmd_fmt.size() + m_access_token.size();
    char* cmd = new char[cmd_len];
    memset(cmd, 0, cmd_len);
#ifdef _MSC_VER
    sprintf_s(cmd, cmd_len, cmd_fmt.data(), m_access_token.c_str(), encoded.c_str());
#else
    sprintf(cmd, cmd_fmt.data(), m_access_token.c_str(), encoded.c_str());
#endif
    Log.trace("call cmd:", cmd);
    std::string response = utils::callcmd(cmd);
    delete[] cmd;
    Log.trace("response:", response);

    auto parse_res = json::parse(response);
    if (!parse_res) {
        return false;
    }
    try {
        auto json = parse_res.value();
        return parse_response(json, out_result, pred);
    }
    catch (...) {
        return false;
    }
}

bool asst::AipOcr::parse_response(const json::value& json, std::vector<TextRect>& out_result, const TextRectProc& pred)
{
    if (!json.exist("words_result")) {
        return false;
    }

    std::vector<TextRect> result;
    std::string log_str_raw;
    std::string log_str_proc;
    for (const auto& word : json.at("words_result").as_array()) {
        std::string text = word.at("words").as_string();

        auto& loc = word.at("location").as_object();
        int left = loc.at("left").as_integer();
        int right = loc.at("right").as_integer();
        int top = loc.at("top").as_integer();
        int bottom = loc.at("bottom").as_integer();

        Rect rect(left, top, right - left, bottom - top);

        TextRect tr{ text, rect, 0 };

        log_str_raw += tr.to_string() + ", ";
        if (!pred || pred(tr)) {
            log_str_proc += tr.to_string() + ", ";
            result.emplace_back(std::move(tr));
        }
    }

    Log.trace("AipOcr::parse_response | raw : ", log_str_raw);
    Log.trace("AipOcr::parse_response | proc : ", log_str_proc);

    out_result = result;
    return true;
}
