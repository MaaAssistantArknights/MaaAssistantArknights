#include "OcrPack.h"

#include <filesystem>

#include "Utils/NoWarningCV.h"
ASST_SUPPRESS_CV_WARNINGS_START
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/ppocr_v3.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
ASST_SUPPRESS_CV_WARNINGS_END

#include "Utils/Demangle.hpp"
#include "Utils/File.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include "Utils/Ranges.hpp"
#include "Utils/StringMisc.hpp"

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

    fastdeploy::RuntimeOption option;
    option.UseOrtBackend();
    if (m_gpu_id) {
        option.UseGpu(*m_gpu_id);
    }

    auto det_model = asst::utils::read_file<std::string>(m_det_model_path);
    option.SetModelBuffer(det_model.data(), det_model.size(), nullptr, 0, fastdeploy::ModelFormat::ONNX);
    m_det = std::make_unique<fastdeploy::vision::ocr::DBDetector>(
        "dummy.onnx",
        std::string(),
        option,
        fastdeploy::ModelFormat::ONNX);

    auto rec_model = asst::utils::read_file<std::string>(m_rec_model_path);
    std::string rec_label = asst::utils::read_file<std::string>(m_rec_label_path);
    option.SetModelBuffer(rec_model.data(), rec_model.size(), nullptr, 0, fastdeploy::ModelFormat::ONNX);
    m_rec = std::make_unique<fastdeploy::vision::ocr::Recognizer>(
        "dummy.onnx",
        std::string(),
        rec_label,
        option,
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
