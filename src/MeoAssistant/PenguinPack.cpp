#include "PenguinPack.h"

#include <filesystem>

#include <meojson/json.hpp>
#include <opencv2/opencv.hpp>
#include <penguin-stats-recognize/recognizer.hpp>

#include "AsstUtils.hpp"
#include "Logger.hpp"

bool asst::PenguinPack::load(const std::string& dir)
{
    LogTraceFunction;

    if (!std::filesystem::exists(dir)) {
        return false;
    }

    bool ret = load_json(dir + "/json/stages.json", dir + "/json/hash_index.json");

    for (const auto& file : std::filesystem::directory_iterator(dir + "/items")) {
        ret &= load_templ(file.path().stem().u8string(), file.path().u8string());
    }
    return ret;
}

void asst::PenguinPack::set_language(const std::string& server)
{
    m_language = server;
    ::load_server(server);
}

std::string asst::PenguinPack::recognize(const cv::Mat image)
{
    Recognizer recognizer("RESULT");
    auto report = recognizer.recognize(image);

    return report.dump(4);
}

bool asst::PenguinPack::load_json(const std::string& stage_path, const std::string& hash_path)
{
    auto stage_parse_ret = json::parse(utils::load_file_without_bom(stage_path));
    if (!stage_parse_ret) {
        m_last_error = stage_path + " parsing failed";
        return false;
    }
    // stage_json 来自 https://penguin-stats.io/PenguinStats/api/v2/stages
    // 和接口需要的json有点区别，这里做个转换
    json::value stage_json = std::move(stage_parse_ret.value());
    json::object cvt_stage_json;
    try {
        for (const json::value& stage_info : stage_json.as_array()) {
            if (!stage_info.contains("dropInfos")) { // 这种一般是以前的活动关，现在已经关闭了的
                continue;
            }
            std::string key = stage_info.at("code").as_string();
            json::value stage_dst;
            stage_dst["stageId"] = stage_info.at("stageId");
            std::vector<json::value> drops_vector;
            for (const json::value& drop_info : stage_info.at("dropInfos").as_array()) {
                if (drop_info.contains("itemId")) {
                    // 幸运掉落，家具啥的，企鹅数据不接，忽略掉
                    if (drop_info.at("dropType").as_string() == "FURNITURE") {
                        continue;
                    }
                    drops_vector.emplace_back(drop_info.at("itemId"));
                }
            }
            stage_dst["drops"] = json::array(std::move(drops_vector));
            stage_dst["existence"] = stage_info.at("existence").at(m_language).at("exist");

            // 企鹅识别 4.2 新增的字段，为了区分第十章的 标准关卡 or 磨难关卡
            json::value difficulty_json;
            if (stage_dst["stageId"].as_string().find("tough_") == 0) {
                difficulty_json["TOUGH"] = std::move(stage_dst);
            }
            else {
                difficulty_json["NORMAL"] = std::move(stage_dst);
            }
            cvt_stage_json[std::move(key)] |= std::move(difficulty_json.as_object());
        }
    }
    catch (json::exception& e) {
        m_last_error = stage_path + " parsing error " + e.what();
        return false;
    }
    std::string cvt_string = cvt_stage_json.to_string();
    ::wload_stage_index(std::move(cvt_stage_json.to_string()));

    ::wload_hash_index(utils::load_file_without_bom(hash_path));
    return true;
}

bool asst::PenguinPack::load_templ(const std::string& item_id, const std::string& path)
{
    cv::Mat templ = cv::imread(path);
    if (templ.empty()) {
        m_last_error = "templ is empty";
        return false;
    }

    auto& resource = penguin::resource;
    if (!resource.contains<std::map<std::string, cv::Mat>>("item_templs")) {
        resource.add("item_templs", std::map<std::string, cv::Mat>());
    }
    auto& item_templs =
        resource.get<std::map<std::string, cv::Mat>>("item_templs");
    item_templs.emplace(item_id, templ);

    return true;
}
