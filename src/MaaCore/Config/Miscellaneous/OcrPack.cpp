#include "OcrPack.h"

#include <algorithm>
#include <filesystem>
#include <thread>

#include "MaaUtils/NoWarningCV.hpp"
MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/ppocr_v3.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
MAA_SUPPRESS_CV_WARNINGS_END

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

asst::OcrPack::ResultsVec asst::OcrPack::recognize(const cv::Mat& image, bool without_det, const Rect& roi)
{
    if (!check_and_load()) {
        Log.error(__FUNCTION__, "check_and_load failed");
        return {};
    }

    fastdeploy::vision::OCRResult ocr_result;

    // 注意：使用 DirectML 时，调试器下会出现大量 _com_error 异常
    // 不影响程序运行但会导致调试器频繁中断，造成响应停顿
    // 即使禁用 C++ 的 "_com_error" 异常中断，调试器仍可能频繁中断
    // 推荐在挂调试器的情况下禁用 GPU 推理
    // 或等待两轮 Predict 卡个几十秒之后正常运行
    auto start_time = std::chrono::steady_clock::now();
    if (!without_det) {
        m_ocr->Predict(image, &ocr_result);
    }
    else {
        std::string rec_text;
        float rec_score = 0;
        m_rec->Predict(image, &rec_text, &rec_score);
        ocr_result.text.emplace_back(std::move(rec_text));
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
        Rect abs_rect;
        Rect det_rect;
        if (i < ocr_result.boxes.size()) {
            const auto& box = ocr_result.boxes.at(i);
            int x_collect[] = { box[0], box[2], box[4], box[6] };
            int y_collect[] = { box[1], box[3], box[5], box[7] };
            auto [left, right] = std::ranges::minmax(x_collect);
            auto [top, bottom] = std::ranges::minmax(y_collect);
            abs_rect = Rect(left, top, right - left, bottom - top);
        }
        
        det_rect = Rect(0, 0, image.cols, image.rows);
        
#ifdef ASST_DEBUG
        cv::rectangle(draw, make_rect<cv::Rect>(det_rect), cv::Scalar(0, 0, 255), 2);
#endif
        raw_results.emplace_back(Result(abs_rect, det_rect, ocr_result.rec_scores.at(i), std::move(ocr_result.text.at(i))));
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

    int logical = std::max(1u, std::thread::hardware_concurrency());
    int cpu_threads;
    if (logical <= 2) {
        cpu_threads = 1;
    }
    else if (logical <= 4) {
        cpu_threads = 2;
    }
    else if (logical <= 12) {
        cpu_threads = 3;
    }
    else {
        cpu_threads = 4;
    }

    fastdeploy::RuntimeOption det_option;
    fastdeploy::RuntimeOption rec_option;
    det_option.UseOrtBackend();
    rec_option.UseOrtBackend();

#ifdef _WIN32
    if (m_gpu_id) {
        det_option.UseDirectML(*m_gpu_id);
        rec_option.UseDirectML(*m_gpu_id);
    }
    else {
        // CPU 模式下限制线程数，避免过高的 CPU 占用
        det_option.SetCpuThreadNum(cpu_threads);
        rec_option.SetCpuThreadNum(cpu_threads);
        Log.info("FastDeploy CPU mode with", cpu_threads, "threads");
    }
#elif defined(__APPLE__)
    // rec 结果不对，先禁用
    // maafw那边用户反馈，det 貌似也不怎么对，疑似 coreml 丢精度了，拉倒
    // https://github.com/microsoft/onnxruntime/blob/main/include/onnxruntime/core/providers/coreml/coreml_provider_factory.h
    // COREML_FLAG_ONLY_ENABLE_DEVICE_WITH_ANE
    // det_option.UseCoreML(0x004);
    // rec_option.UseCoreML(0x004);
    det_option.UseCpu();
    rec_option.UseCpu();
    det_option.SetCpuThreadNum(cpu_threads);
    rec_option.SetCpuThreadNum(cpu_threads);
    Log.info("FastDeploy macOS mode with rec", cpu_threads, "CPU threads");
#else
    det_option.UseCpu();
    rec_option.UseCpu();
    det_option.SetCpuThreadNum(cpu_threads);
    rec_option.SetCpuThreadNum(cpu_threads);
    Log.info("FastDeploy CPU mode with", cpu_threads, "threads");
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
