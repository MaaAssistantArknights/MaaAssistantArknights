#include "PenguinPack.h"

#include <filesystem>

#include <meojson/json.h>
#include <opencv2/opencv.hpp>
namespace penguin
{
#include <penguin-stats-recognize/penguin_wasm.h>
}

#include "AsstUtils.hpp"

bool asst::PenguinPack::load(const std::string& dir)
{
    bool ret = load_json(dir + "/json/stages.json", dir + "/json/hash_index.json");

    for (const auto& file : std::filesystem::directory_iterator(dir + "/items")) {
        ret &= load_templ(file.path().stem().u8string(), file.path().u8string());
    }
    return ret;
}

void asst::PenguinPack::set_language(const std::string& server)
{
    m_language = server;
    penguin::load_server(server.c_str());
}

std::string asst::PenguinPack::recognize(const cv::Mat image)
{
    std::vector<uchar> buf;
    cv::imencode(".png", image, buf);
    return penguin::recognize(buf.data(), buf.size());
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
            if (!stage_info.exist("dropInfos")) { // 这种一般是以前的活动关，现在已经关闭了的
                continue;
            }
            std::string key = stage_info.at("code").as_string();
            json::value stage_dst;
            stage_dst["stageId"] = stage_info.at("stageId");
            std::vector<json::value> drops_vector;
            for (const json::value& drop_info : stage_info.at("dropInfos").as_array()) {
                if (drop_info.exist("itemId")) {
                    // 幸运掉落，家具啥的，企鹅数据不接，忽略掉
                    if (drop_info.at("dropType").as_string() == "FURNITURE") {
                        continue;
                    }
                    drops_vector.emplace_back(drop_info.at("itemId"));
                }
            }
            stage_dst["drops"] = json::array(std::move(drops_vector));
            stage_dst["existence"] = stage_info.at("existence").at(m_language).at("exist");

            cvt_stage_json.emplace(std::move(key), std::move(stage_dst));
        }
    }
    catch (json::exception& e) {
        m_last_error = stage_path + " parsing error " + e.what();
        return false;
    }
    std::string cvt_stage_string = cvt_stage_json.to_string();
    penguin::load_json(cvt_stage_string.c_str(), utils::load_file_without_bom(hash_path).c_str());
    return true;
}

bool asst::PenguinPack::load_templ(const std::string& item_id, const std::string& path)
{
    cv::Mat image = cv::imread(path);
    if (image.empty()) {
        m_last_error = "templ is empty";
        return false;
    }

    std::vector<uchar> buf;
    cv::imencode(".png", image, buf);

    penguin::load_templ(item_id.c_str(), buf.data(), buf.size());

    return true;
}