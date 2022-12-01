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
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include "Utils/StringMisc.hpp"

#ifdef _WIN32
#include <format>
static std::filesystem::path prepare_paddle_dir(const std::filesystem::path& dir, bool* is_temp);
#else
static std::filesystem::path prepare_paddle_dir(const std::filesystem::path& dir, bool* is_temp)
{
    *is_temp = false;
    return dir;
}
#endif

asst::OcrPack::OcrPack()
    : m_ocr_option(std::make_unique<fastdeploy::RuntimeOption>()), m_det(nullptr), m_rec(nullptr), m_ocr(nullptr),
      m_backend(OcrBackend::ONNXRuntime)
{}

asst::OcrPack::~OcrPack() {}

void asst::OcrPack::set_backend_before_load(OcrBackend backend)
{
    m_backend = backend;
}

bool asst::OcrPack::load(const std::filesystem::path& path)
{
    bool use_temp_dir = false;
    auto paddle_dir = prepare_paddle_dir(path, &use_temp_dir);

    if (paddle_dir.empty() || !std::filesystem::exists(paddle_dir)) {
        return false;
    }

    using namespace asst::utils::path_literals;
    const auto dst_model_file = paddle_dir / "det"_p / "inference.pdmodel"_p;
    const auto dst_params_file = paddle_dir / "det"_p / "inference.pdiparams"_p;
    const auto rec_model_file = paddle_dir / "rec"_p / "inference.pdmodel"_p;
    const auto rec_params_file = paddle_dir / "rec"_p / "inference.pdiparams"_p;
    const auto rec_label_file = paddle_dir / "ppocr_keys_v1.txt"_p;

    if (!std::filesystem::exists(dst_model_file) || !std::filesystem::exists(dst_params_file) ||
        !std::filesystem::exists(rec_model_file) || !std::filesystem::exists(rec_params_file) ||
        !std::filesystem::exists(rec_label_file)) {
        return false;
    }

    switch (m_backend) {
    case OcrBackend::ONNXRuntime:
        Log.info("OcrBackend::ONNXRuntime");
        m_ocr_option->UseOrtBackend();
        break;
    case OcrBackend::PaddleInference:
        Log.info("OcrBackend::PaddleInference");
        m_ocr_option->UsePaddleInferBackend();
        break;
    default:
        Log.error("Unknown OCR Backend", static_cast<int>(m_backend));
        return false;
    }

    m_det = std::make_unique<fastdeploy::vision::ocr::DBDetector>(asst::utils::path_to_ansi_string(dst_model_file),
                                                                  asst::utils::path_to_ansi_string(dst_params_file),
                                                                  *m_ocr_option);
    m_rec = std::make_unique<fastdeploy::vision::ocr::Recognizer>(
        asst::utils::path_to_ansi_string(rec_model_file), asst::utils::path_to_ansi_string(rec_params_file),
        asst::utils::path_to_ansi_string(rec_label_file), *m_ocr_option);
    m_ocr = std::make_unique<fastdeploy::pipeline::PPOCRv3>(m_det.get(), m_rec.get());

    if (use_temp_dir) {
        // files can be removed after load
        std::thread([paddle_dir]() {
            for (int i = 0; i < 50; i++) {
                if (std::filesystem::remove(paddle_dir)) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }).detach();
    }

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
    // 如果是带 ROI 的 cv::Mat, data 仍是指向完整的图片数据，仅通过内部的一些其他参数标识 ROI
    // 直接取 data 拿到的不是正确的图，所以拷贝一份出来
    cv::Mat copied = image.clone();
    fastdeploy::vision::OCRResult ocr_result;
    if (!without_det) {
        LogTraceScope("Ocr System with " + class_type);

        m_ocr->Predict(&copied, &ocr_result);
    }
    else {
        LogTraceScope("Ocr Rec with " + class_type);

        std::vector rec_imgs = { std::move(copied) };
        std::vector<std::string> rec_texts;
        std::vector<float> rec_scores;
        m_rec->BatchPredict(rec_imgs, &rec_texts, &rec_scores);

        if (!rec_texts.empty()) {
            ocr_result.text.emplace_back(std::move(rec_texts.front()));
        }
        if (!rec_scores.empty()) {
            ocr_result.rec_scores.emplace_back(rec_scores.front());
        }
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
        std::string text = ocr_result.text.at(i);
        double score = ocr_result.rec_scores.at(i);
        if (score > 2.0) {
            score = 0;
        }

#ifdef ASST_DEBUG
        cv::rectangle(draw, make_rect<cv::Rect>(det_rect), cv::Scalar(0, 0, 255), 2);
#endif

        TextRect tr(score, det_rect, std::move(text));
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

#ifdef _WIN32

static std::filesystem::path prepare_paddle_dir(const std::filesystem::path& dir, bool* is_temp)
{
    static std::atomic<uint64_t> id {};

    *is_temp = false;
    if (!asst::utils::path_to_ansi_string(dir).empty()) {
        // can be passed to paddle via path_to_ansi_string
        return dir;
    }
    // fallback: create junction (reparse point) in user temp directory
    wchar_t tempbuf[MAX_PATH + 1];
    auto templen = GetTempPathW(MAX_PATH + 1, tempbuf);
    std::filesystem::path tempdir(std::wstring_view(tempbuf, templen));
    if (asst::utils::path_to_ansi_string(tempdir).empty()) {
        asst::Log.error("failed to escape unicode path: temp dir cannot be escaped");
        // cannot escape temp dir, no luck
        return {};
    }
    auto pid = GetCurrentProcessId();
    while (1) {
        auto dirname = std::format(L"MaaLink-{}-{}", pid, id++);
        auto linkdir = tempdir / dirname;
        if (CreateDirectoryW(linkdir.c_str(), nullptr)) {
            auto success = asst::win32::SetDirectoryReparsePoint(linkdir, dir);
            if (success) {
                *is_temp = true;
                return linkdir;
            }
            else {
                asst::Log.error("failed to escape unicode path: failed to create junction");
                return {};
            }
        }
        else {
            auto err = GetLastError();
            if (err == ERROR_ALREADY_EXISTS) {
                // try next id
                continue;
            }
            else {
                // cannot create link, no luck
                asst::Log.error("failed to escape unicode path: failed to create junction");
                return {};
            }
        }
    }
}

#endif
