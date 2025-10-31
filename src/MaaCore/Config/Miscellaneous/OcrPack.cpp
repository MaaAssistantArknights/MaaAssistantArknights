#include "OcrPack.h"

#include <filesystem>

#include "MaaUtils/NoWarningCV.hpp"
ASST_SUPPRESS_CV_WARNINGS_START
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/ppocr_v3.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
ASST_SUPPRESS_CV_WARNINGS_END

#include "Utils/Demangle.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include "Utils/StringMisc.hpp"
#include <ranges>

asst::OcrPack::OcrPack() :
    m_det(nullptr),
    m_rec(nullptr),
    m_ocr(nullptr)
{
    LogTraceFunction;
}

asst::OcrPack::~OcrPack()
{
    LogTraceFunction;
    if (m_gpu_id) {
        // FIXME: leak fastdeploy objects to avoid crash (double free?)
        (void)m_det.release();
        (void)m_rec.release();
        (void)m_ocr.release();
    }
}

bool asst::OcrPack::load(const std::filesystem::path& path)
{
    LogTraceFunction;
    Log.info("load", path.lexically_relative(UserDir.get()));

    using namespace asst::utils::path_literals;
    const auto det_dir = path / "det"_p;
    const auto det_model_file = det_dir / "inference.onnx"_p;

    if (std::filesystem::exists(det_model_file) && m_det_model_path != det_model_file) {
        m_det_model_path = det_model_file;
        m_det = nullptr;
    }

    const auto rec_dir = path / "rec"_p;
    const auto rec_model_file = rec_dir / "inference.onnx"_p;
    const auto rec_label_file = rec_dir / "keys.txt"_p;

    if (std::filesystem::exists(rec_model_file) && m_rec_model_path != rec_model_file) {
        m_rec_model_path = rec_model_file;
        m_rec = nullptr;
    }
    if (std::filesystem::exists(rec_label_file) && m_rec_model_path != rec_label_file) {
        m_rec_label_path = rec_label_file;
        m_rec = nullptr;
    }

    if (m_det && m_rec) {
        m_ocr = std::make_unique<fastdeploy::pipeline::PPOCRv3>(m_det.get(), m_rec.get());
    }

    return !m_det_model_path.empty() && !m_rec_model_path.empty() && !m_rec_label_path.empty();
}

asst::OcrPack::ResultsVec asst::OcrPack::recognize(const cv::Mat& image, bool without_det)
{
    if (!check_and_load()) {
        Log.error(__FUNCTION__, "check_and_load failed");
        return {};
    }

    fastdeploy::vision::OCRResult ocr_result;

    auto start_time = std::chrono::steady_clock::now();
    if (!without_det) {
        m_ocr->Predict(image, &ocr_result);
    }
    else {
        std::string rec_text;
        float rec_score = 0;
        m_rec->Predict(image, &rec_text, &rec_score);
#ifdef ASST_DEBUG
        // zzyyyl 注: RelWithDebInfo 时 OCR 莫名很卡，简单查了一下发现主要是这里的
        // _com_error 很多导致的，暂时把 std::move 去掉
        ocr_result.text.emplace_back(rec_text);
#else
        ocr_result.text.emplace_back(std::move(rec_text));
#endif
        ocr_result.rec_scores.emplace_back(rec_score);
    }

#ifdef ASST_DEBUG
    cv::Mat draw = image.clone();
#endif

    ResultsVec raw_results;
    for (size_t i = 0; i != ocr_result.text.size(); ++i) {
        // the box rect like ↓
        // 0 - 1
        // 3 - 2
        Rect det_rect;
        if (!without_det && i < ocr_result.boxes.size()) {
            const auto& box = ocr_result.boxes.at(i);
            int x_collect[] = { box[0], box[2], box[4], box[6] };
            int y_collect[] = { box[1], box[3], box[5], box[7] };
            auto [left, right] = std::ranges::minmax(x_collect);
            auto [top, bottom] = std::ranges::minmax(y_collect);
            det_rect = Rect(left, top, right - left, bottom - top);
        }
        else {
            det_rect = Rect(0, 0, image.cols, image.rows);
        }

#ifdef ASST_DEBUG
        cv::rectangle(draw, make_rect<cv::Rect>(det_rect), cv::Scalar(0, 0, 255), 2);
#endif
        Result result {
            .rect = det_rect,
            .score = ocr_result.rec_scores.at(i),
            .text = std::move(ocr_result.text.at(i)),
        };
        raw_results.emplace_back(std::move(result));
    }

    auto costs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
    std::string class_type = utils::demangle(typeid(*this).name());
    Log.trace(class_type, raw_results, without_det ? "by OCR Rec" : "by OCR Pipeline", ", cost", costs, "ms");
    return raw_results;
}

bool asst::OcrPack::check_and_load()
{
    if (m_det && m_rec) {
        return true;
    }

    LogTraceFunction;

    fastdeploy::RuntimeOption det_option;
    fastdeploy::RuntimeOption rec_option;
    det_option.UseOrtBackend();
    rec_option.UseOrtBackend();

#ifdef _WIN32
    if (m_gpu_id) {
        det_option.UseDirectML(*m_gpu_id);
        rec_option.UseDirectML(*m_gpu_id);
    }
#elif defined(__APPLE__)
    // https://github.com/microsoft/onnxruntime/blob/main/include/onnxruntime/core/providers/coreml/coreml_provider_factory.h
    // COREML_FLAG_ONLY_ENABLE_DEVICE_WITH_ANE
    det_option.UseCoreML(0x004);
    // rec 结果不对，先禁用
    rec_option.UseCpu();
#else
    det_option.UseCpu();
    rec_option.UseCpu();
#endif

    m_det = std::make_unique<fastdeploy::vision::ocr::DBDetector>(
        platform::path_to_utf8_string(m_det_model_path),
        std::string(),
        det_option,
        fastdeploy::ModelFormat::ONNX);

    m_rec = std::make_unique<fastdeploy::vision::ocr::Recognizer>(
        platform::path_to_utf8_string(m_rec_model_path),
        std::string(),
        platform::path_to_utf8_string(m_rec_label_path),
        rec_option,
        fastdeploy::ModelFormat::ONNX);

    if (m_det && m_rec) {
        m_ocr = std::make_unique<fastdeploy::pipeline::PPOCRv3>(m_det.get(), m_rec.get());
    }

    bool det_inited = m_det && m_det->Initialized();
    bool rec_inited = m_rec && m_rec->Initialized();
    bool ocr_inited = m_ocr && m_ocr->Initialized();

    Log.info("det", det_inited, "rec", rec_inited, "ocr", ocr_inited);

    return det_inited && rec_inited && ocr_inited;
}
