#include "StrongholdTierMatcher.h"

#include "Config/StrongholdProtocol/StrongholdProtocolConfig.h"
#include "MaaUtils/NoWarningCV.hpp"

namespace asst::stronghold
{
static std::vector<cv::Range> stroke_runs(const cv::Mat& mask, int y)
{
    const auto* row = mask.ptr<uint8_t>(y);
    std::vector<cv::Range> runs;
    for (int x = 0; x < mask.cols;) {
        if (!row[x]) {
            ++x;
            continue;
        }
        const int start = x;
        while (x < mask.cols && row[x]) {
            ++x;
        }
        runs.emplace_back(start, x);
    }
    return runs;
}

static bool matches_glyph(
    std::string_view glyph,
    const std::vector<cv::Range>& upper,
    const std::vector<cv::Range>& lower,
    int badge_width,
    const StrongholdTierBadgeConfig& cfg)
{
    // I 在上下截面各占一段，V 的两条斜笔在下截面合拢。
    if (lower.size() != glyph.size() || upper.size() != glyph.size() + std::ranges::count(glyph, 'V')) {
        return false;
    }
    const double max_width = badge_width * cfg.max_stroke_width;
    auto next = upper.begin();
    for (size_t i = 0; i < glyph.size(); ++i) {
        const auto& top = *next++;
        const auto& bottom = lower[i];
        if (top.size() > max_width) {
            return false;
        }
        if (glyph[i] == 'I') {
            const double drift = badge_width * cfg.max_stroke_drift;
            if (bottom.size() > max_width || std::abs(top.start - bottom.start) > drift ||
                std::abs(top.end - bottom.end) > drift) {
                return false;
            }
        }
        else {
            const auto& right = *next++;
            const double convergence = badge_width * cfg.min_convergence;
            if (right.size() > max_width || bottom.start - top.start < convergence ||
                right.end - bottom.end < convergence) {
                return false;
            }
        }
    }
    return true;
}

int match_tier_badge(const cv::Mat& image, const Rect& badge)
{
    if (image.empty() || image.type() != CV_8UC3 || badge.width <= 0 || badge.height <= 0 || badge.x < 0 ||
        badge.y < 0 || badge.x > image.cols - badge.width || badge.y > image.rows - badge.height) {
        return 0;
    }
    const auto& cfg = StrongholdProtocol.get_tier_badges();
    const int dx = cvRound(badge.width * cfg.inset[0]);
    const int dy = cvRound(badge.height * cfg.inset[1]);
    const cv::Rect center(badge.x + dx, badge.y + dy, badge.width - 2 * dx, badge.height - 2 * dy);
    if (center.empty()) {
        return 0;
    }

    // HUD 会同时压暗字色和牌面，在每张方牌内单独分割文字。
    cv::Mat hsv, value, ink;
    cv::cvtColor(image(center), hsv, cv::COLOR_BGR2HSV);
    cv::extractChannel(hsv, value, 2);
    cv::threshold(value, ink, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    const int pixels = cv::countNonZero(ink);
    if (pixels == 0 || pixels == center.area() || pixels < center.area() * cfg.foreground_fraction[0] ||
        pixels > center.area() * cfg.foreground_fraction[1]) {
        return 0;
    }
    const double background = cv::mean(value, ~ink)[0];
    if (background > cfg.max_background_value || cv::mean(value, ink)[0] - background < cfg.min_contrast) {
        return 0;
    }
    const cv::Rect bounds = cv::boundingRect(ink);
    if (bounds.height < badge.height * cfg.glyph_height[0] || bounds.height > badge.height * cfg.glyph_height[1] ||
        std::abs(bounds.x + bounds.width * 0.5 - center.width * 0.5) > badge.width * cfg.max_center_offset ||
        std::abs(bounds.y + bounds.height * 0.5 - center.height * 0.5) > badge.height * cfg.max_center_offset) {
        return 0;
    }
    const auto strokes = [&](double fraction) {
        return stroke_runs(ink, bounds.y + cvRound(fraction * (bounds.height - 1)));
    };
    const auto upper = strokes(cfg.stroke_rows[0]);
    const auto lower = strokes(cfg.stroke_rows[1]);

    int match = 0;
    bool color_seen = false;
    cv::Mat colored;
    for (size_t i = 0; i < cfg.hsv.size(); ++i) {
        const auto& [lo, hi] = cfg.hsv[i];
        cv::inRange(hsv, cv::Scalar(lo[0], lo[1], lo[2]), cv::Scalar(hi[0], hi[1], hi[2]), colored);
        cv::bitwise_and(colored, ink, colored);
        if (cv::countNonZero(colored) < pixels * cfg.min_color_fraction) {
            continue;
        }
        const auto glyph = StrongholdTierGlyphs[i];
        // I 和 II 的亮度在阴影里可能重叠，仍能按竖笔数区分。彩色字命中多个色域时弃权。
        if (color_seen && glyph != "II") {
            return 0;
        }
        color_seen = true;
        if (matches_glyph(glyph, upper, lower, badge.width, cfg)) {
            match = static_cast<int>(i) + 1;
        }
    }
    return match;
}
}
