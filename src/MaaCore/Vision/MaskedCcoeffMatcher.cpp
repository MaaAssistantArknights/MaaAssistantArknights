#include "MaskedCcoeffMatcher.h"

#include "MaaUtils/NoWarningCV.hpp"

#include "Utils/Logger.hpp"

#include <array>
#include <vector>

namespace asst
{
namespace
{
// 稀疏路径使用的每个有效 mask 像素的条目
struct SparseEntry
{
    int16_t dx, dy;   // 相对模板左上角的偏移
    float T_prime[3]; // M*(T_c - μT_c)，per-channel
};
}

struct MaskedCcoeffMatcher::TemplatePlan
{
    cv::Mat M;                      // CV_32F mask, 0 or 1
    std::array<cv::Mat, 3> T_prime; // M*(T_c - μT_c)，per-channel
    double sigma_T_sq = 0.0;
    double mask_area = 0.0;
    std::vector<SparseEntry> sparse_entries; // 非零 mask 位置列表
    int K = 0;                               // sparse_entries.size()
};

MaskedCcoeffMatcher& MaskedCcoeffMatcher::get_instance()
{
    static MaskedCcoeffMatcher instance;
    return instance;
}

void MaskedCcoeffMatcher::sync_cache_revision(const uint64_t revision)
{
    if (m_cache_revision.load(std::memory_order_acquire) == revision) {
        return;
    }
    std::lock_guard lk(m_cache_mtx);
    if (m_cache_revision.load(std::memory_order_relaxed) == revision) {
        return;
    }
    // 清一下缓存
    m_template_plan_cache.clear();
    m_lru_list.clear();
    m_cache_total_bytes = 0;
    m_cache_revision.store(revision, std::memory_order_release);
}

void MaskedCcoeffMatcher::fnv1a_update(uint64_t& h, const void* data, size_t size)
{
    const auto* ptr = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        h ^= ptr[i];
        h *= 1'099'511'628'211ULL;
    }
}

std::string MaskedCcoeffMatcher::make_mat_cache_key(const cv::Mat& mat)
{
    uint64_t h = 14'695'981'039'346'656'037ULL;
    const int meta[] = { mat.rows, mat.cols, mat.type() };
    fnv1a_update(h, meta, sizeof(meta));

    const size_t row_bytes = static_cast<size_t>(mat.cols) * mat.elemSize();
    for (int y = 0; y < mat.rows; ++y) {
        fnv1a_update(h, mat.ptr(y), row_bytes);
    }
    // 捏个hash key
    return "mat:" + std::to_string(mat.rows) + "x" + std::to_string(mat.cols) + ":" + std::to_string(mat.type()) + ":" +
           std::to_string(h);
}

size_t MaskedCcoeffMatcher::calc_plan_bytes(const std::string& key, const TemplatePlan& plan)
{
    size_t b = key.size();
    b += plan.M.total() * plan.M.elemSize();
    for (const auto& t : plan.T_prime) {
        b += t.total() * t.elemSize();
    }
    b += plan.sparse_entries.size() * sizeof(SparseEntry);
    return b;
}

std::shared_ptr<const MaskedCcoeffMatcher::TemplatePlan> MaskedCcoeffMatcher::get_or_build_template_plan(
    const std::string& cache_key,
    const cv::Mat& templ_f32,
    const cv::Mat& mask_f32,
    int mask_pixels)
{
    // 记录 miss 时的 revision，插入前校验是否已被清缓存
    const uint64_t revision_at_miss = m_cache_revision.load(std::memory_order_acquire);
    {
        std::lock_guard lk(m_cache_mtx);
        if (auto it = m_template_plan_cache.find(cache_key); it != m_template_plan_cache.end()) {
            m_lru_list.splice(m_lru_list.begin(), m_lru_list, it->second.lru_it);
            return it->second.plan;
        }
    }

    auto plan = std::make_shared<TemplatePlan>();
    plan->M = mask_f32;
    plan->mask_area = mask_pixels;
    if (plan->mask_area < 1.0) {
        return {};
    }

    std::vector<cv::Mat> T_ch(3);
    cv::split(templ_f32, T_ch);

    for (int c = 0; c < 3; ++c) {
        const double mu_T = cv::sum(plan->M.mul(T_ch[c]))[0] / plan->mask_area;
        plan->T_prime[c] = plan->M.mul(T_ch[c] - mu_T);
        plan->sigma_T_sq += cv::sum(plan->T_prime[c].mul(plan->T_prime[c]))[0];
    }

    for (int v = 0; v < templ_f32.rows; ++v) {
        for (int u = 0; u < templ_f32.cols; ++u) {
            if (plan->M.at<float>(v, u) > 0.5f) {
                SparseEntry e {};
                e.dx = static_cast<int16_t>(u);
                e.dy = static_cast<int16_t>(v);
                for (int c = 0; c < 3; ++c) {
                    e.T_prime[c] = plan->T_prime[c].at<float>(v, u);
                }
                plan->sparse_entries.push_back(e);
            }
        }
    }
    plan->K = static_cast<int>(plan->sparse_entries.size());

    const size_t new_bytes = calc_plan_bytes(cache_key, *plan);

    std::lock_guard lk(m_cache_mtx);

    // 二次检查：另一线程可能已插入同一 key，虽然当前设计使用的是单线程 Runner
    if (auto it = m_template_plan_cache.find(cache_key); it != m_template_plan_cache.end()) {
        m_lru_list.splice(m_lru_list.begin(), m_lru_list, it->second.lru_it);
        return it->second.plan;
    }

    // revision 已变说明缓存在 build 期间被清过，旧数据不缓存
    if (m_cache_revision.load(std::memory_order_relaxed) != revision_at_miss) {
        return plan;
    }

    // 从尾部淘汰直到满足内存上限
    while (m_cache_total_bytes + new_bytes > k_max_cache_bytes && !m_lru_list.empty()) {
        const std::string& victim = m_lru_list.back();
        const size_t victim_bytes = m_template_plan_cache.at(victim).bytes;
        Log.debug(
            "MaskedCcoeffMatcher | evict",
            victim,
            victim_bytes / 1024,
            "KB, total",
            m_cache_total_bytes / 1024,
            "KB");
        m_cache_total_bytes -= victim_bytes;
        m_template_plan_cache.erase(victim);
        m_lru_list.pop_back();
    }

    auto list_it = m_lru_list.insert(m_lru_list.begin(), cache_key);
    m_template_plan_cache.emplace(cache_key, CacheEntry { plan, list_it, new_bytes });
    m_cache_total_bytes += new_bytes;
    return plan;
}

bool MaskedCcoeffMatcher::should_fallback_to_opencv(int mask_pixels, int result_positions)
{
    // 神秘调参值
    // - 极小 result + 低 K：稀疏路径整体工作量极小，留给 FFT/sparse
    // - 极小 result + 高 K（如 138×130/105×105）：K 超稀疏阈值，FFT 在小 DFT size 反而不如 OpenCV
    // - 中等 result 配中等 K：Windows 上 OpenCV 紧凑 SIMD 快；Android 上 OpenCV 慢约 300x，阈值大幅收紧

    if (result_positions < 1000 && mask_pixels < 2000) {
        return false;
    }

#ifdef __ANDROID__
    if (result_positions < 3000 && mask_pixels >= 500) {
        return true;
    }
    if (static_cast<long long>(mask_pixels) * result_positions < 8'000'000LL) {
        return true;
    }
#else
    if (result_positions < 12000 && mask_pixels >= 500) {
        return true;
    }
    if (static_cast<long long>(mask_pixels) * result_positions < 25'000'000LL) {
        return true;
    }
#endif

    return false;
}

// 用 cv::dft 直接实现，消除冗余 FFT
//
// 当前 9 次 matchTemplate 的冗余：
//   FFT(I_c)  每通道算两次（分别用于 xcorr(T'_c, I_c) 和 xcorr(M, I_c)）
//   FFT(M)    每通道算两次（分别用于 xcorr(M, I_c) 和 xcorr(M, I_c²)）
//
// 优化后：
//   FFT(I_c) 和 FFT(I_c²) 每通道各算一次并复用
//   TemplatePlan（T'_c、稀疏列表）通过缓存跨调用复用
//
// 等价于 cv::matchTemplate(image, templ, result, TM_CCOEFF_NORMED, mask)
cv::Mat MaskedCcoeffMatcher::match(
    const cv::Mat& image_rgb,     // CV_8UC3
    const cv::Mat& templ_rgb,     // CV_8UC3
    const cv::Mat& mask_u8,       // CV_8UC1, 0 or 255
    const std::string& cache_key, // 模板侧 FFT 缓存键
    int mask_pixels)
{
    const int rh = image_rgb.rows - templ_rgb.rows + 1;
    const int rw = image_rgb.cols - templ_rgb.cols + 1;
    if (rh <= 0 || rw <= 0) {
        return {};
    }

    if (mask_pixels <= 0 || should_fallback_to_opencv(mask_pixels, rh * rw)) {
        return {};
    }

    cv::Mat I, T, M;
    image_rgb.convertTo(I, CV_32F);
    templ_rgb.convertTo(T, CV_32F);
    mask_u8.convertTo(M, CV_32F, 1.0 / 255.0);

    const auto template_plan = get_or_build_template_plan(cache_key, T, M, mask_pixels);
    if (!template_plan) {
        return {};
    }

    const double mask_area = template_plan->mask_area;
    const double sigma_T_sq = template_plan->sigma_T_sq;

    // 图像通道拆分：稀疏和 FFT 两条路径都需要
    std::vector<cv::Mat> I_ch(3);
    cv::split(I, I_ch);

    // 稀疏直接相关（小模板快路径，比如基建任务中那种就很合适）
    // 双重条件：K < SPARSE_K_LIMIT 且总工作量 K×result_positions < SPARSE_WORK_LIMIT
    // 仅满足 K 小但结果矩阵极大时（如 49×28 模板/690×434 图）仍走 FFT 路径
    static constexpr int SPARSE_K_LIMIT = 2000;
    static constexpr long long SPARSE_WORK_LIMIT = 30'000'000LL;
    if (template_plan->K > 0 && template_plan->K < SPARSE_K_LIMIT &&
        static_cast<long long>(template_plan->K) * rh * rw < SPARSE_WORK_LIMIT) {
        cv::Mat numerator = cv::Mat::zeros(rh, rw, CV_32F);
        // 用 CV_64F 避免大数相减时的 float32 catastrophic cancellation：
        cv::Mat sum_MI_r = cv::Mat::zeros(rh, rw, CV_64F);
        cv::Mat sum_MI_g = cv::Mat::zeros(rh, rw, CV_64F);
        cv::Mat sum_MI_b = cv::Mat::zeros(rh, rw, CV_64F);
        cv::Mat sum_MI2 = cv::Mat::zeros(rh, rw, CV_64F); // Σ_c I_c²

        for (const auto& [dx, dy, T_prime] : template_plan->sparse_entries) {
            for (int y = 0; y < rh; ++y) {
                const float* Ir = I_ch[0].ptr<float>(y + dy) + dx;
                const float* Ig = I_ch[1].ptr<float>(y + dy) + dx;
                const float* Ib = I_ch[2].ptr<float>(y + dy) + dx;
                auto* num_p = numerator.ptr<float>(y);
                auto* smir_p = sum_MI_r.ptr<double>(y);
                auto* smig_p = sum_MI_g.ptr<double>(y);
                auto* smib_p = sum_MI_b.ptr<double>(y);
                auto* smi2_p = sum_MI2.ptr<double>(y);

                // 编译器会自动向量化的
                for (int x = 0; x < rw; ++x) {
                    const float r = Ir[x], g = Ig[x], b = Ib[x];
                    num_p[x] += T_prime[0] * r + T_prime[1] * g + T_prime[2] * b;
                    smir_p[x] += r;
                    smig_p[x] += g;
                    smib_p[x] += b;
                    smi2_p[x] += r * r + g * g + b * b;
                }
            }
        }

        // sigma_I² = sum_MI2 - (sum_MI_r² + sum_MI_g² + sum_MI_b²) / mask_area
        // 全程保持 CV_64F，防止大数相减精度损失
        cv::Mat sq_sum, sq_g, sq_b;
        cv::multiply(sum_MI_r, sum_MI_r, sq_sum);
        cv::multiply(sum_MI_g, sum_MI_g, sq_g);
        cv::multiply(sum_MI_b, sum_MI_b, sq_b);
        cv::add(sq_sum, sq_g, sq_sum);
        cv::add(sq_sum, sq_b, sq_sum);
        cv::Mat sigma_I_sq_64;
        cv::subtract(sum_MI2, sq_sum * (1.0 / mask_area), sigma_I_sq_64);
        cv::max(sigma_I_sq_64, 0.0, sigma_I_sq_64);
        cv::Mat sigma_I_sq;
        sigma_I_sq_64.convertTo(sigma_I_sq, CV_32F);

        cv::Mat denom;
        cv::sqrt(sigma_I_sq * sigma_T_sq, denom);
        cv::Mat result;
        cv::divide(numerator, denom, result);
        cv::patchNaNs(result, 0.0);
        const auto sigma_T_norm = static_cast<float>(std::sqrt(sigma_T_sq));
        result.setTo(0.0f, denom < sigma_T_norm * 1e-5f);
        cv::min(result, 1.0f, result);
        cv::max(result, -1.0f, result);
        return result;
    }

    // DFT 的填充尺寸：仅在确认走 FFT 路径后才需要
    const int dft_rows = cv::getOptimalDFTSize(I.rows + T.rows - 1);
    const int dft_cols = cv::getOptimalDFTSize(I.cols + T.cols - 1);

    cv::Mat padded(dft_rows, dft_cols, CV_64F, cv::Scalar(0));
    cv::Mat I_dft(dft_rows, dft_cols, CV_64FC2);
    cv::Mat spectrum(dft_rows, dft_cols, CV_64FC2);
    cv::Mat result_buf(dft_rows, dft_cols, CV_64F);
    cv::Mat sum_MI_buf(rh, rw, CV_64F);
    cv::Mat sum_MI2_buf(rh, rw, CV_64F);
    cv::Mat src_d;

    auto make_dft_into = [&](const cv::Mat& src, cv::Mat& out) {
        padded.setTo(0.0);
        src.convertTo(src_d, CV_64F);
        src_d.copyTo(padded(cv::Rect(0, 0, src_d.cols, src_d.rows)));
        cv::dft(padded, out, cv::DFT_COMPLEX_OUTPUT);
    };

    std::array<cv::Mat, 3> T_prime_dft;
    cv::Mat M_dft;
    make_dft_into(template_plan->M, M_dft);
    for (int c = 0; c < 3; ++c) {
        make_dft_into(template_plan->T_prime[c], T_prime_dft[c]);
    }
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

    cv::Mat numerator = cv::Mat::zeros(rh, rw, CV_64F);
    cv::Mat sigma_I_sq = cv::Mat::zeros(rh, rw, CV_64F);

    // σ_I²(x,y) = Σ_c σ_I_c² = Σ_c [(M ⋆ I_c²) - (M ⋆ I_c)² / N]
    //          = Σ_c (M ⋆ I_c²)        - (1/N) Σ_c (M ⋆ I_c)²
    //          ↑ 把这一项的三通道求和提到卷积外面
    //
    // 利用卷积对加法线性：Σ_c (M ⋆ I_c²) = M ⋆ (Σ_c I_c²)
    // 在空域里先把三通道平方加起来再做一次卷积，比每通道各做一次再相加少 2 次 FFT + 2 次 IFFT
    // 第二项 Σ_c (M ⋆ I_c)² 因为有平方，不能这样合并（平方对加法非线性），仍逐通道算
    cv::Mat I_sq_sum = I_ch[0].mul(I_ch[0]) + I_ch[1].mul(I_ch[1]) + I_ch[2].mul(I_ch[2]);
    cv::Mat I_sq_sum_dft(dft_rows, dft_cols, CV_64FC2);
    make_dft_into(I_sq_sum, I_sq_sum_dft);
    xcorr_into(I_sq_sum_dft, M_dft, sum_MI2_buf);
    cv::add(sigma_I_sq, sum_MI2_buf, sigma_I_sq);

    for (int c = 0; c < 3; ++c) {
        make_dft_into(I_ch[c], I_dft);

        // numerator += xcorr(T'_c, I_c)
        xcorr_add(I_dft, T_prime_dft[c], numerator);

        // sigma_I² 第二项：-Σ_c (sum_MI_c)² / mask_area，逐通道累加
        xcorr_into(I_dft, M_dft, sum_MI_buf);
        cv::Mat var_d;
        cv::multiply(sum_MI_buf, sum_MI_buf, var_d, -1.0 / mask_area);
        cv::add(sigma_I_sq, var_d, sigma_I_sq);
    }

    cv::max(sigma_I_sq, 0.0, sigma_I_sq);
    // 图像块方差远低于模板方差时相关系数无定义（均匀区域，分母≈0），直接归零。
    // 8-bit 图像最小非零单通道方差约为 (K-1)/K ≈ 0.996，variance_eps 远低于此，不影响真实匹配。
    const double variance_eps = sigma_T_sq * 1e-8;
    sigma_I_sq.setTo(0.0, sigma_I_sq < variance_eps);

    cv::Mat denom;
    cv::sqrt(sigma_I_sq * sigma_T_sq, denom);

    cv::Mat result;
    cv::divide(numerator, denom, result);
    // patchNaNs 不支持 CV_64F，用 CMP_NE 自检替代（NaN != NaN）
    cv::Mat nan_mask;
    cv::compare(result, result, nan_mask, cv::CMP_NE);
    result.setTo(0.0, nan_mask);
    const double sigma_T_norm = std::sqrt(sigma_T_sq);
    result.setTo(0.0, denom < sigma_T_norm * 1e-5);
    cv::min(result, 1.0, result);
    cv::max(result, -1.0, result);

    cv::Mat result_f32;
    result.convertTo(result_f32, CV_32F);
    return result_f32;
}
}
