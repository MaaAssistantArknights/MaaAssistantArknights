#include "RoguelikeBlackflowMapAnalyzer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <string_view>

#include "Config/OnnxSessions.h"
#include "Config/Roguelike/RoguelikeMapConfig.h"
#include "Config/TaskData.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Task/Roguelike/RoguelikeConfig.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"

using namespace asst;

// ============================================================================
// 说明：本文件逐函数移植 C:\tmp\map_extractor_solution\extract_map.py。
// MAA 归一化截图为 1280x720，故 scale = w/BASE_WIDTH = 1.0，所有 *scale 取整为恒等。
// 对拍夹具：C:\tmp\ds_fixtures\{1,2,4}_fixture.json。
// ============================================================================

namespace {

// —— numpy 等价的小工具 ——

// 对一组样本值计算 (mean, std)。std 为 numpy 默认的总体标准差（ddof=0）。
inline std::pair<double, double> mean_std(const std::vector<double>& v)
{
    if (v.empty()) {
        return { 0.0, 0.0 };
    }
    const double mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    double acc = 0.0;
    for (double x : v) {
        acc += (x - mean) * (x - mean);
    }
    return { mean, std::sqrt(acc / v.size()) };
}

// numpy.percentile 的线性插值实现（默认 method='linear'）。
inline double percentile(std::vector<double> v, double q)
{
    if (v.empty()) {
        return 0.0;
    }
    std::ranges::sort(v);
    if (v.size() == 1) {
        return v.front();
    }
    const double rank = q / 100.0 * static_cast<double>(v.size() - 1);
    const size_t lo = static_cast<size_t>(std::floor(rank));
    const size_t hi = static_cast<size_t>(std::ceil(rank));
    const double frac = rank - static_cast<double>(lo);
    return v[lo] + (v[hi] - v[lo]) * frac;
}

inline double vmax(const std::vector<double>& v)
{
    if (v.empty()) {
        return 0.0;
    }
    return *std::ranges::max_element(v);
}

inline double vmean(const std::vector<double>& v)
{
    if (v.empty()) {
        return 0.0;
    }
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

inline int iround(double x)
{
    // numpy int(round(x))：round-half-to-even 与 C 的 std::lround(round-half-away) 在 .5 边界不同，
    // 但坐标均为整数像素中心，几乎不触及 .5 边界；此处用四舍五入。
    return static_cast<int>(std::lround(x));
}

} // namespace

// ============================================================================
// 特征上下文
// ============================================================================

RoguelikeBlackflowMapAnalyzer::NodeCtx
    RoguelikeBlackflowMapAnalyzer::make_node_ctx(const cv::Mat& image)
{
    NodeCtx ctx;
    cv::cvtColor(image, ctx.gray, cv::COLOR_BGR2GRAY);
    cv::Mat hsv;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);
    std::array<cv::Mat, 3> ch;
    cv::split(hsv, ch.data());
    ctx.sat = ch[1];
    ctx.val = ch[2];
    cv::Canny(ctx.gray, ctx.canny, 50, 120);
    cv::Mat gx, gy;
    cv::Sobel(ctx.gray, gx, CV_32F, 1, 0, 3);
    cv::Sobel(ctx.gray, gy, CV_32F, 0, 1, 3);
    cv::magnitude(gx, gy, ctx.grad);
    return ctx;
}

RoguelikeBlackflowMapAnalyzer::EdgeCtx
    RoguelikeBlackflowMapAnalyzer::make_edge_ctx(const cv::Mat& image)
{
    EdgeCtx ctx;
    cv::cvtColor(image, ctx.gray, cv::COLOR_BGR2GRAY);
    cv::Mat hsv;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);
    std::array<cv::Mat, 3> ch;
    cv::split(hsv, ch.data());
    ctx.sat = ch[1];
    ctx.val = ch[2];
    cv::Canny(ctx.gray, ctx.canny, 40, 110);
    cv::Mat gx, gy;
    cv::Sobel(ctx.gray, gx, CV_32F, 1, 0, 3);
    cv::Sobel(ctx.gray, gy, CV_32F, 0, 1, 3);
    ctx.gx = cv::abs(gx);
    ctx.gy = cv::abs(gy);
    cv::magnitude(ctx.gx, ctx.gy, ctx.grad);
    return ctx;
}

RoguelikeBlackflowMapAnalyzer::PlayerCtx
    RoguelikeBlackflowMapAnalyzer::make_player_ctx(const cv::Mat& image)
{
    PlayerCtx ctx;
    cv::cvtColor(image, ctx.gray, cv::COLOR_BGR2GRAY);
    cv::Mat hsv;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);
    std::array<cv::Mat, 3> ch;
    cv::split(hsv, ch.data());
    ctx.hue = ch[0];
    ctx.sat = ch[1];
    ctx.val = ch[2];
    cv::Canny(ctx.gray, ctx.canny, 50, 120);
    return ctx;
}

// ============================================================================
// node_features —— 对应 extract_map.py::_node_features（181 维，scale=1.0）
// ============================================================================
std::vector<float> RoguelikeBlackflowMapAnalyzer::node_features(
    const NodeCtx& ctx,
    float x,
    float y,
    const std::vector<cv::Vec3f>& circles)
{
    const int h = ctx.gray.rows;
    const int w = ctx.gray.cols;
    constexpr double scale = 1.0;
    const int radius = 45;
    const int xi = iround(x);
    const int yi = iround(y);
    const int xa = std::max(0, xi - radius);
    const int xb = std::min(w, xi + radius + 1);
    const int ya = std::max(0, yi - radius);
    const int yb = std::min(h, yi + radius + 1);
    const int bw = std::max(0, xb - xa);
    const int bh = std::max(0, yb - ya);
    const size_t n = static_cast<size_t>(bw) * bh;

    // 5 个通道（与 Python arrays 顺序一致）：gray, sat, val, canny, grad
    std::array<std::vector<double>, 5> chan;
    for (auto& c : chan) {
        c.resize(n);
    }
    std::vector<double> rr(n);
    for (int i = 0; i < bh; ++i) {
        const int py = ya + i;
        const uchar* p_gray = ctx.gray.ptr<uchar>(py);
        const uchar* p_sat = ctx.sat.ptr<uchar>(py);
        const uchar* p_val = ctx.val.ptr<uchar>(py);
        const uchar* p_canny = ctx.canny.ptr<uchar>(py);
        const float* p_grad = ctx.grad.ptr<float>(py);
        for (int j = 0; j < bw; ++j) {
            const int px = xa + j;
            const size_t idx = static_cast<size_t>(i) * bw + j;
            chan[0][idx] = p_gray[px];
            chan[1][idx] = p_sat[px];
            chan[2][idx] = p_val[px];
            chan[3][idx] = p_canny[px];
            chan[4][idx] = p_grad[px];
            const double ddx = px - x;
            const double ddy = py - y;
            rr[idx] = std::sqrt(ddx * ddx + ddy * ddy) / scale;
        }
    }

    std::vector<float> f;
    f.reserve(181);

    // 7 半径 × 5 通道 × (mean, std, p90)
    static constexpr std::array<double, 7> radii = { 5, 8, 12, 18, 25, 32, 40 };
    for (double r : radii) {
        for (auto& c : chan) {
            std::vector<double> v;
            v.reserve(n);
            for (size_t k = 0; k < n; ++k) {
                if (rr[k] <= r) {
                    v.push_back(c[k]);
                }
            }
            auto [mean, sd] = mean_std(v);
            f.push_back(static_cast<float>(mean));
            f.push_back(static_cast<float>(sd));
            f.push_back(static_cast<float>(percentile(v, 90)));
        }
    }

    // 5 环带 × 5 通道 × (mean, std)
    static constexpr std::array<std::pair<double, double>, 5> rings = {
        { { 0, 8 }, { 8, 16 }, { 16, 24 }, { 24, 34 }, { 34, 44 } }
    };
    for (auto [r0, r1] : rings) {
        for (auto& c : chan) {
            std::vector<double> v;
            for (size_t k = 0; k < n; ++k) {
                if (rr[k] >= r0 && rr[k] < r1) {
                    v.push_back(c[k]);
                }
            }
            auto [mean, sd] = mean_std(v);
            f.push_back(static_cast<float>(mean));
            f.push_back(static_cast<float>(sd));
        }
    }

    // 5 通道 × 4 方向矩形均值（rect_mean 在全图数组上取，scale=1.0）
    auto rect_mean = [&](const cv::Mat& arr, double dx0, double dy0, double dx1, double dy1) -> double {
        const int x0 = std::max(0, iround(x + dx0));
        const int x1 = std::min(w, iround(x + dx1) + 1);
        const int y0 = std::max(0, iround(y + dy0));
        const int y1 = std::min(h, iround(y + dy1) + 1);
        if (x1 <= x0 || y1 <= y0) {
            return 0.0;
        }
        double sum = 0.0;
        const bool is_f32 = arr.type() == CV_32F;
        for (int yy = y0; yy < y1; ++yy) {
            const uchar* pu = is_f32 ? nullptr : arr.ptr<uchar>(yy);
            const float* pf = is_f32 ? arr.ptr<float>(yy) : nullptr;
            for (int xx = x0; xx < x1; ++xx) {
                sum += is_f32 ? static_cast<double>(pf[xx]) : static_cast<double>(pu[xx]);
            }
        }
        return sum / (static_cast<double>(x1 - x0) * (y1 - y0));
    };
    const std::array<const cv::Mat*, 5> full = { &ctx.gray, &ctx.sat, &ctx.val, &ctx.canny, &ctx.grad };
    for (const cv::Mat* arr : full) {
        f.push_back(static_cast<float>(rect_mean(*arr, -45, -4, -12, 4)));
        f.push_back(static_cast<float>(rect_mean(*arr, 12, -4, 45, 4)));
        f.push_back(static_cast<float>(rect_mean(*arr, -4, -45, 4, -12)));
        f.push_back(static_cast<float>(rect_mean(*arr, -4, 12, 4, 45)));
    }

    // 3 距离环 × (计数, 最大半径)
    static constexpr std::array<std::pair<double, double>, 3> circ_bins = { { { 0, 12 }, { 12, 20 }, { 20, 40 } } };
    for (auto [lo, hi] : circ_bins) {
        double count = 0.0;
        double max_r = 0.0;
        for (const auto& c : circles) {
            const double d = std::hypot(c[0] - x, c[1] - y);
            if (d < 10.0 && c[2] >= lo && c[2] < hi) {
                count += 1.0;
                max_r = std::max(max_r, static_cast<double>(c[2]));
            }
        }
        f.push_back(static_cast<float>(count));
        f.push_back(static_cast<float>(max_r));
    }

    return f;
}

// ============================================================================
// node_type_features —— 节点类型分类模型的输入特征（2118 维）
// ============================================================================
std::vector<float> RoguelikeBlackflowMapAnalyzer::node_type_features(const cv::Mat& image, float x, float y)
{
    constexpr int crop_size = 92;
    constexpr int input_size = 64;
    constexpr int feature_count = 2118;

    const int xi = iround(x);
    const int yi = iround(y);
    const int x0 = std::clamp(xi - crop_size / 2, 0, std::max(0, image.cols - 1));
    const int y0 = std::clamp(yi - crop_size / 2, 0, std::max(0, image.rows - 1));
    const int x1 = std::min(image.cols, x0 + crop_size);
    const int y1 = std::min(image.rows, y0 + crop_size);

    std::vector<float> f;
    f.reserve(feature_count);
    if (x1 <= x0 || y1 <= y0) {
        f.resize(feature_count, 0.0f);
        return f;
    }

    cv::Mat resized;
    cv::resize(image(cv::Rect(x0, y0, x1 - x0, y1 - y0)), resized, cv::Size(input_size, input_size), 0, 0, cv::INTER_AREA);

    cv::Mat hsv;
    cv::cvtColor(resized, hsv, cv::COLOR_BGR2HSV);
    cv::Mat gray_u8;
    cv::cvtColor(resized, gray_u8, cv::COLOR_BGR2GRAY);
    cv::Mat gray;
    gray_u8.convertTo(gray, CV_32F);

    // Python: (gray - mean) / (std + 1e-5)
    cv::Scalar mean, stddev;
    cv::meanStdDev(gray, mean, stddev);
    cv::Mat normalized_gray = (gray - mean[0]) / (stddev[0] + 1e-5);

    cv::Mat gray_small;
    cv::resize(normalized_gray, gray_small, cv::Size(16, 16), 0, 0, cv::INTER_AREA);

    // Python: min-max 归一化后再计算 HOG。
    double min_value = 0.0;
    double max_value = 0.0;
    cv::minMaxLoc(gray, &min_value, &max_value);
    cv::Mat hog_gray = (gray - min_value) / (max_value - min_value + 1e-5) * 255.0;
    cv::Mat gx, gy, magnitude, angle;
    cv::Sobel(hog_gray, gx, CV_32F, 1, 0, 3);
    cv::Sobel(hog_gray, gy, CV_32F, 0, 1, 3);
    cv::cartToPolar(gx, gy, magnitude, angle, true);

    std::array<float, 8 * 8 * 9> cell_hist {};
    for (int row = 0; row < input_size; ++row) {
        for (int col = 0; col < input_size; ++col) {
            float degrees = std::fmod(angle.at<float>(row, col), 180.0f);
            if (degrees < 0.0f) {
                degrees += 180.0f;
            }
            const float position = degrees / 20.0f;
            const int lower = static_cast<int>(std::floor(position)) % 9;
            const int upper = (lower + 1) % 9;
            const float upper_weight = position - std::floor(position);
            const float lower_weight = 1.0f - upper_weight;
            const float value = magnitude.at<float>(row, col);
            const size_t base = (static_cast<size_t>(row / 8) * 8 + col / 8) * 9;
            cell_hist[base + lower] += value * lower_weight;
            cell_hist[base + upper] += value * upper_weight;
        }
    }

    // Python 的 blocks 顺序是 row-major，每个 block 取相邻 2x2 cells。
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 7; ++col) {
            std::array<float, 36> block {};
            size_t index = 0;
            for (int block_row = 0; block_row < 2; ++block_row) {
                for (int block_col = 0; block_col < 2; ++block_col) {
                    const size_t base = (static_cast<size_t>(row + block_row) * 8 + col + block_col) * 9;
                    for (int bin = 0; bin < 9; ++bin) {
                        block[index++] = cell_hist[base + bin];
                    }
                }
            }
            auto l2_normalize = [](std::array<float, 36>& values) {
                float sum = 0.0f;
                for (float value : values) {
                    sum += value * value;
                }
                const float denom = std::sqrt(sum + 1e-12f);
                for (float& value : values) {
                    value /= denom;
                }
            };
            l2_normalize(block);
            for (float& value : block) {
                value = std::min(value, 0.2f);
            }
            l2_normalize(block);
            f.insert(f.end(), block.begin(), block.end());
        }
    }

    // Python gray_small.ravel()。
    for (int row = 0; row < gray_small.rows; ++row) {
        const float* data = gray_small.ptr<float>(row);
        f.insert(f.end(), data, data + gray_small.cols);
    }

    // Python calcHist 的 H/S/V 直方图。三个直方图拼接后统一除以 hist.sum()。
    std::array<float, 18 + 16 + 16> hist {};
    for (int row = 0; row < input_size; ++row) {
        const uchar* data = hsv.ptr<uchar>(row);
        for (int col = 0; col < input_size; ++col) {
            const uchar h = data[col * 3];
            const uchar s = data[col * 3 + 1];
            const uchar v = data[col * 3 + 2];
            ++hist[std::min(17, static_cast<int>(h) * 18 / 180)];
            ++hist[18 + std::min(15, static_cast<int>(s) * 16 / 256)];
            ++hist[34 + std::min(15, static_cast<int>(v) * 16 / 256)];
        }
    }
    const float hist_sum = std::accumulate(hist.begin(), hist.end(), 0.0f) + 1e-6f;
    for (float value : hist) {
        f.push_back(value / hist_sum);
    }

    // Python np.array_split(channel, 4, axis=0/1)；64x64 时每格为 16x16。
    for (int channel = 0; channel < 3; ++channel) {
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                double sum = 0.0;
                for (int py = row * 16; py < (row + 1) * 16; ++py) {
                    const uchar* data = hsv.ptr<uchar>(py);
                    for (int px = col * 16; px < (col + 1) * 16; ++px) {
                        const int value = data[px * 3 + channel];
                        const double divisor = channel == 0 ? 180.0 : 255.0;
                        sum += value / divisor;
                    }
                }
                f.push_back(static_cast<float>(sum / 256.0));
            }
        }
    }

    if (f.size() != feature_count) {
        Log.error(__FUNCTION__, "| unexpected feature count", f.size(), "expected", feature_count);
        f.resize(feature_count, 0.0f);
    }
    return f;
}

// ============================================================================
// edge_features —— 对应 extract_map.py::_edge_features（378 维，scale=1.0）
// ============================================================================
std::vector<float> RoguelikeBlackflowMapAnalyzer::edge_features(
    const EdgeCtx& ctx,
    float x1,
    float y1,
    float x2,
    float y2)
{
    const int h = ctx.gray.rows;
    const int w = ctx.gray.cols;
    const bool horizontal = std::abs(x2 - x1) > std::abs(y2 - y1);

    // arrays 顺序：gray, sat, val, canny, gx, gy, grad
    const std::array<const cv::Mat*, 7> arrays = { &ctx.gray, &ctx.sat, &ctx.val, &ctx.canny,
                                                   &ctx.gx,   &ctx.gy,  &ctx.grad };

    std::vector<float> f;
    f.reserve(378);
    f.push_back(horizontal ? 1.0f : 0.0f);

    // 收集矩形 ROI 内的所有像素值（按行主序 ravel）
    auto collect = [&](const cv::Mat& arr, int y0, int y1r, int x0, int x1r) -> std::vector<double> {
        std::vector<double> v;
        if (y1r <= y0 || x1r <= x0) {
            return v;
        }
        v.reserve(static_cast<size_t>(y1r - y0) * (x1r - x0));
        const bool is_f32 = arr.type() == CV_32F;
        for (int yy = y0; yy < y1r; ++yy) {
            const uchar* pu = is_f32 ? nullptr : arr.ptr<uchar>(yy);
            const float* pf = is_f32 ? arr.ptr<float>(yy) : nullptr;
            for (int xx = x0; xx < x1r; ++xx) {
                v.push_back(is_f32 ? static_cast<double>(pf[xx]) : static_cast<double>(pu[xx]));
            }
        }
        return v;
    };

    // ——（1）7 偏移条带 × 7 通道 × (mean, std, p25, p75, p90, max) ——
    static constexpr std::array<int, 7> offs = { 0, -4, 4, -8, 8, -14, 14 };
    for (int off : offs) {
        int y0, y1r, x0, x1r;
        if (horizontal) {
            const int xa = iround(std::min(x1, x2) + 14);
            const int xb = iround(std::max(x1, x2) - 14);
            const int yc = iround(y1 + off);
            const int half = std::max(1, iround(3));
            y0 = std::max(0, yc - half);
            y1r = std::min(h, yc + half + 1);
            x0 = std::max(0, xa);
            x1r = std::min(w, xb + 1);
        }
        else {
            const int ya = iround(std::min(y1, y2) + 14);
            const int yb = iround(std::max(y1, y2) - 14);
            const int xc = iround(x1 + off);
            const int half = std::max(1, iround(3));
            y0 = std::max(0, ya);
            y1r = std::min(h, yb + 1);
            x0 = std::max(0, xc - half);
            x1r = std::min(w, xc + half + 1);
        }
        for (const cv::Mat* arr : arrays) {
            std::vector<double> v = collect(*arr, y0, y1r, x0, x1r);
            auto [mean, sd] = mean_std(v);
            f.push_back(static_cast<float>(mean));
            f.push_back(static_cast<float>(sd));
            f.push_back(static_cast<float>(percentile(v, 25)));
            f.push_back(static_cast<float>(percentile(v, 75)));
            f.push_back(static_cast<float>(percentile(v, 90)));
            f.push_back(static_cast<float>(vmax(v)));
        }
    }

    // ——（2）9 分段（linspace(0.16,0.84,9) → 8 段）× 4 通道 → 每通道 8 值 + (min,max,std) ——
    // cuts[i] = 0.16 + i*(0.84-0.16)/8
    std::array<double, 9> cuts;
    for (int i = 0; i < 9; ++i) {
        cuts[i] = 0.16 + i * (0.84 - 0.16) / 8.0;
    }
    const std::array<const cv::Mat*, 4> seg_arrays = { &ctx.gray, &ctx.canny, (horizontal ? &ctx.gy : &ctx.gx),
                                                       &ctx.grad };
    for (const cv::Mat* arr : seg_arrays) {
        std::vector<double> vals;
        vals.reserve(8);
        for (int s = 0; s < 8; ++s) {
            const double t0 = cuts[s];
            const double t1 = cuts[s + 1];
            const double xc = x1 + (x2 - x1) * (t0 + t1) / 2.0;
            const double yc = y1 + (y2 - y1) * (t0 + t1) / 2.0;
            int hw, hh;
            if (horizontal) {
                hw = std::max(2, iround((t1 - t0) * std::abs(x2 - x1) / 2.0));
                hh = std::max(2, iround(4));
            }
            else {
                hw = std::max(2, iround(4));
                hh = std::max(2, iround((t1 - t0) * std::abs(y2 - y1) / 2.0));
            }
            // 注意：Python 此处用 int() 截断而非 round()
            const int y0 = std::max(0, static_cast<int>(yc - hh));
            const int y1r = std::min(h, static_cast<int>(yc + hh + 1));
            const int x0 = std::max(0, static_cast<int>(xc - hw));
            const int x1r = std::min(w, static_cast<int>(xc + hw + 1));
            std::vector<double> v = collect(*arr, y0, y1r, x0, x1r);
            vals.push_back(vmean(v));
        }
        for (double vv : vals) {
            f.push_back(static_cast<float>(vv));
        }
        const double mn = *std::ranges::min_element(vals);
        const double mx = *std::ranges::max_element(vals);
        auto [m2, sd2] = mean_std(vals);
        (void)m2;
        f.push_back(static_cast<float>(mn));
        f.push_back(static_cast<float>(mx));
        f.push_back(static_cast<float>(sd2));
    }

    // ——（3）方向连续性剖面（48 采样） ——
    constexpr int n_samples = 48;
    const int half_span = std::max(8, iround(15));
    const int band_half = std::max(1, iround(2.5));
    const cv::Mat& perp_grad = horizontal ? ctx.gy : ctx.gx;

    std::vector<double> route_value, route_contrast, route_edge;
    route_value.reserve(n_samples);
    route_contrast.reserve(n_samples);
    route_edge.reserve(n_samples);

    for (int s = 0; s < n_samples; ++s) {
        const double t = 0.20 + s * (0.80 - 0.20) / (n_samples - 1);
        const int xc = iround(x1 + (x2 - x1) * t);
        const int yc = iround(y1 + (y2 - y1) * t);

        // 计算 value_profile / edge_profile（沿垂直于行进方向的一维剖面）
        std::vector<double> value_profile, edge_profile;
        if (horizontal) {
            const int ylo = std::max(0, yc - half_span);
            const int yhi = std::min(h, yc + half_span + 1);
            const int xlo = std::max(0, xc - 1);
            const int xhi = std::min(w, xc + 2);
            for (int yy = ylo; yy < yhi; ++yy) {
                double vsum = 0.0, esum = 0.0;
                int cnt = 0;
                for (int xx = xlo; xx < xhi; ++xx) {
                    vsum += ctx.val.ptr<uchar>(yy)[xx];
                    esum += perp_grad.ptr<float>(yy)[xx];
                    ++cnt;
                }
                if (cnt > 0) {
                    value_profile.push_back(vsum / cnt);
                    edge_profile.push_back(esum / cnt);
                }
            }
        }
        else {
            const int xlo = std::max(0, xc - half_span);
            const int xhi = std::min(w, xc + half_span + 1);
            const int ylo = std::max(0, yc - 1);
            const int yhi = std::min(h, yc + 2);
            for (int xx = xlo; xx < xhi; ++xx) {
                double vsum = 0.0, esum = 0.0;
                int cnt = 0;
                for (int yy = ylo; yy < yhi; ++yy) {
                    vsum += ctx.val.ptr<uchar>(yy)[xx];
                    esum += perp_grad.ptr<float>(yy)[xx];
                    ++cnt;
                }
                if (cnt > 0) {
                    value_profile.push_back(vsum / cnt);
                    edge_profile.push_back(esum / cnt);
                }
            }
        }

        const int psize = static_cast<int>(value_profile.size());
        if (psize == 0) {
            route_value.push_back(0.0);
            route_contrast.push_back(0.0);
            route_edge.push_back(0.0);
            continue;
        }
        const double centre = (psize - 1) / 2.0;
        const int search_lo = std::max(0, iround(centre - 10));
        const int search_hi = std::min(psize, iround(centre + 10) + 1);
        double best_value = -1.0, best_edge = 0.0;
        int best_idx = iround(centre);
        for (int j = search_lo; j < search_hi; ++j) {
            const int a = std::max(0, j - band_half);
            const int b = std::min(psize, j + band_half + 1);
            double vsum = 0.0, esum = 0.0;
            for (int k = a; k < b; ++k) {
                vsum += value_profile[k];
                esum += edge_profile[k];
            }
            const int cnt = b - a;
            const double vmean = cnt > 0 ? vsum / cnt : 0.0;
            const double emean = cnt > 0 ? esum / cnt : 0.0;
            const double score = vmean + 0.12 * emean;
            if (score > best_value + 0.12 * best_edge) {
                best_value = vmean;
                best_edge = emean;
                best_idx = j;
            }
        }
        const int exclusion = std::max(3, iround(7));
        std::vector<double> bg;
        const int ex_lo = std::max(0, best_idx - exclusion);
        const int ex_hi = std::min(psize, best_idx + exclusion + 1);
        for (int k = 0; k < psize; ++k) {
            if (k < ex_lo || k >= ex_hi) {
                bg.push_back(value_profile[k]);
            }
        }
        const double background = bg.empty() ? percentile(value_profile, 50) : percentile(bg, 50);
        route_value.push_back(best_value);
        route_contrast.push_back(best_value - background);
        route_edge.push_back(best_edge);
    }

    // route_* 的 9 统计量
    for (const std::vector<double>* vals : { &route_value, &route_contrast, &route_edge }) {
        auto [mean, sd] = mean_std(*vals);
        f.push_back(static_cast<float>(mean));
        f.push_back(static_cast<float>(sd));
        f.push_back(static_cast<float>(percentile(*vals, 10)));
        f.push_back(static_cast<float>(percentile(*vals, 25)));
        f.push_back(static_cast<float>(percentile(*vals, 50)));
        f.push_back(static_cast<float>(percentile(*vals, 75)));
        f.push_back(static_cast<float>(percentile(*vals, 90)));
        f.push_back(static_cast<float>(*std::ranges::min_element(*vals)));
        f.push_back(static_cast<float>(*std::ranges::max_element(*vals)));
    }
    // 阈值占比
    auto frac_ge = [](const std::vector<double>& v, double thr) -> float {
        if (v.empty()) {
            return 0.0f;
        }
        int cnt = 0;
        for (double x : v) {
            if (x >= thr) {
                ++cnt;
            }
        }
        return static_cast<float>(static_cast<double>(cnt) / v.size());
    };
    for (double thr : { 3.0, 6.0, 10.0, 16.0 }) {
        f.push_back(frac_ge(route_contrast, thr));
    }
    for (double thr : { 22.0, 32.0, 48.0, 64.0 }) {
        f.push_back(frac_ge(route_value, thr));
    }
    for (double thr : { 8.0, 16.0, 28.0, 45.0 }) {
        f.push_back(frac_ge(route_edge, thr));
    }

    return f;
}

// ============================================================================
// player_features —— 对应 extract_map.py::_player_features（192 维，scale=1.0）
// ============================================================================
std::vector<float> RoguelikeBlackflowMapAnalyzer::player_features(const PlayerCtx& ctx, float x, float y)
{
    const int h = ctx.gray.rows;
    const int w = ctx.gray.cols;
    const int radius = 62;
    const int xi = iround(x);
    const int yi = iround(y);
    const int xa = std::max(0, xi - radius);
    const int xb = std::min(w, xi + radius + 1);
    const int ya = std::max(0, yi - radius);
    const int yb = std::min(h, yi + radius + 1);
    const int bw = std::max(0, xb - xa);
    const int bh = std::max(0, yb - ya);
    const size_t n = static_cast<size_t>(bw) * bh;

    // 逐像素预计算：g, sat, val, edge, white, strong_white, yellow, dx, dy, rr
    std::vector<double> g(n), sat(n), val(n), edge(n);
    std::vector<uchar> white(n), strong(n), yellow(n);
    std::vector<double> dx(n), dy(n), rr(n);
    for (int i = 0; i < bh; ++i) {
        const int py = ya + i;
        const uchar* p_gray = ctx.gray.ptr<uchar>(py);
        const uchar* p_hue = ctx.hue.ptr<uchar>(py);
        const uchar* p_sat = ctx.sat.ptr<uchar>(py);
        const uchar* p_val = ctx.val.ptr<uchar>(py);
        const uchar* p_canny = ctx.canny.ptr<uchar>(py);
        for (int j = 0; j < bw; ++j) {
            const int px = xa + j;
            const size_t idx = static_cast<size_t>(i) * bw + j;
            const int hh = p_hue[px];
            const int ss = p_sat[px];
            const int vv = p_val[px];
            g[idx] = p_gray[px];
            sat[idx] = ss;
            val[idx] = vv;
            edge[idx] = p_canny[px];
            white[idx] = (vv > 180 && ss < 95) ? 1 : 0;
            strong[idx] = (vv > 225 && ss < 60) ? 1 : 0;
            yellow[idx] = (hh >= 15 && hh <= 42 && ss > 90 && vv > 150) ? 1 : 0;
            dx[idx] = px - x;
            dy[idx] = py - y;
            rr[idx] = std::hypot(dx[idx], dy[idx]);
        }
    }

    std::vector<float> f;
    f.reserve(192);

    // ——（1）5 半径 × 7 通道 × (mean, std, p90, max) ——
    // 通道顺序：g, sat, val, edge, white*255, strong*255, yellow*255
    auto get_channel = [&](int ci, size_t idx) -> double {
        switch (ci) {
        case 0:
            return g[idx];
        case 1:
            return sat[idx];
        case 2:
            return val[idx];
        case 3:
            return edge[idx];
        case 4:
            return white[idx] * 255.0;
        case 5:
            return strong[idx] * 255.0;
        default:
            return yellow[idx] * 255.0;
        }
    };
    static constexpr std::array<double, 5> radii = { 12, 20, 30, 42, 55 };
    for (double r : radii) {
        for (int ci = 0; ci < 7; ++ci) {
            std::vector<double> z;
            z.reserve(n);
            for (size_t k = 0; k < n; ++k) {
                if (rr[k] < r) {
                    z.push_back(get_channel(ci, k));
                }
            }
            auto [mean, sd] = mean_std(z);
            f.push_back(static_cast<float>(mean));
            f.push_back(static_cast<float>(sd));
            f.push_back(static_cast<float>(percentile(z, 90)));
            f.push_back(static_cast<float>(vmax(z)));
        }
    }

    // ——（2）3×3 空间块 × 4 掩膜（white, strong, yellow, edge>0）的 mean ——
    static constexpr std::array<std::pair<int, int>, 3> yranges = { { { -55, -15 }, { -15, 15 }, { 15, 55 } } };
    static constexpr std::array<std::pair<int, int>, 3> xranges = { { { -55, -15 }, { -15, 15 }, { 15, 55 } } };
    for (auto [yy0, yy1] : yranges) {
        for (auto [xx0, xx1] : xranges) {
            std::array<double, 4> sums = { 0, 0, 0, 0 };
            int cnt = 0;
            for (size_t k = 0; k < n; ++k) {
                if (dx[k] >= xx0 && dx[k] < xx1 && dy[k] >= yy0 && dy[k] < yy1) {
                    sums[0] += white[k];
                    sums[1] += strong[k];
                    sums[2] += yellow[k];
                    sums[3] += (edge[k] > 0) ? 1.0 : 0.0;
                    ++cnt;
                }
            }
            for (int m = 0; m < 4; ++m) {
                f.push_back(cnt > 0 ? static_cast<float>(sums[m] / cnt) : 0.0f);
            }
        }
    }

    // ——（3）8 方向放射条 × (strong_white.mean, edge.mean/255) ——
    // angle = 0, π/4, ..., 7π/4
    constexpr double kPi = 3.14159265358979323846;
    for (int a = 0; a < 8; ++a) {
        const double angle = a * (kPi / 4.0);
        const double ux = std::cos(angle);
        const double uy = std::sin(angle);
        double s_sum = 0.0, e_sum = 0.0;
        int cnt = 0;
        for (size_t k = 0; k < n; ++k) {
            const double along = dx[k] * ux + dy[k] * uy;
            const double perp = std::abs(-dx[k] * uy + dy[k] * ux);
            if (along > 18 && along < 55 && perp < 2.5) {
                s_sum += strong[k];
                e_sum += edge[k];
                ++cnt;
            }
        }
        // np.mean of empty -> nan in numpy; but reference masks are非空 in practice。
        // 与 Python 一致：空掩膜时 mean 为 nan，这里退化为 0 以免污染下游（罕见）。
        f.push_back(cnt > 0 ? static_cast<float>(s_sum / cnt) : 0.0f);
        f.push_back(cnt > 0 ? static_cast<float>(e_sum / cnt / 255.0) : 0.0f);
    }

    return f;
}

// ============================================================================
// detect_lattice —— 对应 extract_map.py::_detect_lattice（scale=1.0）
// ============================================================================
RoguelikeBlackflowMapAnalyzer::Lattice
    RoguelikeBlackflowMapAnalyzer::detect_lattice(const cv::Mat& gray) const
{
    Lattice lat;
    const int h = gray.rows;
    const int w = gray.cols;
    const int step = std::max(20, iround(w / BASE_WIDTH * BASE_GRID_STEP));

    const int x0 = static_cast<int>(0.055 * w);
    const int x1 = static_cast<int>(0.883 * w);
    const int y0 = static_cast<int>(0.125 * h);
    const int y1 = static_cast<int>(0.86 * h);
    if (x1 <= x0 || y1 <= y0) {
        return lat;
    }

    cv::Mat roi;
    cv::GaussianBlur(gray(cv::Rect(x0, y0, x1 - x0, y1 - y0)), roi, cv::Size(5, 5), 1.2);

    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(
        roi,
        circles,
        cv::HOUGH_GRADIENT,
        1.2,
        std::max(10, static_cast<int>(0.009 * w)), // minDist
        80,                                        // param1
        18,                                        // param2
        std::max(4, static_cast<int>(0.004 * w)),  // minRadius
        std::max(20, static_cast<int>(0.028 * w))  // maxRadius
    );
    if (circles.empty()) {
        Log.error(__FUNCTION__, "| no node-like circles found");
        return lat;
    }
    // 偏移回全图坐标
    for (auto& c : circles) {
        c[0] += x0;
        c[1] += y0;
    }

    // 2D 相位投票：dx[p][m] = min((px-m)%step,(m-px)%step) < tol
    const double tol = std::max(4.0, 0.004 * w);
    const size_t np = circles.size();
    // 对每个候选相位 m 统计 x/y 命中的点集合（布尔矩阵按列压缩）
    std::vector<std::vector<uint8_t>> hitx(step, std::vector<uint8_t>(np, 0));
    std::vector<std::vector<uint8_t>> hity(step, std::vector<uint8_t>(np, 0));
    for (size_t p = 0; p < np; ++p) {
        const double px = circles[p][0];
        const double py = circles[p][1];
        for (int m = 0; m < step; ++m) {
            const double rx1 = std::fmod(px - m, step);
            const double rx2 = std::fmod(m - px, step);
            const double dxm = std::min((rx1 < 0 ? rx1 + step : rx1), (rx2 < 0 ? rx2 + step : rx2));
            if (dxm < tol) {
                hitx[m][p] = 1;
            }
            const double ry1 = std::fmod(py - m, step);
            const double ry2 = std::fmod(m - py, step);
            const double dym = std::min((ry1 < 0 ? ry1 + step : ry1), (ry2 < 0 ? ry2 + step : ry2));
            if (dym < tol) {
                hity[m][p] = 1;
            }
        }
    }
    // votes[xphase][yphase] = sum_p hitx[xphase][p] * hity[yphase][p]，取 argmax
    int best_xphase = 0, best_yphase = 0;
    long best_votes = -1;
    for (int mx = 0; mx < step; ++mx) {
        for (int my = 0; my < step; ++my) {
            long v = 0;
            for (size_t p = 0; p < np; ++p) {
                v += static_cast<long>(hitx[mx][p]) * hity[my][p];
            }
            if (v > best_votes) {
                best_votes = v;
                best_xphase = mx;
                best_yphase = my;
            }
        }
    }

    // _positions(phase, step, lo, hi)
    auto positions = [step](int phase, double lo, double hi) -> std::vector<float> {
        const int k0 = static_cast<int>(std::ceil((lo - phase) / step));
        const int k1 = static_cast<int>(std::floor((hi - phase) / step));
        std::vector<float> out;
        for (int k = k0; k <= k1; ++k) {
            out.push_back(static_cast<float>(phase + k * step));
        }
        return out;
    };
    lat.xs = positions(best_xphase, 0.055 * w, 0.883 * w);
    lat.ys = positions(best_yphase, 0.14 * h, 0.84 * h);
    if (lat.xs.size() < 2 || lat.ys.size() < 2) {
        Log.error(__FUNCTION__, "| failed to construct usable lattice");
        return lat;
    }
    lat.step = step;
    lat.circles = std::move(circles);
    lat.valid = true;
    return lat;
}

// ============================================================================
// ONNX 推理：TreeEnsembleClassifier，返回每行概率（rows x n_classes）
// ============================================================================
std::vector<std::vector<float>> RoguelikeBlackflowMapAnalyzer::run_tree_ensemble(
    const std::string& model_name,
    const std::vector<std::vector<float>>& feats,
    int n_features)
{
    std::vector<std::vector<float>> out;
    if (feats.empty()) {
        return out;
    }
    auto& session = OnnxSessions::get_instance().get(model_name);

    const int64_t batch = static_cast<int64_t>(feats.size());
    std::vector<float> input;
    input.reserve(feats.size() * n_features);
    for (const auto& row : feats) {
        input.insert(input.end(), row.begin(), row.end());
    }

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    std::array<int64_t, 2> input_shape { batch, n_features };
    Ort::Value input_tensor =
        Ort::Value::CreateTensor<float>(memory_info, input.data(), input.size(), input_shape.data(), 2);

    // skl2onnx 输出：output[0]=label(int64), output[1]=probabilities(float [N, n_classes])
    const char* input_names[] = { "X" };
    const char* output_names[] = { "label", "probabilities" };

    Ort::RunOptions run_options;
    auto outputs = session.Run(run_options, input_names, &input_tensor, 1, output_names, 2);

    const Ort::Value& prob = outputs[1];
    auto shape = prob.GetTensorTypeAndShapeInfo().GetShape();
    const int64_t rows = shape[0];
    const int64_t cls = shape.size() >= 2 ? shape[1] : 1;
    const float* pdata = prob.GetTensorData<float>();
    out.resize(static_cast<size_t>(rows));
    for (int64_t i = 0; i < rows; ++i) {
        out[i].assign(pdata + i * cls, pdata + (i + 1) * cls);
    }
    return out;
}

bool RoguelikeBlackflowMapAnalyzer::nameplate_has_check(float cx, float cy) const
{
    // 铭牌左侧的绿色 ✓：在铭牌左端外侧一小块统计绿色像素比例
    const int x = std::clamp(iround(cx - 60), 0, m_image.cols - 1);
    const int y = std::clamp(iround(cy + 14), 0, m_image.rows - 1);
    const int rw = std::min(24, m_image.cols - x);
    const int rh = std::min(28, m_image.rows - y);
    if (rw <= 0 || rh <= 0) {
        return false;
    }
    cv::Mat hsv;
    cv::cvtColor(m_image(cv::Rect(x, y, rw, rh)), hsv, cv::COLOR_BGR2HSV);
    // 绿色 HSV 区间（OpenCV H:0-179）
    cv::Mat mask;
    cv::inRange(hsv, cv::Scalar(35, 80, 80), cv::Scalar(85, 255, 255), mask);
    const double ratio = cv::countNonZero(mask) / static_cast<double>(rw * rh);
    return ratio > 0.06;
}

// ============================================================================
// largest_component —— 对应 extract_map.py::_largest_component
// 选择边最多、其次节点最多的连通分量
// ============================================================================
std::vector<std::pair<int, int>> RoguelikeBlackflowMapAnalyzer::largest_component(
    const std::vector<std::pair<int, int>>& vertices,
    const std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>>& edges)
{
    std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> adj;
    for (const auto& v : vertices) {
        adj[v];
    }
    for (const auto& [a, b] : edges) {
        adj[a].push_back(b);
        adj[b].push_back(a);
    }
    std::set<std::pair<int, int>> seen;
    std::vector<std::vector<std::pair<int, int>>> components;
    for (const auto& [start, _] : adj) {
        if (seen.count(start)) {
            continue;
        }
        std::vector<std::pair<int, int>> comp;
        std::vector<std::pair<int, int>> stack = { start };
        while (!stack.empty()) {
            auto v = stack.back();
            stack.pop_back();
            if (seen.count(v)) {
                continue;
            }
            seen.insert(v);
            comp.push_back(v);
            for (const auto& nb : adj[v]) {
                if (!seen.count(nb)) {
                    stack.push_back(nb);
                }
            }
        }
        components.push_back(std::move(comp));
    }
    if (components.empty()) {
        return {};
    }
    auto edge_count = [&](const std::vector<std::pair<int, int>>& comp) -> int {
        std::set<std::pair<int, int>> cs(comp.begin(), comp.end());
        int cnt = 0;
        for (const auto& [a, b] : edges) {
            if (cs.count(a) && cs.count(b)) {
                ++cnt;
            }
        }
        return cnt;
    };
    return *std::ranges::max_element(components, [&](const auto& c1, const auto& c2) {
        const int e1 = edge_count(c1), e2 = edge_count(c2);
        if (e1 != e2) {
            return e1 < e2;
        }
        return c1.size() < c2.size();
    });
}

// ============================================================================
// analyze —— 主流程（对应 extract_map.py::extract + 节点类型分类模型）
// ============================================================================
RoguelikeBlackflowMapAnalyzer::Result RoguelikeBlackflowMapAnalyzer::analyze()
{
    LogTraceFunction;

    Result result;
    if (m_image.empty() || m_image.channels() != 3) {
        Log.error(__FUNCTION__, "| invalid image");
        return result;
    }

    const NodeCtx node_ctx = make_node_ctx(m_image);
    const Lattice lat = detect_lattice(node_ctx.gray);
    if (!lat.valid) {
        return result;
    }
    const EdgeCtx edge_ctx = make_edge_ctx(m_image);
    const PlayerCtx player_ctx = make_player_ctx(m_image);

    const int ncols = static_cast<int>(lat.xs.size());
    const int nrows = static_cast<int>(lat.ys.size());

    Log.debug(
        __FUNCTION__,
        "| lattice step",
        lat.step,
        "| lattice",
        ncols,
        "x",
        nrows,
        "| x range",
        lat.xs.front(),
        "..",
        lat.xs.back(),
        "| y range",
        lat.ys.front(),
        "..",
        lat.ys.back());

    // 所有格坐标（顺序：iy 外层、ix 内层，与 Python cells 一致）
    std::vector<std::pair<int, int>> cells;
    cells.reserve(static_cast<size_t>(ncols) * nrows);
    for (int iy = 0; iy < nrows; ++iy) {
        for (int ix = 0; ix < ncols; ++ix) {
            cells.emplace_back(ix, iy);
        }
    }

    // —— 节点分类 ——
    std::vector<std::vector<float>> node_feats;
    node_feats.reserve(cells.size());
    for (auto [ix, iy] : cells) {
        node_feats.push_back(node_features(node_ctx, lat.xs[ix], lat.ys[iy], lat.circles));
    }
    const auto node_prob = run_tree_ensemble("Blackflow_node", node_feats, 181);
    // node 类别顺序：['none','object','road']（sklearn classes_ 已验证）
    std::vector<int> node_kind(cells.size(), 0); // 0=none,1=object,2=road
    std::vector<double> node_conf(cells.size(), 0.0);
    for (size_t i = 0; i < cells.size(); ++i) {
        const auto& p = node_prob[i];
        const int argmax = static_cast<int>(std::ranges::max_element(p) - p.begin());
        node_kind[i] = argmax;
        node_conf[i] = 1.0 - (p.empty() ? 1.0 : p[0]); // 1 - P(none)
    }

    // —— 玩家 ——
    std::vector<std::vector<float>> player_feats;
    player_feats.reserve(cells.size());
    for (auto [ix, iy] : cells) {
        player_feats.push_back(player_features(player_ctx, lat.xs[ix], lat.ys[iy]));
    }
    const auto player_prob = run_tree_ensemble("Blackflow_player", player_feats, 192);
    size_t player_idx = 0;
    double best_pscore = -1.0;
    for (size_t i = 0; i < cells.size(); ++i) {
        // player 类别 [0,1]，取 class-1 概率（skl2onnx bug：只能读 [:,1]）
        const double s = player_prob[i].size() >= 2 ? player_prob[i][1] : 0.0;
        if (s > best_pscore) {
            best_pscore = s;
            player_idx = i;
        }
    }
    const auto player_cell = cells[player_idx];

    for (size_t i = 0; i < cells.size(); ++i) {
        const auto [ix, iy] = cells[i];
        const double player_score = player_prob[i].size() >= 2 ? player_prob[i][1] : 0.0;
        Log.debug(
            __FUNCTION__,
            "| candidate",
            ix,
            iy,
            "node_kind",
            node_kind[i],
            "occupied",
            node_conf[i],
            "player",
            player_score,
            (i == player_idx ? "<player>" : ""));
    }

    // —— 节点类型分类：对 object 节点裁剪 92x92 图块，使用 2118 维特征模型 ——
    // key: (ix,iy) -> RoguelikeNodeType；同时记录分类标签与 visited
    std::map<std::pair<int, int>, RoguelikeNodeType> obj_type;
    std::map<std::pair<int, int>, std::string> obj_text;
    std::map<std::pair<int, int>, bool> obj_visited;
    std::set<std::pair<int, int>> road_cells;

    static constexpr std::array<std::string_view, 20> node_type_labels = {
        "Boons",           "BoskyPassage",      "CombatOps",        "DreadfulFoe",       "EmergencyAid",
        "EmergencyOps",    "Encounter",         "FaceOff",          "FerociousPresage",  "HiddenTrader",
        "LostAndFound",    "MysteriousPresage", "PathEnd",          "PathLane",           "ResidentStronghold",
        "RogueTrader",     "SafeHouse",         "Scout",             "VantagePoint",      "WindingPassage",
    };
    std::vector<std::pair<int, int>> object_cells;
    std::vector<std::vector<float>> object_type_feats;
    for (size_t i = 0; i < cells.size(); ++i) {
        if (node_kind[i] == 2 && node_conf[i] >= 0.50) {
            // 布局模型已经确认是道路，直接使用道路结果，不进入节点详细分类模型。
            road_cells.insert(cells[i]);
            continue;
        }
        if (node_kind[i] != 1 || node_conf[i] < 0.50) {
            continue;
        }
        const auto cell = cells[i];
        object_cells.push_back(cell);
        object_type_feats.push_back(node_type_features(m_image, lat.xs[cell.first], lat.ys[cell.second]));
    }
    std::vector<std::vector<float>> object_type_prob;
    if (!object_type_feats.empty()) {
        object_type_prob = run_tree_ensemble("Blackflow_node_type", object_type_feats, 2118);
    }
    for (size_t i = 0; i < object_cells.size(); ++i) {
        const auto& probabilities = object_type_prob[i];
        const auto best = std::ranges::max_element(probabilities);
        const int label_index = best == probabilities.end() ? -1 : static_cast<int>(best - probabilities.begin());
        const double confidence = best == probabilities.end() ? 0.0 : *best;
        std::string label;
        RoguelikeNodeType type = RoguelikeNodeType::Unknown;
        if (label_index >= 0 && label_index < static_cast<int>(node_type_labels.size())) {
            label = std::string(node_type_labels[static_cast<size_t>(label_index)]);
            type = RoguelikeMapConfig::name2type(label);
        }
        const auto cell = object_cells[i];
        obj_type[cell] = type;
        obj_text[cell] = label;
        obj_visited[cell] = nameplate_has_check(lat.xs[cell.first], lat.ys[cell.second]);
        Log.info(
            __FUNCTION__,
            "| object",
            cell.first,
            cell.second,
            "classifier",
            label.empty() ? "Unknown" : label,
            "confidence",
            confidence,
            "type",
            type == RoguelikeNodeType::Unknown ? "Unknown" : RoguelikeMapConfig::type2name(type));
    }

    // —— 顶点集合：node!=none 且 conf>=0.5 ——
    std::set<std::pair<int, int>> vertices;
    for (size_t i = 0; i < cells.size(); ++i) {
        if (node_kind[i] == 0 || node_conf[i] < 0.50) {
            continue;
        }
        const auto cell = cells[i];
        vertices.insert(cell);
    }
    vertices.insert(player_cell);

    Log.info(__FUNCTION__, "| vertices before edge inference", vertices.size());

    // —— 边：仅两端都是顶点的相邻对 ——
    std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> pair_cells;
    std::vector<std::vector<float>> pair_feats;
    for (int iy = 0; iy < nrows; ++iy) {
        for (int ix = 0; ix < ncols; ++ix) {
            const std::pair<int, int> a { ix, iy };
            if (ix + 1 < ncols) {
                const std::pair<int, int> b { ix + 1, iy };
                if (vertices.count(a) && vertices.count(b)) {
                    pair_cells.emplace_back(a, b);
                    pair_feats.push_back(edge_features(edge_ctx, lat.xs[ix], lat.ys[iy], lat.xs[ix + 1], lat.ys[iy]));
                }
            }
            if (iy + 1 < nrows) {
                const std::pair<int, int> b { ix, iy + 1 };
                if (vertices.count(a) && vertices.count(b)) {
                    pair_cells.emplace_back(a, b);
                    pair_feats.push_back(edge_features(edge_ctx, lat.xs[ix], lat.ys[iy], lat.xs[ix], lat.ys[iy + 1]));
                }
            }
        }
    }
    std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> edges;
    if (!pair_feats.empty()) {
        const auto edge_prob = run_tree_ensemble("Blackflow_edge", pair_feats, 378);
        for (size_t i = 0; i < pair_cells.size(); ++i) {
            // edge 类别 [0,1]，读 class-1 概率
            const double p = edge_prob[i].size() >= 2 ? edge_prob[i][1] : 0.0;
            if (p >= 0.50) {
                edges.push_back(pair_cells[i]);
            }
        }
    }

    // —— 连通分量裁剪 ——
    std::vector<std::pair<int, int>> vertex_vec(vertices.begin(), vertices.end());
    const auto component = largest_component(vertex_vec, edges);
    if (component.size() < 2) {
        Log.error(__FUNCTION__, "| no connected map component");
        return result;
    }
    int min_ix = component.front().first, max_ix = component.front().first;
    int min_iy = component.front().second, max_iy = component.front().second;
    for (const auto& [cx, cy] : component) {
        min_ix = std::min(min_ix, cx);
        max_ix = std::max(max_ix, cx);
        min_iy = std::min(min_iy, cy);
        max_iy = std::max(max_iy, cy);
    }

    std::set<std::pair<int, int>> comp_set(component.begin(), component.end());
    // active = 分量 ∪ (bbox 内的顶点)
    std::set<std::pair<int, int>> active = comp_set;
    for (const auto& v : vertices) {
        if (min_ix <= v.first && v.first <= max_ix && min_iy <= v.second && v.second <= max_iy) {
            active.insert(v);
        }
    }
    std::set<std::pair<std::pair<int, int>, std::pair<int, int>>> active_edges;
    for (const auto& [a, b] : edges) {
        if (active.count(a) && active.count(b) && min_ix <= a.first && a.first <= max_ix && min_iy <= a.second &&
            a.second <= max_iy) {
            active_edges.insert({ a, b });
        }
    }

    // —— 组装 Result（坐标平移到 bbox 原点） ——
    result.cols = max_ix - min_ix + 1;
    result.rows = max_iy - min_iy + 1;
    for (const auto& [ix, iy] : active) {
        Cell cell;
        cell.col = ix - min_ix;
        cell.row = iy - min_iy;
        cell.center = Point(iround(lat.xs[ix]), iround(lat.ys[iy]));
        if (road_cells.count({ ix, iy })) {
            // node 模型已确认道路；道路节点不需要详细类型和访问状态。
            cell.kind = CellKind::Road;
            cell.type = RoguelikeNodeType::Unknown;
        }
        else if (obj_type.count({ ix, iy })) {
            cell.kind = CellKind::Object;
            cell.type = obj_type[{ ix, iy }];
            cell.ocr_text = obj_text[{ ix, iy }];
            cell.visited = obj_visited[{ ix, iy }];
        }
        else {
            cell.kind = CellKind::Road;
            cell.type = RoguelikeNodeType::Unknown;
        }
        result.cells.push_back(cell);
    }
    for (const auto& [a, b] : active_edges) {
        result.edges.push_back(
            { { a.first - min_ix, a.second - min_iy }, { b.first - min_ix, b.second - min_iy } });
    }
    result.player = { player_cell.first - min_ix, player_cell.second - min_iy };
    result.valid = true;

    Log.info(
        __FUNCTION__,
        "| map",
        result.cols,
        "x",
        result.rows,
        "| nodes",
        result.cells.size(),
        "| edges",
        result.edges.size(),
        "| player",
        result.player.first,
        result.player.second);

#ifdef ASST_DEBUG
    cv::Mat draw = m_image.clone();
    for (const auto& c : result.cells) {
        const cv::Scalar color = c.kind == CellKind::Object ? cv::Scalar(0, 0, 255) : cv::Scalar(255, 255, 0);
        cv::circle(draw, cv::Point(c.center.x, c.center.y), 20, color, 2);
        std::string label = (c.kind == CellKind::Object ? RoguelikeMapConfig::type2name(c.type) : "road");
        if (c.visited) {
            label += "*";
        }
        cv::putText(draw, label, cv::Point(c.center.x - 40, c.center.y - 24), cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);
    }
    // 玩家格
    for (const auto& c : result.cells) {
        if (c.col == result.player.first && c.row == result.player.second) {
            cv::circle(draw, cv::Point(c.center.x, c.center.y), 26, cv::Scalar(0, 255, 255), 3);
        }
    }
    // 边（用中心连线）
    auto center_of = [&](int col, int row) -> std::optional<cv::Point> {
        for (const auto& c : result.cells) {
            if (c.col == col && c.row == row) {
                return cv::Point(c.center.x, c.center.y);
            }
        }
        return std::nullopt;
    };
    for (const auto& [a, b] : result.edges) {
        auto pa = center_of(a.first, a.second);
        auto pb = center_of(b.first, b.second);
        if (pa && pb) {
            cv::line(draw, *pa, *pb, cv::Scalar(255, 255, 255), 1);
        }
    }
    utils::save_debug_image(draw, utils::path("debug") / "roguelikeMap", true, "blackflow map", "draw");
#endif

    return result;
}
