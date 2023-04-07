#include "OcrPack.h"

#include <filesystem>

#include "Utils/NoWarningCV.h"
ASST_SUPPRESS_CV_WARNINGS_START
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/ppocr_v3.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
ASST_SUPPRESS_CV_WARNINGS_END

#include "Config/GeneralConfig.h"
#include "Utils/Demangle.hpp"
#include "Utils/File.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include "Utils/Ranges.hpp"
#include "Utils/StringMisc.hpp"

asst::OcrPack::OcrPack() : m_det(nullptr), m_rec(nullptr), m_ocr(nullptr)
{
    LogTraceFunction;
}

asst::OcrPack::~OcrPack()
{
    LogTraceFunction;
}

bool asst::OcrPack::load(const std::filesystem::path& path)
{
    LogTraceFunction;
    Log.info("load", path);

    auto& paddle_dir = path;

    fastdeploy::RuntimeOption det_option;
    det_option.UseOrtBackend();
    bool build_det = false;

    fastdeploy::RuntimeOption rec_option;
    rec_option.UseOrtBackend();
    std::string rec_label;
    bool build_rec = false;

    do {
        using namespace asst::utils::path_literals;
        const auto det_dir = paddle_dir / "det"_p;
        const auto det_model_file = det_dir / "inference.onnx"_p;
        const auto rec_dir = paddle_dir / "rec"_p;
        const auto rec_model_file = rec_dir / "inference.onnx"_p;
        const auto rec_label_file = rec_dir / "keys.txt"_p;

        // fastdeploy 同时加载两个实例有可能会炸，不知道为啥，加个锁
        static std::mutex load_mutex;

        if (std::filesystem::exists(det_model_file)) {
            auto det_model = asst::utils::read_file<std::string>(det_model_file);
            det_option.SetModelBuffer(det_model.data(), det_model.size(), nullptr, 0, fastdeploy::ModelFormat::ONNX);
            build_det = true;
        }

        if (std::filesystem::exists(rec_model_file) && std::filesystem::exists(rec_label_file)) {
            auto rec_model = asst::utils::read_file<std::string>(rec_model_file);
            rec_label = asst::utils::read_file<std::string>(rec_label_file);
            rec_option.SetModelBuffer(rec_model.data(), rec_model.size(), nullptr, 0, fastdeploy::ModelFormat::ONNX);
            build_rec = true;
        }

        if (build_det) {
            std::unique_lock<std::mutex> lock(load_mutex);
            m_det = std::make_unique<fastdeploy::vision::ocr::DBDetector>("dummy.onnx", std::string(), det_option,
                                                                          fastdeploy::ModelFormat::ONNX);
        }

        if (build_rec) {
            std::unique_lock<std::mutex> lock(load_mutex);
            m_rec = std::make_unique<fastdeploy::vision::ocr::Recognizer>("dummy.onnx", std::string(), rec_label,
                                                                          rec_option, fastdeploy::ModelFormat::ONNX);
        }

        if (m_det && m_rec) {
            m_ocr = std::make_unique<fastdeploy::pipeline::PPOCRv3>(m_det.get(), m_rec.get());
        }
        else {
            break;
        }
    } while (false);

    return m_det != nullptr && m_rec != nullptr && m_ocr != nullptr && m_det->Initialized() && m_rec->Initialized() &&
           m_ocr->Initialized();
}

std::vector<asst::TextRect> asst::OcrPack::recognize(const cv::Mat& image, const Rect& roi,
                                                     const asst::TextRectProc& pred, bool without_det, bool trim)
{
    auto rect_cor = [&roi, &pred, &without_det](TextRect& tr) -> bool {
        if (without_det) {
            tr.rect = roi;
        }
        else {
            tr.rect.x += roi.x;
            tr.rect.y += roi.y;
        }
        return pred(tr);
    };
    Log.trace("OcrPack::recognize | roi:", roi);
    cv::Mat roi_img = image(make_rect<cv::Rect>(roi));
    return recognize(roi_img, rect_cor, without_det, trim);
}

std::vector<asst::TextRect> asst::OcrPack::recognize(const cv::Mat& image, const asst::TextRectProc& pred,
                                                     bool without_det, bool trim)
{
    std::string class_type = utils::demangle(typeid(*this).name());
    fastdeploy::vision::OCRResult ocr_result;
    if (!without_det) {
        LogTraceScope("Ocr Pipeline with " + class_type);
        m_ocr->Predict(image, &ocr_result);
    }
    else {
        LogTraceScope("Ocr Rec with " + class_type);
        std::string rec_text;
        float rec_score = 0;
        m_rec->Predict(image, &rec_text, &rec_score);
        ocr_result.text.emplace_back(std::move(rec_text));
        ocr_result.rec_scores.emplace_back(rec_score);
    }

#ifdef ASST_DEBUG
    cv::Mat draw = image.clone();
#endif

    std::vector<TextRect> raw_result;
    std::vector<TextRect> proced_result;
    for (size_t i = 0; i != ocr_result.text.size(); ++i) {
        // the box rect like ↓
        // 0 - 1
        // 3 - 2
        Rect det_rect;
        if (!without_det && i < ocr_result.boxes.size()) {
            const auto& box = ocr_result.boxes.at(i);
            int x_collect[] = { box[0], box[2], box[4], box[6] };
            int y_collect[] = { box[1], box[3], box[5], box[7] };
            auto [left, right] = ranges::minmax(x_collect);
            auto [top, bottom] = ranges::minmax(y_collect);
            det_rect = Rect(left, top, right - left, bottom - top);
        }
        else {
            det_rect = Rect(0, 0, image.cols, image.rows);
        }

#ifdef ASST_DEBUG
        cv::rectangle(draw, make_rect<cv::Rect>(det_rect), cv::Scalar(0, 0, 255), 2);
#endif

        double score = ocr_result.rec_scores.at(i);
        if (score > 2.0) {
            score = 0;
        }
        TextRect tr(score, det_rect, std::move(ocr_result.text.at(i)));
        raw_result.emplace_back(tr);
        if (trim) {
            utils::string_trim(tr.text);
        }
        if (!pred || pred(tr)) {
            proced_result.emplace_back(std::move(tr));
        }
    }

    Log.trace("OcrPack::recognize | raw:", raw_result);
    Log.trace("OcrPack::recognize | proc:", proced_result);
    return proced_result;
}
