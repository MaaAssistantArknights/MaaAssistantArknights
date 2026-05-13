#include "Matcher.h"

#include "MaaUtils/NoWarningCV.hpp"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "MaaUtils/ImageIo.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

#include <mutex>
#include <unordered_map>

using namespace asst;

Matcher::ResultOpt Matcher::analyze() const
{
    if (m_roi.empty()) {
        return std::nullopt;
    }
    const auto match_results = preproc_and_match(make_roi(m_image, m_roi), m_params);

    for (size_t i = 0; i < match_results.size(); ++i) {
        const auto& [matched, templ, templ_name] = match_results[i];
        if (matched.empty()) {
            continue;
        }

        double min_val = 0.0, max_val = 0.0;
        cv::Point min_loc, max_loc;
        cv::Mat valid_mask;
        cv::inRange(matched, 0.0f, 1.0f + 1e-5f, valid_mask);
        cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc, valid_mask);

        Rect rect(max_loc.x + m_roi.x, max_loc.y + m_roi.y, templ.cols, templ.rows);

        double threshold = m_params.templ_thres[i];
        if (m_log_tracing && max_val > 0.5 && max_val > threshold - 0.2) { // 得分太低的肯定不对，没必要打印
            Log.trace("match_templ |", templ_name, "score:", max_val, "rect:", rect, "roi:", m_roi);
#ifdef ASST_DEBUG
            if (!m_params.methods.empty() && m_params.methods[0] == MatchMethod::HSVCount) {
                const cv::Rect expanded_roi(
                    std::max(rect.x - 200, 0),
                    std::max(rect.y - 50, 0),
                    std::min(rect.width + 400, m_image.cols - std::max(rect.x - 200, 0)),
                    std::min(rect.height + 100, m_image.rows - std::max(rect.y - 50, 0)));
                cv::Mat cropped = m_image(expanded_roi).clone();
                const cv::Rect roi_in_cropped(
                    rect.x - expanded_roi.x,
                    rect.y - expanded_roi.y,
                    rect.width,
                    rect.height);
                cv::rectangle(cropped, roi_in_cropped, cv::Scalar(0, 0, 255), 1);
                const std::string name = std::filesystem::path(templ_name).stem().string();
                const std::string text = name + " " + std::to_string(max_val);
                const cv::Size text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, nullptr);
                const cv::Point text_pos(
                    std::max(roi_in_cropped.x + roi_in_cropped.width / 2 - text_size.width / 2, 0),
                    std::max(roi_in_cropped.y - 5, text_size.height));
                cv::putText(cropped, text, text_pos, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);

                const static std::vector<int> jpeg_params = { cv::IMWRITE_JPEG_QUALITY,
                                                              95,
                                                              cv::IMWRITE_JPEG_OPTIMIZE,
                                                              1 };
                utils::save_debug_image(cropped, utils::path("debug") / "hsv", true, text, "", "jpeg", jpeg_params);
            }
#endif
        }
        else {
            Log.debug("match_templ |", templ_name, "score:", max_val, "rect:", rect, "roi:", m_roi);
        }
        if (max_val < threshold) {
            continue;
        }

        // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
        m_result.rect = rect;
        m_result.score = max_val;
        m_result.templ_name = templ_name;
        return m_result;
    }

    return std::nullopt;
}

// 模板侧 FFT 缓存
// 每个 (templ_name, mask_params, dft_size) 只计算一次 FFT(T'_c) 和 FFT(M)。
// cv::Mat 使用引用计数，缓存存储开销低（不会深拷贝像素数据）。

// 稀疏路径使用的每个有效 mask 像素的条目
struct SparseEntry
{
    int16_t dx, dy;        // 相对模板左上角的偏移
    float   T_prime[3];    // M*(T_c - μT_c)，per-channel
};

struct MaskedMatchTemplFFT
{
    // FFT 路径
    std::array<cv::Mat, 3> T_prime_dft; // FFT(M*(T_c - μT_c))，每通道一个
    cv::Mat                M_dft;       // FFT(M)
    double                 sigma_T_sq = 0.0;
    double                 mask_area  = 0.0;

    // 稀疏路径
    std::vector<SparseEntry> sparse_entries; // 非零 mask 位置列表
    int K = 0;                               // sparse_entries.size()
};

static std::mutex s_fft_cache_mtx;
static std::unordered_map<std::string, MaskedMatchTemplFFT> s_fft_cache;

// 用 cv::dft 直接实现，消除冗余 FFT
//
// 当前 9 次 matchTemplate 的冗余：
//   FFT(I_c)  每通道算两次（分别用于 xcorr(T'_c, I_c) 和 xcorr(M, I_c)）
//   FFT(M)    每通道算两次（分别用于 xcorr(M, I_c) 和 xcorr(M, I_c²)）
//
// 优化后：
//   FFT(I_c) 和 FFT(I_c²) 每通道各算一次并复用
//   FFT(T'_c) 和 FFT(M) 通过缓存跨调用复用
//
// 数学等价于 cv::matchTemplate(image, templ, result, TM_CCOEFF_NORMED, mask)
static cv::Mat fast_masked_ccoeff_normed(
    const cv::Mat& image_rgb,      // CV_8UC3
    const cv::Mat& templ_rgb,      // CV_8UC3
    const cv::Mat& mask_u8,        // CV_8UC1, 0 or 255
    const std::string& cache_key)  // 模板侧 FFT 缓存键
{
    cv::Mat I, T, M;
    image_rgb.convertTo(I, CV_32F);
    templ_rgb.convertTo(T, CV_32F);
    mask_u8.convertTo(M, CV_32F, 1.0 / 255.0);

    const int rh = I.rows - T.rows + 1;
    const int rw = I.cols - T.cols + 1;
    if (rh <= 0 || rw <= 0) return {};

    // DFT 的填充尺寸：用于 cache key（buffer 延迟到确认走 FFT 路径后再分配）
    const int dft_rows = cv::getOptimalDFTSize(I.rows + T.rows - 1);
    const int dft_cols = cv::getOptimalDFTSize(I.cols + T.cols - 1);

    // ---- 读取或计算模板侧 FFT 缓存 ---
    // key 包含 dft_size，不同图像尺寸会产生不同填充，分开存储
    const std::string full_key =
        cache_key + ":" + std::to_string(dft_rows) + "x" + std::to_string(dft_cols);

    MaskedMatchTemplFFT local_cache;
    {
        std::lock_guard<std::mutex> lk(s_fft_cache_mtx);
        auto it = s_fft_cache.find(full_key);
        if (it != s_fft_cache.end()) {
            local_cache = it->second; // cv::Mat 浅拷贝，不复制像素
        }
    }

    if (local_cache.mask_area < 1.0) {
        // 缓存未命中，一次性计算 FFT 路径和稀疏路径所需的所有模板侧数据
        const double mask_area = cv::sum(M)[0];
        if (mask_area < 1.0) return {};

        local_cache.mask_area = mask_area;

        // cache-miss 时用局部临时 padded，避免为大 DFT buffer 提前分配
        cv::Mat init_pad = cv::Mat::zeros(dft_rows, dft_cols, CV_32F);
        auto compute_into = [&](const cv::Mat& src, cv::Mat& out) {
            init_pad.setTo(0.0f);
            src.copyTo(init_pad(cv::Rect(0, 0, src.cols, src.rows)));
            cv::dft(init_pad, out, cv::DFT_COMPLEX_OUTPUT);
        };
        compute_into(M, local_cache.M_dft);

        std::vector<cv::Mat> T_ch(3);
        cv::split(T, T_ch);

        // 先把三通道的 T_prime 全部算出来，后续 FFT 和稀疏都用这批数据
        std::array<cv::Mat, 3> T_prime_mats;
        for (int c = 0; c < 3; ++c) {
            const double mu_T = cv::sum(M.mul(T_ch[c]))[0] / mask_area;
            T_prime_mats[c]   = M.mul(T_ch[c] - mu_T);
            local_cache.sigma_T_sq += cv::sum(T_prime_mats[c].mul(T_prime_mats[c]))[0];
            compute_into(T_prime_mats[c], local_cache.T_prime_dft[c]);
        }

        // 稀疏路径：收集所有非零 mask 位置及其 T' 值
        for (int v = 0; v < T.rows; ++v) {
            for (int u = 0; u < T.cols; ++u) {
                if (M.at<float>(v, u) > 0.5f) {
                    SparseEntry e{};
                    e.dx = static_cast<int16_t>(u);
                    e.dy = static_cast<int16_t>(v);
                    for (int c = 0; c < 3; ++c)
                        e.T_prime[c] = T_prime_mats[c].at<float>(v, u);
                    local_cache.sparse_entries.push_back(e);
                }
            }
        }
        local_cache.K = static_cast<int>(local_cache.sparse_entries.size());

        std::lock_guard<std::mutex> lk(s_fft_cache_mtx);
        s_fft_cache.emplace(full_key, local_cache);
    }

    const double mask_area  = local_cache.mask_area;
    const double sigma_T_sq = local_cache.sigma_T_sq;

    // 图像通道拆分：稀疏和 FFT 两条路径都需要
    std::vector<cv::Mat> I_ch(3);
    cv::split(I, I_ch);

    // ---- 稀疏直接相关（小模板快路径）----------------------
    // 双重条件：K < SPARSE_K_LIMIT 且总工作量 K×result_positions < SPARSE_WORK_LIMIT。
    // 仅满足 K 小但结果矩阵极大时（如 49×28 模板/690×434 图）仍走 FFT 路径。
    static constexpr int       SPARSE_K_LIMIT    = 2000;
    static constexpr long long SPARSE_WORK_LIMIT = 30'000'000LL;
    if (local_cache.K > 0 && local_cache.K < SPARSE_K_LIMIT &&
        static_cast<long long>(local_cache.K) * rh * rw < SPARSE_WORK_LIMIT) {
        cv::Mat numerator  = cv::Mat::zeros(rh, rw, CV_32F);
        cv::Mat sum_MI_r   = cv::Mat::zeros(rh, rw, CV_32F);
        cv::Mat sum_MI_g   = cv::Mat::zeros(rh, rw, CV_32F);
        cv::Mat sum_MI_b   = cv::Mat::zeros(rh, rw, CV_32F);
        cv::Mat sum_MI2    = cv::Mat::zeros(rh, rw, CV_32F); // Σ_c I_c²

        for (const auto& e : local_cache.sparse_entries) {
            for (int y = 0; y < rh; ++y) {
                const float* Ir = I_ch[0].ptr<float>(y + e.dy) + e.dx;
                const float* Ig = I_ch[1].ptr<float>(y + e.dy) + e.dx;
                const float* Ib = I_ch[2].ptr<float>(y + e.dy) + e.dx;
                float* num_p  = numerator.ptr<float>(y);
                float* smir_p = sum_MI_r.ptr<float>(y);
                float* smig_p = sum_MI_g.ptr<float>(y);
                float* smib_p = sum_MI_b.ptr<float>(y);
                float* smi2_p = sum_MI2.ptr<float>(y);

                // 内层循环：连续内存，编译器向量化
                for (int x = 0; x < rw; ++x) {
                    const float r = Ir[x], g = Ig[x], b = Ib[x];
                    num_p[x]  += e.T_prime[0]*r + e.T_prime[1]*g + e.T_prime[2]*b;
                    smir_p[x] += r;
                    smig_p[x] += g;
                    smib_p[x] += b;
                    smi2_p[x] += r*r + g*g + b*b;
                }
            }
        }

        // sigma_I² = sum_MI2 - (sum_MI_r² + sum_MI_g² + sum_MI_b²) / mask_area
        cv::Mat sq_sum, sq_g, sq_b;
        cv::multiply(sum_MI_r, sum_MI_r, sq_sum);
        cv::multiply(sum_MI_g, sum_MI_g, sq_g);
        cv::multiply(sum_MI_b, sum_MI_b, sq_b);
        cv::add(sq_sum, sq_g, sq_sum);
        cv::add(sq_sum, sq_b, sq_sum);
        cv::Mat sigma_I_sq;
        cv::subtract(sum_MI2, sq_sum * (1.0 / mask_area), sigma_I_sq);
        cv::max(sigma_I_sq, 0.0, sigma_I_sq);

        cv::Mat denom;
        cv::sqrt(sigma_I_sq * sigma_T_sq, denom);
        cv::Mat result;
        cv::divide(numerator, denom, result);
        cv::patchNaNs(result, 0.0);
        const float sigma_T_norm = static_cast<float>(std::sqrt(sigma_T_sq));
        result.setTo(0.0f, denom < sigma_T_norm * 1e-5f);
        cv::min(result,  1.0f, result);
        cv::max(result, -1.0f, result);
        return result;
    }

    // ---- FFT buffer 仅在确认走 FFT 路径后才分配 ---
    cv::Mat padded(dft_rows, dft_cols, CV_32F, cv::Scalar(0));
    cv::Mat I_dft(dft_rows, dft_cols, CV_32FC2);
    cv::Mat I_sq_dft(dft_rows, dft_cols, CV_32FC2);
    cv::Mat spectrum(dft_rows, dft_cols, CV_32FC2);
    cv::Mat result_buf(dft_rows, dft_cols, CV_32F);
    cv::Mat sum_MI_buf(rh, rw, CV_32F);
    cv::Mat sum_MI2_buf(rh, rw, CV_32F);

    auto make_dft_into = [&](const cv::Mat& src, cv::Mat& out) {
        padded.setTo(0.0f);
        src.copyTo(padded(cv::Rect(0, 0, src.cols, src.rows)));
        cv::dft(padded, out, cv::DFT_COMPLEX_OUTPUT);
    };
    auto xcorr_into = [&](const cv::Mat& dft_A, const cv::Mat& dft_B, cv::Mat& out) {
        cv::mulSpectrums(dft_A, dft_B, spectrum, 0, true);
        cv::dft(spectrum, result_buf, cv::DFT_INVERSE | cv::DFT_REAL_OUTPUT | cv::DFT_SCALE);
        result_buf(cv::Rect(0, 0, rw, rh)).copyTo(out);
    };
    auto xcorr_add = [&](const cv::Mat& dft_A, const cv::Mat& dft_B, cv::Mat& accum) {
        cv::mulSpectrums(dft_A, dft_B, spectrum, 0, true);
        cv::dft(spectrum, result_buf, cv::DFT_INVERSE | cv::DFT_REAL_OUTPUT | cv::DFT_SCALE);
        cv::add(accum, result_buf(cv::Rect(0, 0, rw, rh)), accum);
    };

    // 图像侧每通道 FFT 只算一次
    cv::Mat numerator     = cv::Mat::zeros(rh, rw, CV_32F);
    cv::Mat sigma_I_sq_d  = cv::Mat::zeros(rh, rw, CV_64F); // float64 避免 sum_MI2-sum_MI² 灾难性精度损失

    for (int c = 0; c < 3; ++c) {
        make_dft_into(I_ch[c], I_dft);
        make_dft_into(I_ch[c].mul(I_ch[c]), I_sq_dft);

        // numerator += xcorr(T'_c, I_c)
        xcorr_add(I_dft, local_cache.T_prime_dft[c], numerator);

        // σ_I_c²(x,y) = sum_MI2 - sum_MI² / mask_area
        // 两项量级相近时相减会损失精度，用 float64 累加
        xcorr_into(I_dft,    local_cache.M_dft, sum_MI_buf);
        xcorr_into(I_sq_dft, local_cache.M_dft, sum_MI2_buf);
        cv::Mat sum_MI_d, sum_MI2_d, var_d;
        sum_MI_buf.convertTo(sum_MI_d,   CV_64F);
        sum_MI2_buf.convertTo(sum_MI2_d, CV_64F);
        cv::multiply(sum_MI_d, sum_MI_d, var_d, -1.0 / mask_area);
        cv::add(var_d, sum_MI2_d, var_d);
        cv::add(sigma_I_sq_d, var_d, sigma_I_sq_d);
    }

    cv::Mat sigma_I_sq;
    sigma_I_sq_d.convertTo(sigma_I_sq, CV_32F);
    cv::max(sigma_I_sq, 0.0, sigma_I_sq);

    cv::Mat denom;
    cv::sqrt(sigma_I_sq * sigma_T_sq, denom);

    cv::Mat result;
    cv::divide(numerator, denom, result);
    cv::patchNaNs(result, 0.0);
    const float sigma_T_norm = static_cast<float>(std::sqrt(sigma_T_sq));
    result.setTo(0.0f, denom < sigma_T_norm * 1e-5f);
    cv::min(result,  1.0f, result);
    cv::max(result, -1.0f, result);
    return result;
}

std::vector<Matcher::RawResult> Matcher::preproc_and_match(const cv::Mat& image, const MatcherConfig::Params& params)
{
    std::vector<Matcher::RawResult> results;

    // Image-side color conversions: compute once, reuse across all templates
    cv::Mat image_match;
    cv::cvtColor(image, image_match, cv::COLOR_BGR2RGB);

    cv::Mat image_gray;
    if (!params.mask_ranges.empty() || !params.color_scales.empty()) {
        cv::cvtColor(image, image_gray, cv::COLOR_BGR2GRAY);
    }

    cv::Mat image_hsv;

    for (size_t i = 0; i != params.templs.size(); ++i) {
        const auto& ptempl = params.templs[i];
        auto method = MatchMethod::Ccoeff;
        if (params.methods.size() <= i) {
            Log.warn("methods is empty, use default method: Ccoeff");
        }
        else {
            method = params.methods[i];
        }

        if (method == MatchMethod::Invalid) {
            Log.error(__FUNCTION__, "| invalid method");
            return {};
        }

        cv::Mat templ;
        std::string templ_name;

        if (std::holds_alternative<std::string>(ptempl)) {
            templ_name = std::get<std::string>(ptempl);
            if (templ_name == "empty.png") {
                LogError << __FUNCTION__ << "| template is empty.png";
            }
            templ = TemplResource::get_instance().get_templ(templ_name);
        }
        else if (std::holds_alternative<cv::Mat>(ptempl)) {
            templ = std::get<cv::Mat>(ptempl);
        }
        else {
            Log.error("templ is none");
        }

        if (templ.empty()) {
            Log.error("templ is empty!", templ_name);
#ifdef ASST_DEBUG
            throw std::runtime_error("templ is empty: " + templ_name);
#else
            return {};
#endif
        }

        if (templ.cols > image.cols || templ.rows > image.rows) {
            Log.error(
                "templ size is too large",
                templ_name,
                "image size:",
                image.cols,
                image.rows,
                "templ size:",
                templ.cols,
                templ.rows);
            return {};
        }

        cv::Mat matched;
        cv::Mat templ_match, templ_count, templ_gray;
        cv::cvtColor(templ, templ_match, cv::COLOR_BGR2RGB);
        if (!image_gray.empty()) {
            cv::cvtColor(templ, templ_gray, cv::COLOR_BGR2GRAY);
        }

        cv::Mat image_count;
        if (method == MatchMethod::HSVCount) {
            if (image_hsv.empty()) {
                cv::cvtColor(image, image_hsv, cv::COLOR_BGR2HSV);
            }
            image_count = image_hsv;
            cv::cvtColor(templ, templ_count, cv::COLOR_BGR2HSV);
        }
        else if (method == MatchMethod::RGBCount) {
            image_count = image_match;
            templ_count = templ_match;
        }

        // 目前所有的匹配都是用 TM_CCOEFF_NORMED
        int match_algorithm = cv::TM_CCOEFF_NORMED;

        auto calc_mask = [&templ_name](
                             const MatchTaskInfo::Ranges mask_ranges,
                             const cv::Mat& templ,
                             const cv::Mat& templ_gray,
                             bool with_close) -> std::optional<cv::Mat> {
            // Union all masks, not intersection
            cv::Mat mask = cv::Mat::zeros(templ_gray.size(), CV_8UC1);
            for (const auto& range : mask_ranges) {
                cv::Mat current_mask;
                if (std::holds_alternative<MatchTaskInfo::GrayRange>(range)) {
                    const auto& gray_range = std::get<MatchTaskInfo::GrayRange>(range);
                    cv::inRange(templ_gray, gray_range.first, gray_range.second, current_mask);
                }
                else if (std::holds_alternative<MatchTaskInfo::ColorRange>(range)) {
                    const auto& color_range = std::get<MatchTaskInfo::ColorRange>(range);
                    cv::inRange(templ, color_range.first, color_range.second, current_mask);
                }
                else {
                    Log.error("The task with template", templ_name, "holds invalid mask range");
                    return std::nullopt;
                }
                cv::bitwise_or(mask, current_mask, mask);
            }

            if (with_close) {
                cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
                cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
            }
            return mask;
        };

        if (params.mask_ranges.empty()) {
            cv::matchTemplate(image_match, templ_match, matched, match_algorithm);
        }
        else {
            // match 时使用的 mask_range 当作 RGB 的
            auto mask_opt = calc_mask(
                params.mask_ranges,
                params.mask_src ? image_match : templ_match,
                params.mask_src ? image_gray : templ_gray,
                params.mask_close);
            if (!mask_opt) {
                return {};
            }
            // mask_src=false 时 mask 完全由模板决定，用 FFT 路径替代标量滑窗
            if (!params.mask_src) {
                //  cache key：templ_name + mask_ranges 序列化
                // （mask_src=false 时 mask 由模板+参数唯一确定）
                // cv::Mat 形式的模板没有文件名，用像素内容 hash 保证 key 唯一
                std::string fft_key = templ_name;
                if (fft_key.empty()) {
                    uint64_t h = 14695981039346656037ULL;
                    const auto* ptr = templ.data;
                    const size_t n = static_cast<size_t>(templ.total()) * templ.elemSize();
                    for (size_t j = 0; j < n; ++j) { h ^= ptr[j]; h *= 1099511628211ULL; }
                    fft_key = "mat:" + std::to_string(h);
                }
                for (const auto& r : params.mask_ranges) {
                    if (std::holds_alternative<MatchTaskInfo::GrayRange>(r)) {
                        const auto& g = std::get<MatchTaskInfo::GrayRange>(r);
                        fft_key += ":G" + std::to_string(g.first) + '_' + std::to_string(g.second);
                    }
                    else if (std::holds_alternative<MatchTaskInfo::ColorRange>(r)) {
                        const auto& col = std::get<MatchTaskInfo::ColorRange>(r);
                        fft_key += ":C";
                        for (auto v : col.first)  fft_key += std::to_string(v) + ',';
                        fft_key += '_';
                        for (auto v : col.second) fft_key += std::to_string(v) + ',';
                    }
                }
                fft_key += params.mask_close ? ":1" : ":0";

                matched = fast_masked_ccoeff_normed(
                    image_match, templ_match, mask_opt.value(), fft_key);
            }
            if (matched.empty()) {
                cv::matchTemplate(image_match, templ_match, matched, match_algorithm, mask_opt.value());
            }
        }

        if (method == MatchMethod::RGBCount || method == MatchMethod::HSVCount) {
            auto templ_active_opt = calc_mask(params.color_scales, templ_count, templ_gray, params.color_close);
            auto image_active_opt = calc_mask(params.color_scales, image_count, image_gray, params.color_close);
            if (!image_active_opt || !templ_active_opt) [[unlikely]] {
                return {};
            }
            cv::Mat templ_active = std::move(templ_active_opt).value();
            cv::Mat image_active = std::move(image_active_opt).value();

            cv::threshold(templ_active, templ_active, 1, 1, cv::THRESH_BINARY);
            cv::threshold(image_active, image_active, 1, 1, cv::THRESH_BINARY);
            // tp = image_active 与 templ_active 的共激活像素数（TM_CCORR 当 count 用）
            cv::Mat tp;
            int tp_fn = cv::countNonZero(templ_active);
            cv::matchTemplate(image_active, templ_active, tp, cv::TM_CCORR);
            tp.convertTo(tp, CV_32S);
            // sum_active = 每个窗口内 image_active 的总激活数
            // 由于 tp + fp = sum_active，用积分图代替第二次 matchTemplate
            cv::Mat image_active_f;
            image_active.convertTo(image_active_f, CV_32F);
            cv::Mat integ;
            cv::integral(image_active_f, integ, CV_32F);
            const int kh = templ_active.rows, kw = templ_active.cols;
            // 四角公式：sum_active[y,x] = integ[y+kh,x+kw] - integ[y,x+kw] - integ[y+kh,x] + integ[y,x]
            cv::Mat sum_active =
                integ(cv::Rect(kw, kh, tp.cols, tp.rows))
                - integ(cv::Rect(0,  kh, tp.cols, tp.rows))
                - integ(cv::Rect(kw, 0,  tp.cols, tp.rows))
                + integ(cv::Rect(0,  0,  tp.cols, tp.rows));
            cv::Mat sum_active_i;
            sum_active.convertTo(sum_active_i, CV_32S);
            cv::Mat count_result;
            cv::divide(2 * tp, sum_active_i + tp_fn, count_result, 1, CV_32F); // 数色结果为 f1_score

            if (params.pure_color) {
                matched = 1.0f;
            }

            cv::multiply(matched, count_result, matched); // 最终结果是数色和模板匹配的点积
        }
        results.emplace_back(RawResult { .matched = matched, .templ = templ, .templ_name = templ_name });
    }
    return results;
}
