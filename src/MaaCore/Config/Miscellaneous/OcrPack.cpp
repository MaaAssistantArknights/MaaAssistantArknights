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
#include "Utils/Ranges.hpp"
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
    : m_ocr_option(std::make_unique<fastdeploy::RuntimeOption>()), m_det(nullptr), m_rec(nullptr), m_ocr(nullptr)
{
    LogTraceFunction;
    m_ocr_option->UseOrtBackend();
}

asst::OcrPack::~OcrPack()
{
    LogTraceFunction;
}

bool asst::OcrPack::load(const std::filesystem::path& path)
{
    LogTraceFunction;
    Log.info("load", path);

    bool use_temp_dir = false;
    auto paddle_dir = prepare_paddle_dir(path, &use_temp_dir);

    if (paddle_dir.empty() || !std::filesystem::exists(paddle_dir)) {
        return false;
    }

    do {
        using namespace asst::utils::path_literals;
        const auto det_dir = paddle_dir / "det"_p;
        const auto dst_model_file = det_dir / "inference.onnx"_p;
        // const auto dst_params_file = det_dir / "inference.pdiparams"_p;
        const auto rec_dir = paddle_dir / "rec"_p;
        const auto rec_model_file = rec_dir / "inference.onnx"_p;
        // const auto rec_params_file = rec_dir / "inference.pdiparams"_p;
        const auto rec_label_file = rec_dir / "keys.txt"_p;

        if (std::filesystem::exists(dst_model_file)) {
            m_det = std::make_unique<fastdeploy::vision::ocr::DBDetector>(
                asst::utils::path_to_ansi_string(dst_model_file), std::string(), *m_ocr_option,
                fastdeploy::ModelFormat::ONNX);
        }
        else if (!m_det) {
            break;
        }
        // else 沿用原来的模型

        if (std::filesystem::exists(rec_model_file) && std::filesystem::exists(rec_label_file)) {
            m_rec = std::make_unique<fastdeploy::vision::ocr::Recognizer>(
                asst::utils::path_to_ansi_string(rec_model_file), std::string(),
                asst::utils::path_to_ansi_string(rec_label_file), *m_ocr_option, fastdeploy::ModelFormat::ONNX);
        }
        else if (!m_rec) {
            break;
        }
        // else 沿用原来的模型

        if (m_det && m_rec) {
            m_ocr = std::make_unique<fastdeploy::pipeline::PPOCRv3>(m_det.get(), m_rec.get());
        }
        else {
            break;
        }
    } while (false);

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

#ifdef _WIN32

static std::filesystem::path prepare_paddle_dir(const std::filesystem::path& dir, bool* is_temp)
{
    static std::atomic<uint64_t> id {};

    *is_temp = false;
    auto is_ascii = [](const std::filesystem::path& path) {
        return asst::ranges::all_of(path.wstring(), [](auto ch) { return ch < 127; });
    };

    if (is_ascii(dir)) {
        // can be passed to paddle via path_to_ansi_string
        return dir;
    }
    // fallback: create junction (reparse point) in user temp directory
    std::filesystem::path tempdir = std::filesystem::temp_directory_path();
    if (!is_ascii(tempdir)) {
        // 临时目录（如 C:\Users\XXX\AppData\Local\Temp）也是中文路径，无法使用
        asst::Log.warn("failed to escape unicode path: temp dir cannot be escaped", tempdir);
        // 只能随地拉屎了。这里使用盘符根目录
        tempdir = dir.root_path();
    }

    auto pid = GetCurrentProcessId();
    while (true) {
        auto dirname = std::format(L"MaaLink-{}-{}", pid, id++);
        auto linkdir = tempdir / dirname;
        asst::Log.info("tempdir", tempdir, "linkdir", linkdir);

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
