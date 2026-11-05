#ifdef __ANDROID__

#include "OcrPack.h"

#include "OcrPackNcnn.h"

#include <algorithm>
#include <filesystem>
#include <ranges>

#include "MaaUtils/NoWarningCV.hpp"

#include "Utils/Demangle.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

#include <cpu.h>

namespace asst
{

struct OcrPack::Impl
{
    std::unique_ptr<OcrPackNcnn> ncnn;
    std::filesystem::path det_model_path;
    std::filesystem::path rec_model_path;
    std::filesystem::path rec_label_path;
};

OcrPack::OcrPack() :
    m_impl(std::make_unique<Impl>())
{
}

OcrPack::~OcrPack()
{
    LogTraceFunction;
}

bool OcrPack::load(const std::filesystem::path& path)
{
    LogTraceFunction;
    Log.info("load", path.lexically_relative(UserDir.get()));

    using namespace asst::utils::path_literals;

    const auto det_dir = path / "det"_p;
    const auto det_model_file = det_dir / "det.ncnn.param"_p;
    const auto rec_dir = path / "rec"_p;
    const auto rec_model_file = rec_dir / "rec.ncnn.param"_p;
    const auto rec_label_file = rec_dir / "keys.txt"_p;

    if (std::filesystem::exists(det_model_file) && m_impl->det_model_path != det_model_file) {
        m_impl->det_model_path = det_model_file;
        m_impl->ncnn = nullptr;
    }
    else if (m_impl->det_model_path.empty()) {
        const auto fallback_det_file = path.parent_path() / "PaddleOCR" / "det" / "det.ncnn.param"_p;
        if (std::filesystem::exists(fallback_det_file)) {
            m_impl->det_model_path = fallback_det_file;
            m_impl->ncnn = nullptr;
        }
    }
    if (std::filesystem::exists(rec_model_file) && m_impl->rec_model_path != rec_model_file) {
        m_impl->rec_model_path = rec_model_file;
        m_impl->ncnn = nullptr;
    }
    if (std::filesystem::exists(rec_label_file) && m_impl->rec_label_path != rec_label_file) {
        m_impl->rec_label_path = rec_label_file;
        m_impl->ncnn = nullptr;
    }

    return !m_impl->det_model_path.empty() && !m_impl->rec_model_path.empty() && !m_impl->rec_label_path.empty();
}

OcrPack::ResultsVec OcrPack::recognize(const cv::Mat& image, bool without_det, const std::optional<Rect>& base_roi)
{
    if (!check_and_load()) {
        Log.error(__FUNCTION__, "check_and_load failed");
        return {};
    }

    auto start_time = std::chrono::steady_clock::now();

    auto raw_results = m_impl->ncnn->recognize(image, without_det);

    auto costs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
    std::string class_type = utils::demangle(typeid(*this).name());
    if (!base_roi) {
        Log.trace(class_type, raw_results, without_det ? "by OCR Rec" : "by OCR Pipeline", ", cost", costs, "ms");
    }
    else {
        std::string output = "[";
        for (size_t i = 0; i < raw_results.size(); ++i) {
            if (i != 0) {
                output += ", ";
            }
            output += std::format(
                "{{ text: {}, rect: [ {} ({}), {} ({}), {}, {} ], score: {:.6f} }}",
                raw_results[i].text,
                raw_results[i].rect.x,
                raw_results[i].rect.x + base_roi->x,
                raw_results[i].rect.y,
                raw_results[i].rect.y + base_roi->y,
                raw_results[i].rect.width,
                raw_results[i].rect.height,
                raw_results[i].score);
        }
        output += "]";
        Log.trace(class_type, output, without_det ? "by OCR Rec" : "by OCR Pipeline", ", cost", costs, "ms");
    }
    return raw_results;
}

bool OcrPack::check_and_load()
{
    if (m_impl->ncnn && m_impl->ncnn->initialized()) {
        return true;
    }

    LogTraceFunction;

    m_impl->ncnn = std::make_unique<OcrPackNcnn>();
    m_impl->ncnn->set_cpu_threads(OcrPackNcnn::bind_performance_cores());

    const auto det_bin = std::filesystem::path(m_impl->det_model_path).replace_extension("bin");
    const auto rec_bin = std::filesystem::path(m_impl->rec_model_path).replace_extension("bin");
    bool ok =
        m_impl->ncnn->load(m_impl->det_model_path, det_bin, m_impl->rec_model_path, rec_bin, m_impl->rec_label_path);

    Log.info("ncnn ocr inited", ok);
    return ok;
}

} // namespace asst

// ================================================================================
//  OcrPackNcnn implementation
// ================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <fstream>
#include <numeric>
#include <utility>

#include <ncnn/net.h>

#include <polyclipping/clipper.hpp>

// 以下检测后处理、裁切与 rec 预处理逐行对齐 fastdeploy（MaaXYZ/FastDeploy）的 ppocr 实现，
// 包括其整数取整与排序细节，使 Android 与桌面端在同一输入上得到一致的框与文本
namespace asst::ocr_ncnn
{
constexpr int kDetLimitSideLen = 960;
constexpr float kDetMean[3] = { 0.485f * 255.f, 0.456f * 255.f, 0.406f * 255.f };
constexpr float kDetNorm[3] = { 1.f / (0.229f * 255.f), 1.f / (0.224f * 255.f), 1.f / (0.225f * 255.f) };

constexpr double kDetThresh = 0.2; // fastdeploy SetDetDBThresh 的参数为 double
constexpr float kDetBoxThresh = 0.45f;
constexpr float kDetUnclipRatio = 1.4f;
constexpr int kDetMinSize = 3;
constexpr int kDetMaxCandidates = 3000;

constexpr int kRecImgH = 48;
constexpr int kRecImgW = 320;
constexpr size_t kRecBatchSize = 6; // fastdeploy PPOCRv2::rec_batch_size_ 默认值，影响同批的补边宽度
constexpr float kRecMean[3] = { 127.5f, 127.5f, 127.5f };
constexpr float kRecNorm[3] = { 1.f / 127.5f, 1.f / 127.5f, 1.f / 127.5f };

constexpr int kMaxCpuThreads = 4;

cv::Mat ensure_continuous_bgr(const cv::Mat& img)
{
    cv::Mat m = img;
    if (m.channels() == 4) {
        cv::cvtColor(m, m, cv::COLOR_BGRA2BGR);
    }
    else if (m.channels() == 1) {
        cv::cvtColor(m, m, cv::COLOR_GRAY2BGR);
    }
    if (!m.isContinuous()) {
        m = m.clone();
    }
    return m;
}

// fastdeploy 在少量元素上用 std::sort，MSVC 对小区间走插入排序（等价稳定），这里用 stable_sort 对齐并列项顺序
// fastdeploy PostProcessor::GetMiniBoxes
std::array<cv::Point2f, 4> get_mini_boxes(const cv::RotatedRect& box, float& ssid)
{
    ssid = std::max(box.size.width, box.size.height);

    cv::Mat points;
    cv::boxPoints(box, points);
    std::array<cv::Point2f, 4> a;
    for (int i = 0; i < 4; ++i) {
        a[i] = { points.at<float>(i, 0), points.at<float>(i, 1) };
    }
    std::ranges::stable_sort(a, [](const cv::Point2f& l, const cv::Point2f& r) { return l.x < r.x; });

    cv::Point2f idx1, idx2, idx3, idx4;
    if (a[3].y <= a[2].y) {
        idx2 = a[3];
        idx3 = a[2];
    }
    else {
        idx2 = a[2];
        idx3 = a[3];
    }
    if (a[1].y <= a[0].y) {
        idx1 = a[1];
        idx4 = a[0];
    }
    else {
        idx1 = a[0];
        idx4 = a[1];
    }
    return { idx1, idx2, idx3, idx4 };
}

// fastdeploy PostProcessor::PolygonScoreAcc
float polygon_score_acc(const std::vector<cv::Point>& contour, const cv::Mat& pred)
{
    const int width = pred.cols;
    const int height = pred.rows;
    int xmin = width;
    int xmax = 0;
    int ymin = height;
    int ymax = 0;
    for (const auto& p : contour) {
        xmin = std::min(xmin, p.x);
        xmax = std::max(xmax, p.x);
        ymin = std::min(ymin, p.y);
        ymax = std::max(ymax, p.y);
    }
    xmin = std::clamp(xmin, 0, width - 1);
    xmax = std::clamp(xmax, 0, width - 1);
    ymin = std::clamp(ymin, 0, height - 1);
    ymax = std::clamp(ymax, 0, height - 1);

    cv::Mat mask = cv::Mat::zeros(ymax - ymin + 1, xmax - xmin + 1, CV_8UC1);
    std::vector<cv::Point> shifted;
    shifted.reserve(contour.size());
    for (const auto& p : contour) {
        shifted.emplace_back(p.x - xmin, p.y - ymin);
    }
    cv::fillPoly(mask, std::vector<std::vector<cv::Point>> { shifted }, cv::Scalar(1));
    const cv::Mat region = pred(cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1));
    return static_cast<float>(cv::mean(region, mask)[0]);
}

// fastdeploy PostProcessor::UnClip（含 GetContourArea）
cv::RotatedRect unclip(const std::array<cv::Point2f, 4>& box, float unclip_ratio)
{
    float area = 0.f;
    float dist = 0.f;
    for (int i = 0; i < 4; ++i) {
        const auto& p = box[i];
        const auto& q = box[(i + 1) % 4];
        area += p.x * q.y - p.y * q.x;
        dist += sqrtf((p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y));
    }
    area = std::fabs(static_cast<float>(area / 2.0));
    const float distance = area * unclip_ratio / dist;

    ClipperLib::Path path;
    for (const auto& p : box) {
        path << ClipperLib::IntPoint(static_cast<int>(p.x), static_cast<int>(p.y));
    }
    ClipperLib::ClipperOffset offset;
    offset.AddPath(path, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
    ClipperLib::Paths soln;
    offset.Execute(soln, distance);

    std::vector<cv::Point2f> points;
    for (const auto& sub : soln) {
        // fastdeploy 内层循环上界取的是最后一个解的点数，单解时等价
        for (size_t i = 0; i < soln.back().size() && i < sub.size(); ++i) {
            points.emplace_back(static_cast<float>(sub[i].X), static_cast<float>(sub[i].Y));
        }
    }
    if (points.empty()) {
        return cv::RotatedRect(cv::Point2f(0, 0), cv::Size2f(1, 1), 0);
    }
    return cv::minAreaRect(points);
}

// fastdeploy PostProcessor::OrderPointsClockwise
std::array<cv::Point, 4> order_points_clockwise(std::array<cv::Point, 4> box)
{
    std::ranges::stable_sort(box, [](const cv::Point& l, const cv::Point& r) { return l.x < r.x; });
    std::array<cv::Point, 2> left = { box[0], box[1] };
    std::array<cv::Point, 2> right = { box[2], box[3] };
    if (left[0].y > left[1].y) {
        std::swap(left[0], left[1]);
    }
    if (right[0].y > right[1].y) {
        std::swap(right[0], right[1]);
    }
    return { left[0], right[0], right[1], left[1] };
}

// fastdeploy SortBoxes：先按左上角 y、x 排序，再做一轮同行（y 差 < 10）按 x 的调整。
// 其交换下标用的是 i 而非 j，属上游原样行为，这里照搬以保证输出顺序一致
template <typename Box>
void sort_boxes(std::vector<Box>& boxes)
{
    std::ranges::stable_sort(boxes, [](const Box& a, const Box& b) {
        if (a.pts[0].y != b.pts[0].y) {
            return a.pts[0].y < b.pts[0].y;
        }
        return a.pts[0].x < b.pts[0].x;
    });
    if (boxes.empty()) {
        return;
    }
    for (int i = 0; i < static_cast<int>(boxes.size()) - 1; ++i) {
        for (int j = i; j >= 0; --j) {
            if (std::abs(boxes[j + 1].pts[0].y - boxes[j].pts[0].y) < 10 && boxes[j + 1].pts[0].x < boxes[j].pts[0].x) {
                std::swap(boxes[i], boxes[i + 1]);
            }
        }
    }
}
} // namespace asst::ocr_ncnn

using namespace asst::ocr_ncnn;

asst::OcrPackNcnn::OcrPackNcnn() = default;

asst::OcrPackNcnn::~OcrPackNcnn() = default;

// ncnn 的 powersave 2 以各核最高频率的中点划分大小核，中核低于中点的 SoC（如骁龙 870）只会绑到一个超大核，
// 这里改为只排除最低频的一簇
int asst::OcrPackNcnn::bind_performance_cores()
{
    const int count = ncnn::get_cpu_count();
    std::vector<long> max_freq(count, 0);
    long lowest = 0;
    bool readable = true;
    for (int i = 0; i < count; ++i) {
        std::ifstream ifs(utils::path(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/cpuinfo_max_freq", i)));
        if (!(ifs >> max_freq[i]) || max_freq[i] <= 0) {
            readable = false;
            break;
        }
        lowest = lowest == 0 ? max_freq[i] : std::min(lowest, max_freq[i]);
    }
    ncnn::CpuSet mask;
    for (int i = 0; readable && i < count; ++i) {
        if (max_freq[i] > lowest) {
            mask.enable(i);
        }
    }

    if (mask.num_enabled() == 0) {
        ncnn::set_cpu_powersave(0);
        const int threads = count <= 2 ? 1 : count <= 4 ? 2 : count <= 12 ? 3 : 4;
        LogInfo << __FUNCTION__ << "no frequency tiers, unbound, threads" << threads;
        return threads;
    }

    ncnn::set_cpu_thread_affinity(mask);
    const int threads = std::min(mask.num_enabled(), kMaxCpuThreads);
    LogInfo << __FUNCTION__ << "performance cores" << mask.num_enabled() << "threads" << threads;
    return threads;
}

bool asst::OcrPackNcnn::load(
    const std::filesystem::path& det_param,
    const std::filesystem::path& det_bin,
    const std::filesystem::path& rec_param,
    const std::filesystem::path& rec_bin,
    const std::filesystem::path& keys_path)
{
    LogTraceFunction;

    auto make_net =
        [this](const std::filesystem::path& param, const std::filesystem::path& bin) -> std::unique_ptr<ncnn::Net> {
        auto net = std::make_unique<ncnn::Net>();
        net->opt.use_vulkan_compute = false;
        net->opt.num_threads = m_cpu_threads;
        net->opt.use_fp16_packed = false;
        net->opt.use_fp16_storage = false;
        net->opt.use_fp16_arithmetic = false;
        if (net->load_param(platform::path_to_utf8_string(param).c_str()) != 0) {
            Log.error("OcrPackNcnn load_param failed:", param);
            return nullptr;
        }
        if (net->load_model(platform::path_to_utf8_string(bin).c_str()) != 0) {
            Log.error("OcrPackNcnn load_model failed:", bin);
            return nullptr;
        }
        return net;
    };

    m_det = make_net(det_param, det_bin);
    m_rec = make_net(rec_param, rec_bin);
    if (!m_det || !m_rec) {
        m_loaded = false;
        return false;
    }

    std::vector<std::string> keys;
    {
        std::ifstream ifs(keys_path, std::ios::binary);
        if (!ifs.is_open()) {
            Log.error("OcrPackNcnn open keys failed:", keys_path);
            m_loaded = false;
            return false;
        }
        std::string line;
        bool first = true;
        while (std::getline(ifs, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (first) {
                first = false;
                if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
                    static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF) {
                    line.erase(0, 3);
                }
            }
            keys.push_back(line);
        }
    }

    auto probe_rec = [this](int width, ncnn::Mat& out) {
        ncnn::Mat dummy(width, kRecImgH, 3);
        dummy.fill(0.f);
        ncnn::Extractor ex = m_rec->create_extractor();
        ex.input("in0", dummy);
        return ex.extract("out0", out) == 0;
    };

    int num_classes = 0;
    int steps = 0;
    {
        ncnn::Mat out;
        if (!probe_rec(kRecImgW, out)) {
            Log.error("OcrPackNcnn rec probe extract failed");
            m_loaded = false;
            return false;
        }
        num_classes = out.w;
        steps = out.h;
        const float* row0 = out.row(0);
        float s = 0.f;
        float mn = row0[0];
        for (int c = 0; c < num_classes; ++c) {
            s += row0[c];
            mn = std::min(mn, row0[c]);
        }
        m_rec_output_is_prob = (mn >= -1e-6f) && (std::abs(s - 1.f) < 1e-2f);
    }
    {
        // pnnx 动态转换的 rec 在 2 倍宽度下输出 2 倍时间步；静态转换的仍输出 320 宽对应的步数
        ncnn::Mat out;
        if (!probe_rec(kRecImgW * 2, out)) {
            m_rec_dynamic_width = false;
            LogWarn << __FUNCTION__ << "rec probe extract failed at width" << kRecImgW * 2
                    << ", fall back to static width";
        }
        else {
            m_rec_dynamic_width = out.h == steps * 2 && out.w == num_classes;
        }
    }

    m_charset.clear();
    m_charset.reserve(num_classes);
    m_charset.emplace_back("<blank>");
    for (auto& k : keys) {
        m_charset.emplace_back(k);
    }
    if (num_classes == static_cast<int>(m_charset.size()) + 1) {
        m_charset.emplace_back(" ");
    }
    if (num_classes != static_cast<int>(m_charset.size())) {
        Log.warn("OcrPackNcnn charset size", m_charset.size(), "!= num_classes", num_classes, "; keys", keys.size());
        if (num_classes > static_cast<int>(m_charset.size())) {
            m_charset.resize(num_classes, "<unk>");
        }
        else {
            m_charset.resize(num_classes);
        }
    }

    m_loaded = true;
    LogInfo << __FUNCTION__ << "num_classes" << num_classes << "charset" << m_charset.size() << "threads"
            << m_cpu_threads << "rec_dynamic_width" << m_rec_dynamic_width;
    return true;
}

std::vector<asst::OcrPackNcnn::DetBox> asst::OcrPackNcnn::detect(const cv::Mat& image) const
{
    const int src_h = image.rows;
    const int src_w = image.cols;

    // fastdeploy DBDetectorPreprocessor::OcrDetectorGetInfo
    float ratio = 1.f;
    if (std::max(src_w, src_h) > kDetLimitSideLen) {
        ratio = src_h > src_w ? static_cast<float>(kDetLimitSideLen) / static_cast<float>(src_h)
                              : static_cast<float>(kDetLimitSideLen) / static_cast<float>(src_w);
    }
    auto align32 = [](int v) {
        return std::max(static_cast<int>(std::round(static_cast<float>(v) / 32) * 32), 32);
    };
    const int resize_h = align32(static_cast<int>(static_cast<float>(src_h) * ratio));
    const int resize_w = align32(static_cast<int>(static_cast<float>(src_w) * ratio));

    // 缩放用 cv::resize（同 fastdeploy Resize 算子），ncnn 自带的 from_pixels_resize 双线性取整与 OpenCV 不同。
    // 不转 RGB，kDetMean/kDetNorm 按 BGR 通道施加，同 fastdeploy NormalizeAndPermute（swap_rb 默认 false）
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(resize_w, resize_h), 0, 0, cv::INTER_LINEAR);
    ncnn::Mat in = ncnn::Mat::from_pixels(resized.data, ncnn::Mat::PIXEL_BGR, resize_w, resize_h);
    in.substract_mean_normalize(kDetMean, kDetNorm);

    ncnn::Mat out;
    {
        ncnn::Extractor ex = m_det->create_extractor();
        ex.input("in0", in);
        if (ex.extract("out0", out) != 0) {
            Log.error("OcrPackNcnn det extract failed");
            return {};
        }
    }
    ncnn::Mat plane = out.channel(0);
    const cv::Mat pred(out.h, out.w, CV_32FC1, plane.data);

    // fastdeploy DBDetectorPostprocessor::SingleBatchPostprocessor：概率图先截断成 uint8 再二值化
    cv::Mat cbuf(pred.rows, pred.cols, CV_8UC1);
    {
        const auto* src = pred.ptr<float>();
        auto* dst = cbuf.ptr<unsigned char>();
        const size_t n = pred.total();
        for (size_t i = 0; i < n; ++i) {
            dst[i] = static_cast<unsigned char>(src[i] * 255);
        }
    }
    cv::Mat bitmap;
    cv::threshold(cbuf, bitmap, kDetThresh * 255, 255, cv::THRESH_BINARY);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bitmap, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    // fastdeploy PostProcessor::BoxesFromBitmap + FilterTagDetRes
    const float ratio_w = static_cast<float>(resize_w) / static_cast<float>(src_w);
    const float ratio_h = static_cast<float>(resize_h) / static_cast<float>(src_h);
    std::vector<DetBox> results;
    const int num_contours = std::min(static_cast<int>(contours.size()), kDetMaxCandidates);
    for (int ci = 0; ci < num_contours; ++ci) {
        const auto& contour = contours[ci];
        if (contour.size() <= 2) {
            continue;
        }
        float ssid = 0.f;
        const auto mini_box = get_mini_boxes(cv::minAreaRect(contour), ssid);
        if (ssid < kDetMinSize) {
            continue;
        }
        const float score = polygon_score_acc(contour, pred);
        if (score < kDetBoxThresh) {
            continue;
        }
        const cv::RotatedRect clip_rect = unclip(mini_box, kDetUnclipRatio);
        if (clip_rect.size.height < 1.001 && clip_rect.size.width < 1.001) {
            continue;
        }
        const auto clip_box = get_mini_boxes(clip_rect, ssid);
        if (ssid < kDetMinSize + 2) {
            continue;
        }

        std::array<cv::Point, 4> int_box;
        for (int k = 0; k < 4; ++k) {
            const auto dw = static_cast<float>(pred.cols);
            const auto dh = static_cast<float>(pred.rows);
            int_box[k].x = static_cast<int>(std::clamp(roundf(clip_box[k].x / dw * dw), 0.f, dw));
            int_box[k].y = static_cast<int>(std::clamp(roundf(clip_box[k].y / dh * dh), 0.f, dh));
        }
        int_box = order_points_clockwise(int_box);
        DetBox db;
        db.score = score;
        for (int k = 0; k < 4; ++k) {
            // 同 fastdeploy 的 int /= float：缩回原图坐标时截断取整
            const int x = static_cast<int>(static_cast<float>(int_box[k].x) / ratio_w);
            const int y = static_cast<int>(static_cast<float>(int_box[k].y) / ratio_h);
            db.pts[k] = { std::clamp(x, 0, src_w - 1), std::clamp(y, 0, src_h - 1) };
        }
        const auto rect_w = static_cast<int>(
            std::sqrt(std::pow(db.pts[0].x - db.pts[1].x, 2) + std::pow(db.pts[0].y - db.pts[1].y, 2)));
        const auto rect_h = static_cast<int>(
            std::sqrt(std::pow(db.pts[0].x - db.pts[3].x, 2) + std::pow(db.pts[0].y - db.pts[3].y, 2)));
        if (rect_w <= 4 || rect_h <= 4) {
            continue;
        }
        results.push_back(db);
    }

    sort_boxes(results);
    return results;
}

cv::Mat asst::OcrPackNcnn::crop_rotated(const cv::Mat& image, const DetBox& box)
{
    // fastdeploy GetRotateCropImage：先取外接矩形，再按 0-1、0-3 边长透视摆正
    const auto [left, right] = std::minmax({ box.pts[0].x, box.pts[1].x, box.pts[2].x, box.pts[3].x });
    const auto [top, bottom] = std::minmax({ box.pts[0].y, box.pts[1].y, box.pts[2].y, box.pts[3].y });
    const cv::Mat img_crop = image(cv::Rect(left, top, right - left, bottom - top)).clone();

    cv::Point2f src[4];
    for (int i = 0; i < 4; ++i) {
        src[i] = cv::Point2f(static_cast<float>(box.pts[i].x - left), static_cast<float>(box.pts[i].y - top));
    }
    const int w = static_cast<int>(std::sqrt(std::pow(src[0].x - src[1].x, 2) + std::pow(src[0].y - src[1].y, 2)));
    const int h = static_cast<int>(std::sqrt(std::pow(src[0].x - src[3].x, 2) + std::pow(src[0].y - src[3].y, 2)));
    const cv::Point2f dst[4] = { { 0.f, 0.f },
                                 { static_cast<float>(w), 0.f },
                                 { static_cast<float>(w), static_cast<float>(h) },
                                 { 0.f, static_cast<float>(h) } };
    const cv::Mat m = cv::getPerspectiveTransform(src, dst);
    cv::Mat crop;
    // fastdeploy 把 BORDER_REPLICATE 传在了 flags 位，实际生效为 INTER_LINEAR + 默认
    // BORDER_CONSTANT，这里按实际行为对齐
    cv::warpPerspective(img_crop, crop, m, cv::Size(w, h), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
    if (static_cast<float>(crop.rows) >= static_cast<float>(crop.cols) * 1.5f) {
        cv::rotate(crop, crop, cv::ROTATE_90_COUNTERCLOCKWISE);
    }
    return crop;
}

std::pair<std::string, float> asst::OcrPackNcnn::recognize_line(const cv::Mat& line_in, float max_wh_ratio) const
{
    cv::Mat line = ensure_continuous_bgr(line_in);
    if (line.empty()) {
        return { std::string(), 0.f };
    }

    // fastdeploy RecognizerPreprocessor::OcrRecognizerResizeImage：画布宽度按同批最大宽高比加宽，保持行内比例。
    // 静态宽度的 rec 模型（pnnx 无法动态转换时）只能固定 320，长行横向压缩
    const int img_w = m_rec_dynamic_width ? static_cast<int>(kRecImgH * max_wh_ratio) : kRecImgW;
    const float ratio = static_cast<float>(line.cols) / static_cast<float>(line.rows);
    const int resize_w = ceilf(kRecImgH * ratio) > img_w ? img_w : static_cast<int>(ceilf(kRecImgH * ratio));
    cv::Mat resized;
    cv::resize(line, resized, cv::Size(resize_w, kRecImgH), 0, 0, cv::INTER_LINEAR);
    cv::Mat canvas;
    cv::copyMakeBorder(resized, canvas, 0, 0, 0, img_w - resize_w, cv::BORDER_CONSTANT, cv::Scalar(127, 127, 127));

    // 同 det，用 PIXEL_BGR 不转 RGB，对齐 fastdeploy rec_preprocessor.cc
    ncnn::Mat in = ncnn::Mat::from_pixels(canvas.data, ncnn::Mat::PIXEL_BGR, img_w, kRecImgH);
    in.substract_mean_normalize(kRecMean, kRecNorm);

    ncnn::Mat out;
    {
        ncnn::Extractor ex = m_rec->create_extractor();
        ex.input("in0", in);
        if (ex.extract("out0", out) != 0) {
            Log.error("OcrPackNcnn rec extract failed");
            return { std::string(), 0.f };
        }
    }
    const int T = out.h;
    const int nc = out.w;

    // fastdeploy RecognizerPostprocessor::SingleBatchPostprocessor
    std::string text;
    float score = 0.f;
    int count = 0;
    int prev = -1;
    for (int t = 0; t < T; ++t) {
        const float* row = out.row(t);
        int best = 0;
        float best_v = row[0];
        for (int c = 1; c < nc; ++c) {
            if (row[c] > best_v) {
                best_v = row[c];
                best = c;
            }
        }
        float conf = best_v;
        if (!m_rec_output_is_prob) {
            float denom = 0.f;
            for (int c = 0; c < nc; ++c) {
                denom += std::exp(row[c] - best_v);
            }
            conf = denom > 0.f ? 1.f / denom : 0.f;
        }
        if (best != 0 && best != prev && best < static_cast<int>(m_charset.size())) {
            text += m_charset[best];
            score += conf;
            ++count;
        }
        prev = best;
    }
    score /= (count + 1e-6f);
    if (count == 0 || std::isnan(score)) {
        score = 0.f;
    }
    return { std::move(text), score };
}

asst::OcrPackNcnn::ResultsVec asst::OcrPackNcnn::recognize(const cv::Mat& image_in, bool without_det)
{
    if (!m_loaded) {
        return {};
    }
    cv::Mat image = ensure_continuous_bgr(image_in);

    const float base_ratio = static_cast<float>(kRecImgW * 1.0 / kRecImgH);
    auto line_ratio = [](const cv::Mat& m) {
        return static_cast<float>(m.cols * 1.0 / m.rows);
    };

    ResultsVec results;
    if (without_det) {
        auto [text, score] = recognize_line(image, std::max(base_ratio, line_ratio(image)));
        results.emplace_back(Rect(0, 0, image.cols, image.rows), score, std::move(text));
        return results;
    }

    const std::vector<DetBox> boxes = detect(image);
    if (boxes.empty()) {
        // 对齐 fastdeploy PPOCRv2::BatchPredict：det 无框时整图当一行送 rec
        auto [text, score] = recognize_line(image, std::max(base_ratio, line_ratio(image)));
        results.emplace_back(Rect(0, 0, image.cols, image.rows), score, std::move(text));
        return results;
    }

    std::vector<cv::Mat> crops;
    crops.reserve(boxes.size());
    for (const auto& box : boxes) {
        crops.emplace_back(crop_rotated(image, box));
    }

    // 对齐 fastdeploy PPOCRv2::BatchPredict：按宽高比升序分批，同批补边到批内最大宽高比
    std::vector<float> ratios;
    ratios.reserve(crops.size());
    for (const auto& c : crops) {
        ratios.emplace_back(static_cast<float>(c.cols) / static_cast<float>(c.rows));
    }
    std::vector<size_t> order(crops.size());
    std::iota(order.begin(), order.end(), 0);
    std::ranges::stable_sort(order, [&](size_t a, size_t b) { return ratios[a] < ratios[b]; });

    std::vector<std::pair<std::string, float>> rec(crops.size());
    for (size_t start = 0; start < order.size(); start += kRecBatchSize) {
        const size_t end = std::min(start + kRecBatchSize, order.size());
        float max_wh_ratio = base_ratio;
        for (size_t i = start; i < end; ++i) {
            max_wh_ratio = std::max(max_wh_ratio, line_ratio(crops[order[i]]));
        }
        for (size_t i = start; i < end; ++i) {
            rec[order[i]] = recognize_line(crops[order[i]], max_wh_ratio);
        }
    }

    for (size_t i = 0; i < boxes.size(); ++i) {
        const auto& pts = boxes[i].pts;
        const auto [left, right] = std::minmax({ pts[0].x, pts[1].x, pts[2].x, pts[3].x });
        const auto [top, bottom] = std::minmax({ pts[0].y, pts[1].y, pts[2].y, pts[3].y });
        results.emplace_back(Rect(left, top, right - left, bottom - top), rec[i].second, std::move(rec[i].first));
    }
    return results;
}

#endif // __ANDROID__
