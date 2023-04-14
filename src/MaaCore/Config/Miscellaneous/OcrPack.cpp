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

    fastdeploy::RuntimeOption option;
    option.UseOrtBackend();

    using namespace asst::utils::path_literals;
    const auto det_dir = path / "det"_p;
    const auto det_model_file = det_dir / "inference.onnx"_p;

    if (std::filesystem::exists(det_model_file)) {
        auto det_model = asst::utils::read_file<std::string>(det_model_file);
        option.SetModelBuffer(det_model.data(), det_model.size(), nullptr, 0, fastdeploy::ModelFormat::ONNX);
        m_det = std::make_unique<fastdeploy::vision::ocr::DBDetector>("dummy.onnx", std::string(), option,
                                                                      fastdeploy::ModelFormat::ONNX);
    }

    const auto rec_dir = path / "rec"_p;
    const auto rec_model_file = rec_dir / "inference.onnx"_p;
    const auto rec_label_file = rec_dir / "keys.txt"_p;

    if (std::filesystem::exists(rec_model_file) && std::filesystem::exists(rec_label_file)) {
        auto rec_model = asst::utils::read_file<std::string>(rec_model_file);
        std::string rec_label = asst::utils::read_file<std::string>(rec_label_file);
        option.SetModelBuffer(rec_model.data(), rec_model.size(), nullptr, 0, fastdeploy::ModelFormat::ONNX);
        m_rec = std::make_unique<fastdeploy::vision::ocr::Recognizer>("dummy.onnx", std::string(), rec_label, option,
                                                                      fastdeploy::ModelFormat::ONNX);
    }

    if (m_det && m_rec) {
        m_ocr = std::make_unique<fastdeploy::pipeline::PPOCRv3>(m_det.get(), m_rec.get());
    }

    return m_det != nullptr && m_rec != nullptr && m_ocr != nullptr && m_det->Initialized() && m_rec->Initialized() &&
           m_ocr->Initialized();
}

asst::OcrPack::ResultsVec asst::OcrPack::recognize(const cv::Mat& image, bool without_det)
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
            .text = std::move(ocr_result.text.at(i)),
            .rect = det_rect,
            .score = ocr_result.rec_scores.at(i),
        };
        raw_results.emplace_back(std::move(result));
    }

    Log.trace("OcrPack::recognize | raw:", raw_results);
    return raw_results;
}
