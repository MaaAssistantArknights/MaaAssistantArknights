#include "OcrPack.h"

#include <PaddleOCR/paddle_ocr.h>
#include <opencv2/opencv.hpp>

#include "AsstUtils.hpp"
#include "Logger.hpp"

asst::OcrPack::~OcrPack()
{
    PaddleOcrDestroy(m_ocr);
}

bool asst::OcrPack::load(const std::string& dir)
{
    constexpr static const char* DetName = "/det";
    //constexpr static const char* ClsName = "/cls";
    constexpr static const char* RecName = "/rec";
    constexpr static const char* KeysName = "/ppocr_keys_v1.txt";

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

std::vector<asst::TextRect> asst::OcrPack::recognize(const cv::Mat image, const asst::TextRectProc& pred, bool without_det)
{
    LogTraceFunction;

    std::vector<uchar> buf;
    cv::imencode(".png", image, buf);

    constexpr static size_t MaxBoxSize = 128;
    // each box has 8 value ( 4 points, x and y )
    int boxes[MaxBoxSize * 8] = { 0 };
    char* strs[MaxBoxSize] = { 0 };
    for (size_t i = 0; i != MaxBoxSize; ++i) {
        constexpr static size_t MaxTextSize = 1024;
        *(strs + i) = new char[MaxTextSize];
        memset(*(strs + i), 0, MaxTextSize);
    }
    float scores[MaxBoxSize] = { 0 };
    size_t size = 0;

    Log.trace("Without Det:", without_det);
    if (!without_det) {
        PaddleOcrSystem(m_ocr, buf.data(), buf.size(), false,
            boxes, strs, scores, &size, nullptr, nullptr);
    }
    else {
        PaddleOcrRec(m_ocr, buf.data(), buf.size(),
            strs, scores, &size, nullptr, nullptr);
    }

    std::vector<TextRect> result;
    std::string log_str_raw;
    std::string log_str_proc;
    for (size_t i = 0; i != size; ++i) {
        // the box rect like ↓
        // 0 - 1
        // 3 - 2
        Rect rect;
        if (!without_det) {
            int* box = boxes + i * 8;
            int x_collect[4] = { *(box + 0), *(box + 2), *(box + 4), *(box + 6) };
            int y_collect[4] = { *(box + 1), *(box + 3), *(box + 5), *(box + 7) };
            int left = int(*std::min_element(x_collect, x_collect + 4));
            int right = int(*std::max_element(x_collect, x_collect + 4));
            int top = int(*std::min_element(y_collect, y_collect + 4));
            int bottom = int(*std::max_element(y_collect, y_collect + 4));
            rect = Rect(left, top, right - left, bottom - top);
        }
        std::string text(*(strs + i));
        float score = *(scores + i);

        TextRect tr{ text, rect, score };

        log_str_raw += tr.to_string() + ", ";
        if (!pred || pred(tr)) {
            log_str_proc += tr.to_string() + ", ";
            result.emplace_back(std::move(tr));
        }
    }
    for (size_t i = 0; i != MaxBoxSize; ++i) {
        delete[] * (strs + i);
    }

    Log.trace("OcrPack::recognize | raw : ", log_str_raw);
    Log.trace("OcrPack::recognize | proc : ", log_str_proc);
    return result;
}

std::vector<asst::TextRect> asst::OcrPack::recognize(const cv::Mat image, const asst::Rect& roi, const asst::TextRectProc& pred, bool without_det)
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
    return recognize(image(utils::make_rect<cv::Rect>(roi)), rect_cor, without_det);
}
