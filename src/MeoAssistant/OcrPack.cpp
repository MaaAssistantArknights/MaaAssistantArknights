#include "OcrPack.h"

#include <filesystem>

#include "NoWarningCV.h"
#include <PaddleOCR/paddle_ocr.h>

#include "AsstUtils.hpp"
#include "Logger.hpp"

asst::OcrPack::OcrPack()
{
    Log.info("hardware_concurrency:", std::thread::hardware_concurrency());
    for (size_t i = 0; i != MaxBoxSize; ++i) {
        constexpr static size_t MaxTextSize = 1024;
        *(m_strs_buffer + i) = new char[MaxTextSize];
        //memset(*(m_strs_buffer + i), 0, MaxTextSize);
    }
}

asst::OcrPack::~OcrPack()
{
    for (size_t i = 0; i != MaxBoxSize; ++i) {
        delete[] * (m_strs_buffer + i);
    }

    PaddleOcrDestroy(m_ocr);
}

bool asst::OcrPack::load(const std::string& dir)
{
    LogTraceFunction;

    if (!std::filesystem::exists(dir)) {
        return false;
    }

    constexpr static auto DetName = "/det";
    //constexpr static const char* ClsName = "/cls";
    constexpr static auto RecName = "/rec";
    constexpr static auto KeysName = "/ppocr_keys_v1.txt";

    const std::string dst_filename = dir + DetName;
    //const std::string cls_filename = dir + ClsName;
    const std::string rec_filename = dir + RecName;
    const std::string keys_filename = dir + KeysName;

    if (m_ocr != nullptr) {
        PaddleOcrDestroy(m_ocr);
    }
    m_ocr = PaddleOcrCreate(dst_filename.c_str(), rec_filename.c_str(), keys_filename.c_str(), nullptr);

    return m_ocr != nullptr;
}

std::vector<asst::TextRect> asst::OcrPack::recognize(const cv::Mat& image, const asst::TextRectProc& pred, bool without_det)
{
    LogTraceFunction;

    size_t size = 0;

    std::vector<uchar> buf;
    cv::imencode(".png", image, buf);

    if (!without_det) {
        Log.trace("Ocr System");
        PaddleOcrSystem(m_ocr, buf.data(), buf.size(),
            false, m_boxes_buffer, m_strs_buffer, m_scores_buffer, &size, nullptr, nullptr);
    }
    else {
        Log.trace("Ocr Rec");
        PaddleOcrRec(m_ocr, buf.data(), buf.size(),
            m_strs_buffer, m_scores_buffer, &size, nullptr, nullptr);
    }

    std::vector<TextRect> result;
    std::string log_str_raw;
    std::string log_str_proc;

#ifdef ASST_DEBUG
    cv::Mat draw = image.clone();
#endif

    for (size_t i = 0; i != size; ++i) {
        // the box rect like ↓
        // 0 - 1
        // 3 - 2
        Rect rect;
        if (!without_det) {
            int* box = m_boxes_buffer + i * 8;
            int x_collect[4] = { *(box + 0), *(box + 2), *(box + 4), *(box + 6) };
            int y_collect[4] = { *(box + 1), *(box + 3), *(box + 5), *(box + 7) };
            int left = static_cast<int>(*std::min_element(x_collect, x_collect + 4));
            int right = static_cast<int>(*std::max_element(x_collect, x_collect + 4));
            int top = static_cast<int>(*std::min_element(y_collect, y_collect + 4));
            int bottom = static_cast<int>(*std::max_element(y_collect, y_collect + 4));
            rect = Rect(left, top, right - left, bottom - top);
        }
        std::string text(*(m_strs_buffer + i));
        float score = *(m_scores_buffer + i);
        if (score > 2.0) {
            score = 0;
        }

        TextRect tr{ score, rect, text };
#ifdef ASST_DEBUG
        cv::rectangle(draw, utils::make_rect<cv::Rect>(rect), cv::Scalar(0, 0, 255), 2);
#endif
        log_str_raw += tr.to_string() + ", ";
        if (!pred || pred(tr)) {
            log_str_proc += tr.to_string() + ", ";
            result.emplace_back(std::move(tr));
        }
    }

    Log.trace("OcrPack::recognize | raw : ", log_str_raw);
    Log.trace("OcrPack::recognize | proc : ", log_str_proc);
    return result;
}

std::vector<asst::TextRect> asst::OcrPack::recognize(const cv::Mat& image, const Rect& roi, const asst::TextRectProc& pred, bool without_det)
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
    Log.trace("OcrPack::recognize | roi : ", roi.to_string());
    cv::Mat roi_img = image(utils::make_rect<cv::Rect>(roi));
    return recognize(roi_img, rect_cor, without_det);
}
