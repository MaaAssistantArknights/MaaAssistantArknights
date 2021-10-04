#pragma once

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <tuple>
#include <optional>

#include <opencv2/opencv.hpp>
#include <OcrLiteOnnx/OcrLiteCaller.h>

#include "AsstDef.h"

namespace asst {
    class WinMacro;

    class Identify
    {
    public:
        struct FindImageResult {
            FindImageResult() = default;
            FindImageResult(AlgorithmType algorithm, double value, asst::Rect rect)
                : algorithm(algorithm), score(value), rect(rect) {
                ;
            }
            AlgorithmType algorithm;
            double score = 0.0;
            asst::Rect rect;
        };
    public:
        Identify() = default;
        ~Identify() = default;

        /*** OpenCV package ***/
        bool add_image(const std::string& name, const std::string& path);

        constexpr static double NotAddCache = 999.0;
        FindImageResult find_image(
            const cv::Mat& image, const std::string& templ_name, double add_cache_thres = NotAddCache, bool rect_zoom = true);
        std::vector<FindImageResult> find_all_images(
            const cv::Mat& image, const std::string& templ_name, double threshold = 0, bool rect_zoom = true) const;

        void clear_cache();

        /*** OcrLite package ***/
        void set_ocr_param(int gpu_index, int thread_number);
        bool ocr_init_models(const std::string& dir);

        std::vector<TextArea> ocr_detect(const cv::Mat& image);

        [[deprecated]] std::optional<Rect> find_text(const cv::Mat& image, const std::string& text);
        [[deprecated]] std::vector<TextArea> find_text(const cv::Mat& image, const std::vector<std::string>& texts);
        [[deprecated]] std::vector<TextArea> find_text(const cv::Mat& image, const std::unordered_set<std::string>& texts);

        /*** Penguin package ***/
        bool penguin_load_server(const std::string& server);
        bool penguin_load_json(const std::string& stage_path, const std::string& hash_path);
        bool penguin_load_templ(const std::string& item_id, const std::string& path);
        std::string penguin_recognize(const cv::Mat& image);

    private:
        cv::Mat image_2_hist(const cv::Mat& src);
        double image_hist_comp(const cv::Mat& src, const cv::MatND& hist);
        // return pair< suitability, raw opencv::point>
        std::pair<double, cv::Point> match_template(const cv::Mat& image, const cv::Mat& templ);

        std::unordered_map<std::string, cv::Mat> m_mat_map;
        std::unordered_map<std::string, std::pair<cv::Rect, cv::Mat>> m_cache_map;	// 位置、直方图缓存

        std::string m_penguin_server;

        OcrLiteCaller m_ocr_lite;
    };
}
