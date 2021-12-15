#include "OcrPack.h"

#include <OcrLiteOnnx/OcrLiteCaller.h>
#include <opencv2/opencv.hpp>

#include "AsstUtils.hpp"
#include "Logger.hpp"

asst::OcrPack::OcrPack()
    : m_ocr_ptr(std::make_unique<OcrLiteCaller>())
{}

asst::OcrPack::~OcrPack() = default;

bool asst::OcrPack::load(const std::string & dir)
{
    constexpr static const char* DetName = "\\models\\dbnet.onnx";
    constexpr static const char* ClsName = "\\models\\angle_net.onnx";
    constexpr static const char* RecName = "\\models\\crnn_lite_lstm.onnx";
    constexpr static const char* KeysName = "\\models\\keys.txt";

    const std::string dst_filename = dir + DetName;
    const std::string cls_filename = dir + ClsName;
    const std::string rec_filename = dir + RecName;
    const std::string keys_filename = dir + KeysName;

    return m_ocr_ptr->initModels(dst_filename, cls_filename, rec_filename, keys_filename);
}

void asst::OcrPack::set_param(int /*gpu_index*/, int thread_number)
{
    // gpu_index是ncnn的参数，onnx架构的没有，预留参数接口
    m_ocr_ptr->setNumThread(thread_number);
}

std::vector<asst::TextRect> asst::OcrPack::recognize(const cv::Mat & image, const asst::TextRectProc & pred)
{
    constexpr int padding = 50;
    constexpr int maxSideLen = 0;
    constexpr float boxScoreThresh = 0.2f;
    constexpr float boxThresh = 0.3f;
    constexpr float unClipRatio = 2.0f;
    constexpr bool doAngle = false;
    constexpr bool mostAngle = false;

    OcrResult ocr_results = m_ocr_ptr->detect(image,
                                              padding, maxSideLen,
                                              boxScoreThresh, boxThresh,
                                              unClipRatio, doAngle, mostAngle);

    std::vector<TextRect> result;
    std::string log_str_raw;
    std::string log_str_proc;
    for (TextBlock& text_block : ocr_results.textBlocks) {
        if (text_block.boxPoint.size() != 4) {
            continue;
        }
        // the rect like ↓
        // 0 - 1
        // 3 - 2
        int x = text_block.boxPoint.at(0).x;
        int y = text_block.boxPoint.at(0).y;
        int width = text_block.boxPoint.at(1).x - x;
        int height = text_block.boxPoint.at(3).y - y;

        TextRect tr{ std::move(text_block.text), Rect(x, y, width, height) };
        log_str_raw += (std::string)tr + ", ";
        if (!pred || pred(tr)) {
            log_str_proc += tr.to_string() + ", ";
            result.emplace_back(std::move(tr));
        }
    }
    Log.trace("OcrPack::recognize | raw : ", log_str_raw);
    Log.trace("OcrPack::recognize | proc : ", log_str_proc);
    return result;
}

std::vector<asst::TextRect> asst::OcrPack::recognize(const cv::Mat & image, const asst::Rect & roi, const asst::TextRectProc & pred)
{
    auto rect_cor = [&roi, &pred](TextRect& tr) -> bool {
        tr.rect.x += roi.x;
        tr.rect.y += roi.y;
        return pred(tr);
    };
    Log.trace("OcrPack::recognize | roi : ", roi.to_string());
    return recognize(image(utils::make_rect<cv::Rect>(roi)), rect_cor);
}
