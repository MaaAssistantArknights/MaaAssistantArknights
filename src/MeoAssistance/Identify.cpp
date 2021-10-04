#include "Identify.h"

#include <algorithm>
#include <numeric>
#include <filesystem>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#include <json.h>

namespace penguin {
#include <penguin-stats-recognize/penguin_wasm.h>
}

#include "Logger.hpp"
#include "AsstAux.h"

using namespace asst;
using namespace cv;

bool Identify::add_image(const std::string& name, const std::string& path)
{
    Mat image = imread(path);
    if (image.empty()) {
        return false;
    }
    m_mat_map.emplace(name, image);
    return true;
}

Mat Identify::image_2_hist(const cv::Mat& src)
{
    Mat src_hsv;
    cvtColor(src, src_hsv, COLOR_BGR2HSV);

    int histSize[] = { 50, 60 };
    float h_ranges[] = { 0, 180 };
    float s_ranges[] = { 0, 256 };
    const float* ranges[] = { h_ranges, s_ranges };
    int channels[] = { 0, 1 };

    MatND src_hist;

    calcHist(&src_hsv, 1, channels, Mat(), src_hist, 2, histSize, ranges);
    normalize(src_hist, src_hist, 0, 1, NORM_MINMAX);

    return src_hist;
}

double Identify::image_hist_comp(const cv::Mat& src, const cv::MatND& hist)
{
    // keep the interface return value unchanged
    return 1 - compareHist(image_2_hist(src), hist, CV_COMP_BHATTACHARYYA);
}

std::vector<TextArea> asst::Identify::ocr_detect(const cv::Mat& image)
{
    OcrResult ocr_results = m_ocr_lite.detect(image,
        50, 0,
        0.2f, 0.3f,
        2.0f, false, false);

    std::vector<TextArea> result;
    for (TextBlock& text_block : ocr_results.textBlocks) {
        if (text_block.boxPoint.size() != 4) {
            continue;
        }
        // the rect like ↓
        // 0 - 1
        // 3 - 2
        int x = text_block.boxPoint.at(0).x;
        int y = text_block.boxPoint.at(0).y;
        int width = text_block.boxPoint.at(1).x - x;
        int height = text_block.boxPoint.at(3).y - y;
        result.emplace_back(std::move(text_block.text), x, y, width, height);
    }
    return result;
}

std::pair<double, cv::Point> Identify::match_template(const cv::Mat& image, const cv::Mat& templ)
{
    Mat image_hsv;
    Mat templ_hsv;
    cvtColor(image, image_hsv, COLOR_BGR2HSV);
    cvtColor(templ, templ_hsv, COLOR_BGR2HSV);

    Mat matched;
    matchTemplate(image_hsv, templ_hsv, matched, cv::TM_CCOEFF_NORMED);

    double minVal = 0, maxVal = 0;
    cv::Point minLoc, maxLoc;
    minMaxLoc(matched, &minVal, &maxVal, &minLoc, &maxLoc);
    return { maxVal, maxLoc };
}

asst::Identify::FindImageResult asst::Identify::find_image(
    const cv::Mat& image, const std::string& templ_name, double add_cache_thres, bool rect_zoom)
{
    if (m_mat_map.find(templ_name) == m_mat_map.cend()) {
        return { AlgorithmType::JustReturn, 0, Rect() };
    }

    // 有缓存，用直方图比较，CPU占用会低很多，但要保证每次按钮图片的位置不变
    if (m_cache_map.find(templ_name) != m_cache_map.cend()) {
        const auto& [raw_rect, hist] = m_cache_map.at(templ_name);
        double value = image_hist_comp(image(raw_rect), hist);
        Rect dst_rect = make_rect<asst::Rect>(raw_rect);
        if (rect_zoom) {
            dst_rect = dst_rect.center_zoom(0.8);
        }
        return { AlgorithmType::CompareHist, value, dst_rect };
    }
    else {	// 没缓存就模板匹配
        const cv::Mat& templ_mat = m_mat_map.at(templ_name);
        const auto& [value, point] = match_template(image, templ_mat);
        cv::Rect raw_rect(point.x, point.y, templ_mat.cols, templ_mat.rows);
        if (value >= add_cache_thres) {
            m_cache_map.emplace(templ_name, std::make_pair(raw_rect, image_2_hist(image(raw_rect))));
        }
        Rect dst_rect = make_rect<asst::Rect>(raw_rect);
        if (rect_zoom) {
            dst_rect = dst_rect.center_zoom(0.8);
        }
        return { AlgorithmType::MatchTemplate, value, dst_rect };
    }
}

std::vector<asst::Identify::FindImageResult> asst::Identify::find_all_images(
    const cv::Mat& image, const std::string& templ_name, double threshold, bool rect_zoom) const
{
    if (m_mat_map.find(templ_name) == m_mat_map.cend()) {
        return std::vector<FindImageResult>();
    }
    const cv::Mat& templ_mat = m_mat_map.at(templ_name);

    Mat image_hsv;
    Mat templ_hsv;
    cvtColor(image, image_hsv, COLOR_BGR2HSV);
    cvtColor(templ_mat, templ_hsv, COLOR_BGR2HSV);

    Mat matched;
    matchTemplate(image_hsv, templ_hsv, matched, cv::TM_CCOEFF_NORMED);

    std::vector<FindImageResult> results;
    int mini_distance = (std::min)(templ_mat.cols, templ_mat.rows) * 0.5;
    for (int i = 0; i != matched.rows; ++i) {
        for (int j = 0; j != matched.cols; ++j) {
            auto value = matched.at<float>(i, j);
            if (value >= threshold) {
                Rect rect = Rect(j, i, templ_mat.cols, templ_mat.rows);
                if (rect_zoom) {
                    rect = rect.center_zoom(0.8);
                }

                bool need_push = true;
                // 如果有两个点离得太近，只取里面得分高的那个
                // 一般相邻的都是刚刚push进去的，这里倒序快一点
                for (auto iter = results.rbegin(); iter != results.rend(); ++iter) {
                    if (std::abs(j - iter->rect.x) < mini_distance
                        && std::abs(i - iter->rect.y) < mini_distance) {
                        if (iter->score < value) {
                            iter->rect = rect;
                            iter->score = value;
                        }	// else 这个点就放弃了
                        need_push = false;
                        break;
                    }
                }
                if (need_push) {
                    results.emplace_back(AlgorithmType::MatchTemplate, value, std::move(rect));
                }
            }
        }
    }

    std::sort(results.begin(), results.end(), [](const auto& lhs, const auto& rhs) -> bool {
        return lhs.score > rhs.score;
        });

#ifdef LOG_TRACE
    cv::Mat draw_mat = image.clone();
    for (const auto& info : results) {
        cv::rectangle(draw_mat, make_rect<cv::Rect>(info.rect), cv::Scalar(0, 0, 255), 1);
        cv::putText(draw_mat, std::to_string(info.score), cv::Point(info.rect.x, info.rect.y), 1, 1.0, cv::Scalar(0, 0, 255));
    }
#endif

    return results;
}

void Identify::clear_cache()
{
    m_cache_map.clear();
}

// gpu_index是ncnn框架的参数，现在换了onnx的，已经没有这个参数了，但是为了保持接口一致性，保留这个参数，实际不起作用
void asst::Identify::set_ocr_param(int gpu_index, int number_thread)
{
    m_ocr_lite.setNumThread(number_thread);
}

bool asst::Identify::ocr_init_models(const std::string& dir)
{
    constexpr static const char* DetName = "dbnet.onnx";
    constexpr static const char* ClsName = "angle_net.onnx";
    constexpr static const char* RecName = "crnn_lite_lstm.onnx";
    constexpr static const char* KeysName = "keys.txt";

    const std::string dst_filename = dir + DetName;
    const std::string cls_filename = dir + ClsName;
    const std::string rec_filename = dir + RecName;
    const std::string keys_filename = dir + KeysName;

    return m_ocr_lite.initModels(dst_filename, cls_filename, rec_filename, keys_filename);
}

std::optional<asst::Rect> asst::Identify::find_text(const cv::Mat& image, const std::string& text)
{
    std::vector<TextArea> results = ocr_detect(image);
    for (const TextArea& res : results) {
        if (res.text == text) {
            return res.rect;
        }
    }
    return std::nullopt;
}

std::vector<TextArea> asst::Identify::find_text(const cv::Mat& image, const std::vector<std::string>& texts)
{
    std::vector<TextArea> dst;
    std::vector<TextArea> detect_result = ocr_detect(image);
    for (TextArea& res : detect_result) {
        for (const std::string& t : texts) {
            if (res.text == t) {
                dst.emplace_back(std::move(res));
            }
        }
    }
    return dst;
}

std::vector<TextArea> asst::Identify::find_text(const cv::Mat& image, const std::unordered_set<std::string>& texts)
{
    std::vector<TextArea> dst;
    std::vector<TextArea> detect_result = ocr_detect(image);
    for (TextArea& res : detect_result) {
        for (const std::string& t : texts) {
            if (res.text == t) {
                dst.emplace_back(std::move(res));
            }
        }
    }
    return dst;
}

bool asst::Identify::penguin_load_server(const std::string& server)
{
    m_penguin_server = server;
    penguin::load_server(server.c_str());
    return true;
}

bool asst::Identify::penguin_load_json(const std::string& stage_path, const std::string& hash_path)
{
    auto load_file = [](const std::string& path) -> std::string {
        std::ifstream ifs(path, std::ios::in);
        if (!ifs.is_open()) {
            return std::string();
        }
        std::stringstream iss;
        iss << ifs.rdbuf();
        ifs.close();
        return iss.str();
    };

    auto stage_parse_ret = json::parse(load_file(stage_path));
    if (!stage_parse_ret) {
        return false;
    }
    // stage_json 来自 https://penguin-stats.io/PenguinStats/api/v2/stages
    // 和接口需要的json有点区别，这里做个转换
    json::value stage_json = std::move(stage_parse_ret.value());
    json::object cvt_stage_json;
    try {
        for (const json::value& stage_info : stage_json.as_array()) {
            if (!stage_info.exist("dropInfos")) {   // 这种一般是以前的活动关，现在已经关闭了的
                continue;
            }
            std::string key = stage_info.at("code").as_string();
            json::value stage_dst;
            stage_dst["stageId"] = stage_info.at("stageId");
            std::vector<json::value> drops_vector;
            for (const json::value& drop_info : stage_info.at("dropInfos").as_array()) {
                if (drop_info.exist("itemId")) {
                    if (drop_info.at("dropType").as_string() == "FURNITURE") {  // 幸运掉落，家具啥的，企鹅物流不接，忽略掉
                        continue;
                    }
                    drops_vector.emplace_back(drop_info.at("itemId"));
                }
            }
            stage_dst["drops"] = json::array(std::move(drops_vector));
            std::string server = m_penguin_server.empty() ? "CN" : m_penguin_server;
            stage_dst["existence"] = stage_info.at("existence").at(server).at("exist");

            cvt_stage_json.emplace(std::move(key), std::move(stage_dst));
        }
    }
    catch (json::exception& e) {
        DebugTraceError(stage_path, "parse error", e.what());
        return false;
    }
    std::string cvt_stage_string = cvt_stage_json.to_string();
    //DebugTrace("stage_index | ", cvt_stage_string);
    penguin::load_json(cvt_stage_string.c_str(), load_file(hash_path).c_str());
    return true;
}

bool asst::Identify::penguin_load_templ(const std::string& item_id, const std::string& path)
{
    cv::Mat image = cv::imread(path);

    std::vector<uchar> buf;
    cv::imencode(".png", image, buf);

    penguin::load_templ(item_id.c_str(), buf.data(), buf.size());

    return true;
}

std::string asst::Identify::penguin_recognize(const cv::Mat& image)
{
    std::vector<uchar> buf;
    cv::imencode(".png", image, buf);
    std::string res = penguin::recognize(buf.data(), buf.size());
    DebugTrace("penguin_recognize | ", res);
    return res;
}

/*
std::pair<double, asst::Rect> Identify::findImageWithFile(const cv::Mat& cur, const std::string& filename)
{
    Mat image = imread(filename);
    if (image.empty()) {
        return { 0, asst::Rect() };
    }
    return findImage(cur, image);
}
*/