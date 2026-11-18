#include "SPAFarmTaskPlugin.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <numeric>
#include <queue>
#include <random>
#include <regex>
#include <sstream>
#include <thread>

#include "Config/GeneralConfig.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Config/Miscellaneous/OcrPack.h"
#include "Config/ResourceLoader.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Vision/MaskedCcoeffMatcher.h"

namespace asst
{

namespace
{

constexpr double kLeaveTimeoutSec = 30.0;
constexpr double kStallTimeoutSec = 600.0;

// `ht` in service.pretty.mjs L7855-7868 —— 严格 12 个，不得增补。
// `en()` 在 goal=="medal" 时直接取 `ua.stackers`，多出来的名字会改变选人与刷新判断。

// `er` in service.pretty.mjs L7869-7870 —— 严格 4 个
const std::vector<std::string> kQuickFinishers = {
    "蕾缪安", "塔露拉", "艾雅法拉", "维伊"
};

const std::vector<std::string> kAuxiliaryNames = {
    "幻影", "Touch", "预备干员医疗", "机动盾牌", "工程蓄水炮", "樱桃三号", "结构性原理"
};

const std::vector<std::pair<std::string, std::string>> kPriorityFull = {
    { "塔露拉", "塔露拉" },     { "维什戴尔", "维什戴尔" }, { "星熊", "斩业星熊" },
    { "安洁莉娜", "予愿安洁莉娜" }, { "能天使", "新约能天使" }, { "凯尔希", "凯尔希·思衡托" },
    { "望", "望" },             { "初雪", "圣聆初雪" },     { "逻各斯", "逻各斯" },
    { "蕾缪安", "蕾缪安" },     { "贝洛内", "贝洛内" },     { "冻河", "冻河" },
    { "明霄陈", "赤刃明霄陈" }, { "结城理", "结城理" }
};

const std::vector<std::pair<std::string, std::string>> kPriorityLow = {
    { "塔露拉", "塔露拉" },     { "史尔特尔", "史尔特尔" }, { "耀骑士", "耀骑士临光" },
    { "逻各斯", "逻各斯" },     { "克莱门莎", "克莱门莎" }, { "锏", "锏" },
    { "帕拉斯", "帕拉斯" },     { "真言", "真言" },         { "蕾缪安", "蕾缪安" },
    { "烛煌", "烛煌" },         { "艾丽妮", "艾丽妮" },     { "薇薇安娜", "薇薇安娜" },
    { "维娜", "维娜·维多利亚" }, { "乌尔比安", "乌尔比安" }, { "棘刺", "棘刺" },
    { "凛冬", "怒潮凛冬" },     { "初雪", "圣聆初雪" },     { "迷迭香", "迷迭香" },
    { "鸿雪", "鸿雪" },         { "贝洛内", "贝洛内" },     { "佩佩", "佩佩" },
    { "隐德来希", "隐德来希" }, { "冻河", "冻河" },         { "酒神", "酒神" }
};

// Exact 1:1 port of `pr` in service.pretty.mjs lines 7883-7912
struct GoalTaskSpec
{
    std::string id;
    std::vector<std::string> bonds;
    int layers = 100;
    std::vector<std::string> strategies;
};

const std::vector<GoalTaskSpec> kGoalTasks = {
    { "task-1", { "炎", "莱塔尼亚", "奥术" }, 100, { "杰西卡" } },
    { "task-2", { "阿戈尔", "卡西米尔", "突袭" }, 100, { "杰西卡" } },
    { "task-3", { "叙拉古", "萨尔贡", "精准" }, 100, { "杰西卡" } },
    { "task-4", { "维多利亚", "谢拉格", "灵巧" }, 100, { "杰西卡" } },
    { "task-5", { "乌萨斯", "伊比利亚", "聚合" }, 100, { "杰西卡" } },
    { "task-6", { "奇迹", "远见", "投资人" }, 50, { "杰西卡" } },
    { "task-7", { "迅捷", "不屈", "坚守", "助力" }, 50, { "杰西卡" } }
};

double now_sec()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

std::vector<std::string> split_utf8_chars(std::string_view s)
{
    std::vector<std::string> out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        size_t len = 1;
        if ((c & 0x80) == 0) {
            len = 1;
        }
        else if ((c & 0xE0) == 0xC0) {
            len = 2;
        }
        else if ((c & 0xF0) == 0xE0) {
            len = 3;
        }
        else if ((c & 0xF8) == 0xF0) {
            len = 4;
        }
        if (i + len > s.size()) {
            len = s.size() - i;
        }
        out.emplace_back(s.substr(i, len));
        i += len;
    }
    return out;
}

bool is_cjk_or_alnum_cp(std::string_view ch)
{
    if (ch.size() == 1) {
        unsigned char c = static_cast<unsigned char>(ch[0]);
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    }
    if (ch.size() == 3) {
        unsigned char b0 = static_cast<unsigned char>(ch[0]);
        unsigned char b1 = static_cast<unsigned char>(ch[1]);
        unsigned char b2 = static_cast<unsigned char>(ch[2]);
        uint32_t cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
        return cp >= 0x4E00 && cp <= 0x9FFF;
    }
    return false;
}

std::string normalize_variants(std::string_view s)
{
    static const std::vector<std::pair<std::string_view, std::string_view>> kVariants = {
        { "衛", "卫" }, { "戍", "戍" }, { "協", "协" }, { "議", "议" }, { "極", "极" },
        { "擬", "拟" }, { "獨", "独" }, { "選", "选" }, { "擇", "择" }, { "略", "略" },
        { "確", "确" }, { "認", "认" }, { "準", "准" }, { "備", "备" }, { "編", "编" },
        { "隊", "队" }, { "級", "级" }, { "滿", "满" }, { "購", "购" }, { "買", "买" },
        { "賣", "卖" }, { "資", "资" }, { "金", "金" }, { "刷", "刷" }, { "新", "新" },
        { "凍", "冻" }, { "結", "结" }, { "返", "返" }, { "頁", "页" }, { "斷", "断" },
        { "線", "线" }, { "連", "连" }, { "繼", "继" }, { "續", "续" }, { "離", "离" },
        { "開", "开" }, { "關", "关" }, { "閉", "闭" }, { "決", "决" }, { "策", "策" },
        { "戰", "战" }, { "術", "术" }, { "獎", "奖" }, { "勵", "励" }, { "獲", "获" },
        { "裝", "装" }, { "穿", "穿" }, { "戴", "戴" }, { "領", "领" }, { "袖", "袖" },
        { "敵", "敌" }, { "訓", "训" }, { "維", "维" }, { "爾", "尔" }, { "邏", "逻" },
        { "絲", "丝" }, { "藍", "蓝" }, { "遠", "远" }, { "煙", "烟" }, { "閃", "闪" },
        { "潔", "洁" }, { "麗", "丽" }, { "凱", "凯" }, { "聖", "圣" }, { "聆", "聆" },
        { "蕾", "蕾" }, { "繆", "缪" }, { "貝", "贝" }, { "陳", "陈" }, { "鋒", "锋" },
        { "術", "术" }, { "醫", "医" }, { "療", "疗" }, { "護", "护" }, { "堅", "坚" },
        { "強", "强" }, { "精", "精" }, { "奧", "奥" }, { "靈", "灵" }, { "跡", "迹" },
        { "辭", "辞" }, { "歲", "岁" }, { "榮", "荣" }, { "薩", "萨" }, { "貢", "贡" },
        { "謝", "谢" }, { "格", "格" }, { "蘭", "兰" }
    };
    std::string out(s);
    for (const auto& [from, to] : kVariants) {
        size_t pos = 0;
        while ((pos = out.find(from, pos)) != std::string::npos) {
            out.replace(pos, from.size(), to);
            pos += to.size();
        }
    }
    return out;
}

std::string extract_digits_mapped(std::string_view s)
{
    std::string out;
    auto chars = split_utf8_chars(s);
    for (const auto& ch : chars) {
        if (ch.size() == 1) {
            char c = ch[0];
            if (c >= '0' && c <= '9') {
                out.push_back(c);
            }
            else if (c == 'O' || c == 'o') {
                out.push_back('0');
            }
        }
        else if (ch == "Ｏ" || ch == "〇") {
            out.push_back('0');
        }
    }
    return out;
}

// ===========================================================================
// 图像原语 —— 严格对齐 service.pretty.mjs 里 sharp 的语义
//   qe(x).extract({left,top,width,height})          -> img_crop
//   .resize(w,h,{kernel:"cubic"})                   -> img_resize_cubic
//   .resize({height, kernel:"cubic"})               -> img_resize_h_cubic
//   .extend({top,bottom,left,right,bg:"#000"})      -> img_extend_black
//   .grayscale() / .greyscale()                     -> img_gray
//   .clahe({width,height,maxSlope})                 -> img_clahe
//   .extractChannel("red").linear(a,b).toColourspace("srgb") -> img_red_linear
//   .removeAlpha().raw().toBuffer()                 -> img_to_rgb（RGB 通道序，与 JS 一致）
// 说明：参考实现所有坐标基准都是 1920x1080，本层只负责像素，不做任何坐标换算。
// ===========================================================================

struct RawRgb
{
    std::vector<uint8_t> data;
    int width = 0;
    int height = 0;
    int channels = 3;
};

cv::Mat img_crop(const cv::Mat& img, int left, int top, int width, int height)
{
    if (img.empty() || width <= 0 || height <= 0) {
        return {};
    }
    const int x = std::clamp(left, 0, std::max(0, img.cols - 1));
    const int y = std::clamp(top, 0, std::max(0, img.rows - 1));
    const int w = std::min(width, img.cols - x);
    const int h = std::min(height, img.rows - y);
    if (w <= 0 || h <= 0) {
        return {};
    }
    return img(cv::Rect(x, y, w, h)).clone();
}

cv::Mat img_resize_cubic(const cv::Mat& img, int width, int height)
{
    if (img.empty() || width <= 0 || height <= 0) {
        return {};
    }
    if (img.cols == width && img.rows == height) {
        return img.clone();
    }
    cv::Mat out;
    cv::resize(img, out, cv::Size(width, height), 0.0, 0.0, cv::INTER_CUBIC);
    return out;
}

cv::Mat img_resize_h_cubic(const cv::Mat& img, int height)
{
    if (img.empty() || height <= 0) {
        return {};
    }
    const double scale = static_cast<double>(height) / static_cast<double>(img.rows);
    const int width = std::max(1, static_cast<int>(std::lround(static_cast<double>(img.cols) * scale)));
    return img_resize_cubic(img, width, height);
}

cv::Mat img_extend_black(const cv::Mat& img, int top, int bottom, int left, int right)
{
    if (img.empty()) {
        return {};
    }
    cv::Mat out;
    cv::copyMakeBorder(img, out, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    return out;
}

cv::Mat img_gray(const cv::Mat& img)
{
    if (img.empty()) {
        return {};
    }
    if (img.channels() == 1) {
        return img.clone();
    }
    cv::Mat out;
    cv::cvtColor(img, out, img.channels() == 4 ? cv::COLOR_BGRA2GRAY : cv::COLOR_BGR2GRAY);
    return out;
}

cv::Mat img_gray_to_bgr(const cv::Mat& gray)
{
    if (gray.empty()) {
        return {};
    }
    if (gray.channels() == 3) {
        return gray.clone();
    }
    cv::Mat out;
    cv::cvtColor(gray, out, cv::COLOR_GRAY2BGR);
    return out;
}

cv::Mat img_clahe(const cv::Mat& gray, int tile_w, int tile_h, double max_slope)
{
    if (gray.empty()) {
        return {};
    }
    cv::Mat g = img_gray(gray);
    // sharp 的 clahe({width,height}) 是**瓦片像素尺寸**；OpenCV 的 tileGridSize 是**瓦片个数**
    const int grid_x = std::max(1, g.cols / std::max(1, tile_w));
    const int grid_y = std::max(1, g.rows / std::max(1, tile_h));
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(max_slope, cv::Size(grid_x, grid_y));
    cv::Mat out;
    clahe->apply(g, out);
    return img_gray_to_bgr(out);
}

// sharp 的 extractChannel("red").linear(a, b)：单通道 out = clamp(a*R + b)
cv::Mat img_red_linear(const cv::Mat& img, double a, double b)
{
    if (img.empty()) {
        return {};
    }
    cv::Mat bgr;
    if (img.channels() == 4) {
        cv::cvtColor(img, bgr, cv::COLOR_BGRA2BGR);
    }
    else if (img.channels() == 1) {
        cv::cvtColor(img, bgr, cv::COLOR_GRAY2BGR);
    }
    else {
        bgr = img;
    }
    cv::Mat out(bgr.rows, bgr.cols, CV_8UC3);
    for (int y = 0; y < bgr.rows; ++y) {
        const auto* src = bgr.ptr<cv::Vec3b>(y);
        auto* dst = out.ptr<cv::Vec3b>(y);
        for (int x = 0; x < bgr.cols; ++x) {
            const int r = src[x][2];
            const int v = std::clamp(static_cast<int>(std::lround(a * r + b)), 0, 255);
            dst[x] = cv::Vec3b(static_cast<uint8_t>(v), static_cast<uint8_t>(v), static_cast<uint8_t>(v));
        }
    }
    return out;
}

// .removeAlpha().raw()：输出 RGB 序（JS 侧 R,G,B 就是 data[i], data[i+1], data[i+2]）
RawRgb img_to_rgb(const cv::Mat& img)
{
    RawRgb out;
    if (img.empty()) {
        return out;
    }
    cv::Mat bgr;
    if (img.channels() == 4) {
        cv::cvtColor(img, bgr, cv::COLOR_BGRA2BGR);
    }
    else if (img.channels() == 1) {
        cv::cvtColor(img, bgr, cv::COLOR_GRAY2BGR);
    }
    else {
        bgr = img;
    }
    out.width = bgr.cols;
    out.height = bgr.rows;
    out.channels = 3;
    out.data.resize(static_cast<size_t>(bgr.cols) * bgr.rows * 3);
    for (int y = 0; y < bgr.rows; ++y) {
        const auto* row = bgr.ptr<cv::Vec3b>(y);
        for (int x = 0; x < bgr.cols; ++x) {
            const size_t o = (static_cast<size_t>(y) * bgr.cols + x) * 3;
            out.data[o + 0] = row[x][2];
            out.data[o + 1] = row[x][1];
            out.data[o + 2] = row[x][0];
        }
    }
    return out;
}

// ===========================================================================
// 文本原语 —— 严格对齐 service.pretty.mjs
//   rt(a)      L5055  去掉空白与标点，其余字符全部保留（不做繁简转换）
//   Jr(a)      L5063  去重 + 按长度降序
//   Fi(a,s)    L5074  字符袋交集 / max(len)
//   vi(a,s)    L5080  整串模糊匹配（长度差<=1，>=0.66，并列视为歧义）
//   xr(a,s)    L5087  按 token 贪心前缀匹配 + 兜底 vi
//   yi(a,s)    L5139  覆盖率
// ===========================================================================

bool is_rt_punct(const std::string& ch)
{
    if (ch.size() == 1) {
        const unsigned char c = static_cast<unsigned char>(ch[0]);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') {
            return true;
        }
    }
    static const std::unordered_set<std::string> kPunct = {
        "·", "・", ".", "。", ",", "，", ":", "：", "'", "\"", "“", "”", "‘", "’",
        "(", ")", "（", "）", "【", "】", "[", "]", "<", ">", "《", "》", "|", "\\",
        "/", "_", "-", "—", "~", "～", "!", "！", "?", "？", "\u3000"
    };
    return kPunct.count(ch) > 0;
}

// `rt`：只删标点/空白，其余原样保留
std::string strip_punct(std::string_view s)
{
    std::string out;
    for (const auto& ch : split_utf8_chars(s)) {
        if (!is_rt_punct(ch)) {
            out += ch;
        }
    }
    return out;
}

// `.replace(/\s/g, "")`：只删空白
std::string strip_ws(std::string_view s)
{
    std::string out;
    for (const auto& ch : split_utf8_chars(s)) {
        if (ch.size() == 1) {
            const unsigned char c = static_cast<unsigned char>(ch[0]);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') {
                continue;
            }
        }
        if (ch == "\u3000") {
            continue;
        }
        out += ch;
    }
    return out;
}

int utf8_len(std::string_view s)
{
    int n = 0;
    for (size_t i = 0; i < s.size();) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        size_t len = 1;
        if ((c & 0x80) == 0) {
            len = 1;
        }
        else if ((c & 0xE0) == 0xC0) {
            len = 2;
        }
        else if ((c & 0xF0) == 0xE0) {
            len = 3;
        }
        else if ((c & 0xF8) == 0xF0) {
            len = 4;
        }
        i += std::min(len, s.size() - i);
        ++n;
    }
    return n;
}

bool regex_test(const std::string& text, const std::string& pattern)
{
    try {
        const std::regex re(pattern, std::regex::ECMAScript);
        return std::regex_search(text, re);
    }
    catch (const std::regex_error&) {
        Log.error("SPAFarm | 非法正则", pattern);
        return false;
    }
}

// 返回第一个整体匹配；对应 JS 的 `s.match(/re/)?.[0]`
std::optional<std::string> regex_first(const std::string& text, const std::string& pattern)
{
    try {
        const std::regex re(pattern, std::regex::ECMAScript);
        std::smatch m;
        if (std::regex_search(text, m, re) && !m.empty()) {
            return m[0].str();
        }
    }
    catch (const std::regex_error&) {
        Log.error("SPAFarm | 非法正则", pattern);
    }
    return std::nullopt;
}

// 返回第一个捕获组；对应 JS 的 `m[1]`
std::optional<std::string> regex_first_group(const std::string& text, const std::string& pattern)
{
    try {
        const std::regex re(pattern, std::regex::ECMAScript);
        std::smatch m;
        if (std::regex_search(text, m, re) && m.size() > 1) {
            return m[1].str();
        }
    }
    catch (const std::regex_error&) {
        Log.error("SPAFarm | 非法正则", pattern);
    }
    return std::nullopt;
}


} // namespace

// ============================================================================
// Public Entry & Configuration
// ============================================================================

bool SPAFarmTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }
    const std::string& task = details.get("details", "task", std::string());
    return task.ends_with("MiniGame@SPA@BadgeFarm");
}

bool SPAFarmTaskPlugin::set_params(const json::value& params)
{
    if (auto p = params.find<std::string>("profile"); p) {
        m_profile = (*p == "low") ? "low" : "full";
    }
    else if (auto lp = params.find<bool>("low_profile"); lp) {
        m_profile = *lp ? "low" : "full";
    }
    if (auto g = params.find<std::string>("goal"); g && !g->empty()) {
        // 只接受 `pr` 里存在的 id，其余回落 medal（`Bs` 的 `pr.some(...) ? goal : "medal"`）
        const bool known = (*g == "medal")
            || std::any_of(kGoalTasks.begin(), kGoalTasks.end(), [&](const GoalTaskSpec& t) { return t.id == *g; });
        m_goal = known ? *g : "medal";
    }
    apply_goal_bonds();
    // `Bs`：`Number(medalWins) === 6 ? 6 : 3`
    int target_val = params.get("medalWins", params.get("target", params.get("target_clears", 3)));
    m_target = (target_val == 6) ? 6 : 3;
    m_verify_upgrade = params.get("verifyUpgrade", params.get("verify_upgrade", false));
    m_runs = std::max(1, params.get("runs", params.get("max_runs", 20)));
    if (auto mode = params.find<std::string>("mode"); mode && !mode->empty()) {
        set_mode(*mode);
    }
    else {
        set_mode(m_goal == "medal" ? "quick" : m_mode); // `Bs` L8541：medal 强制 quick
    }
    m_attempts = std::max(0, params.get("attempts", 0));
    return true;
}

void SPAFarmTaskPlugin::apply_goal_bonds()
{
    // `en(goal, ...)`：medal → {精准, 奥术}；task-N → `pr` 里的 bonds（L7864-7916）
    if (m_goal == "medal") {
        m_active_core_bonds = { "精准", "奥术" };
        rebuild_stacker_names();
        return;
    }
    for (const auto& t : kGoalTasks) {
        if (t.id == m_goal) {
            m_active_core_bonds = t.bonds;
            rebuild_stacker_names();
            return;
        }
    }
    m_active_core_bonds = { "精准", "奥术" };
}

// `la(a, s)` —— service.pretty.mjs L7914-7923
//   名单 = 特质里同时满足「含 层数+N」与下列任一条件的干员（保持 chess 原序）：
//     · 文本里出现 `【目标盟约】`
//     · 通用表述：层数最多的盟约 / 所有已激活盟约 / 已激活盟约层数 / 身前一格 / 身后一格 / 整备区每名干员已激活的盟约
//     · 「自身已激活的盟约」且该干员自己带目标盟约（`w`）
bool SPAFarmTaskPlugin::trait_mentions_stacking(
    const OperatorInfo& op,
    const std::vector<std::string>& bonds)
{
    const bool has_goal_bond = std::any_of(bonds.begin(), bonds.end(), [&](const std::string& b) {
        return std::find(op.bond_names.begin(), op.bond_names.end(), b) != op.bond_names.end();
    });
    for (const auto& tr : op.traits) {
        const std::string t = strip_ws(tr.text);
        if (!std::regex_search(t, std::regex(R"(层数\+\d)", std::regex::ECMAScript))) {
            continue;
        }
        bool hit = false;
        for (const auto& b : bonds) {
            if (t.find("【" + b + "】") != std::string::npos) {
                hit = true;
                break;
            }
        }
        if (!hit) {
            static const std::vector<std::string> kGeneric = {
                "层数最多的盟约", "所有已激活盟约", "已激活盟约层数",
                "身前一格", "身后一格", "整备区每名干员已激活的盟约"
            };
            for (const auto& g : kGeneric) {
                if (t.find(g) != std::string::npos) {
                    hit = true;
                    break;
                }
            }
        }
        if (!hit && t.find("自身已激活的盟约") != std::string::npos && has_goal_bond) {
            hit = true;
        }
        if (hit) {
            return true;
        }
    }
    return false;
}

void SPAFarmTaskPlugin::rebuild_stacker_names()
{
    // `en(a, s)` L7955-7963：`ht` = medal ? `ua.stackers` : `la(目标盟约, chess)`
    static const std::vector<std::string> kMedalStackers = {
        "伺夜", "空弦", "黑键", "雪猎", "旅骨", "霍尔海雅",
        "迷迭香", "鸿雪", "Miss.Christine", "溯光星源", "逻各斯", "夕"
    };
    m_stacker_list.clear();
    if (m_goal == "medal" || m_operators.empty()) {
        m_stacker_list = kMedalStackers;
    }
    else {
        for (const auto& op : m_operators) {
            if (trait_mentions_stacking(op, m_active_core_bonds)) {
                m_stacker_list.push_back(op.name);
            }
        }
    }
    m_stacker_names.clear();
    for (const auto& n : m_stacker_list) {
        m_stacker_names.insert(n);
    }
}

bool SPAFarmTaskPlugin::in_stacker_list(std::string_view name) const
{
    if (m_stacker_names.empty()) {
        static const std::unordered_set<std::string> kFallback = {
            "伺夜", "空弦", "黑键", "雪猎", "旅骨", "霍尔海雅",
            "迷迭香", "鸿雪", "Miss.Christine", "溯光星源", "逻各斯", "夕"
        };
        return kFallback.count(std::string(name)) > 0;
    }
    return m_stacker_names.count(std::string(name)) > 0;
}

void SPAFarmTaskPlugin::set_profile(bool low_profile)
{
    m_profile = low_profile ? "low" : "full";
}

void SPAFarmTaskPlugin::set_target_clears(int target_clears)
{
    m_target = (target_clears >= 6) ? 6 : 3;
}

void SPAFarmTaskPlugin::set_max_runs(int max_runs)
{
    m_runs = std::max(1, max_runs);
}

void SPAFarmTaskPlugin::reset_match_state()
{
    m_match_info_read = false;
    m_stage_id = "act3autochess_m01";
    m_stage_source = "default";
    m_stage_vote = { "", 0 };
    m_current_round = 0;
    m_last_prep_round = -1;
    m_current_shop_level = 1;
    m_current_gold = 0;
    m_deploy_cap = 8;
    m_upgrade_x_Ref.reset();
    m_units.clear();
    m_shop.clear();
    m_remembered_units.clear();
    m_panel_verified_slots.clear();
    m_op_hist_bank.clear();
    m_empty_cell_refs.clear();
    m_recent_bought_names.clear();
    m_bought_items.clear();
    m_current_boss.clear();
    m_equip_patch_cache.clear();
    m_equip_name_cache.clear();
    m_overflow_equip_units.clear();
    m_equip_drag_missed = false;
    m_unit_equipped_count.clear();
    m_mob_used_this_round = false;
    m_pager_used_round = -1;
    m_active_core_bonds = { "精准", "奥术" };
    apply_goal_bonds();
}

// ============================================================================
// Low-level Controller & Timing
// ============================================================================

Point SPAFarmTaskPlugin::to_720p(double x_1080, double y_1080)
{
    int x = static_cast<int>(std::lround(std::clamp(x_1080, 0.0, 1919.0) * 1280.0 / 1920.0));
    int y = static_cast<int>(std::lround(std::clamp(y_1080, 0.0, 1079.0) * 720.0 / 1080.0));
    return Point(x, y);
}

cv::Mat SPAFarmTaskPlugin::shot() const
{
    for (int i = 0; i < 5; ++i) {
        cv::Mat raw = ctrler()->get_image();
        if (raw.empty()) {
            continue;
        }
        cv::Mat bgr;
        if (raw.channels() == 4) {
            cv::cvtColor(raw, bgr, cv::COLOR_BGRA2BGR);
        }
        else if (raw.channels() == 1) {
            cv::cvtColor(raw, bgr, cv::COLOR_GRAY2BGR);
        }
        else {
            bgr = raw;
        }
        if (bgr.cols != 1920 || bgr.rows != 1080) {
            cv::Mat resized;
            cv::resize(bgr, resized, cv::Size(1920, 1080), 0.0, 0.0, cv::INTER_CUBIC);
            return resized;
        }
        return bgr.clone();
    }
    return {};
}

bool SPAFarmTaskPlugin::sleep_ms(int ms)
{
    if (ms <= 0) {
        return !need_exit();
    }
    return sleep(static_cast<unsigned int>(ms));
}

bool SPAFarmTaskPlugin::sleep_sec(double sec)
{
    if (sec <= 0.0) {
        return !need_exit();
    }
    return sleep(static_cast<unsigned int>(std::lround(sec * 1000.0)));
}

void SPAFarmTaskPlugin::tap(double x, double y, int wait_ms)
{
    ctrler()->click(to_720p(x, y));
    sleep_ms(wait_ms);
}

void SPAFarmTaskPlugin::swipe(double x1, double y1, double x2, double y2, int duration_ms, int wait_ms)
{
    Point p1 = to_720p(x1, y1);
    Point p2 = to_720p(x2, y2);
    ctrler()->swipe(Rect(p1.x, p1.y, 1, 1), Rect(p2.x, p2.y, 1, 1), duration_ms);
    sleep_ms(wait_ms);
}

void SPAFarmTaskPlugin::touch_event(int type, double x, double y) const
{
    InputEvent ev;
    ev.type = static_cast<InputEvent::Type>(type);
    ev.pointerId = 0;
    ev.point = to_720p(x, y);
    ctrler()->inject_input_event(ev);
}

void SPAFarmTaskPlugin::drag_segments(
    double x1,
    double y1,
    double x2,
    double y2,
    int steps,
    int step_ms,
    int hold_ms)
{
    steps = std::max(1, steps);
    touch_event(static_cast<int>(InputEvent::Type::TOUCH_DOWN), x1, y1);
    sleep_ms(static_cast<int>(std::lround(50.0 * m_step_scale)));
    for (int i = 1; i <= steps; ++i) {
        double r = static_cast<double>(i) / static_cast<double>(steps);
        touch_event(
            static_cast<int>(InputEvent::Type::TOUCH_MOVE),
            x1 + (x2 - x1) * r,
            y1 + (y2 - y1) * r);
        sleep_ms(static_cast<int>(std::lround(step_ms * m_step_scale)));
    }
    sleep_ms(static_cast<int>(std::lround(hold_ms * m_step_scale)));
    touch_event(static_cast<int>(InputEvent::Type::TOUCH_UP), x2, y2);
}

cv::Mat SPAFarmTaskPlugin::drag_board_unit(
    double x1,
    double y1,
    double x2,
    double y2,
    bool capture_before_up)
{
    // Exact 1:1 port of `we(I, k, D, B, ...)` in service.pretty.mjs lines 9085-9123:
    // Stage 1: touch_down(x1, y1), hold 220ms, lift vertically to mid_y = min(y1 - 140, y2) in 8 steps (25ms each), hold 60ms
    // Stage 2: move from (x1, mid_y) to (x2, y2) in 10 steps (25ms each), hold 120ms, optional screenshot, touch_up(x2, y2)
    const double mid_y = std::min(y1 - 140.0, y2);
    touch_event(static_cast<int>(InputEvent::Type::TOUCH_DOWN), x1, y1);
    sleep_ms(static_cast<int>(std::lround(220.0 * m_step_scale)));
    for (int i = 1; i <= 8; ++i) {
        double r = static_cast<double>(i) / 8.0;
        touch_event(
            static_cast<int>(InputEvent::Type::TOUCH_MOVE),
            x1,
            y1 + (mid_y - y1) * r);
        sleep_ms(static_cast<int>(std::lround(25.0 * m_step_scale)));
    }
    sleep_ms(static_cast<int>(std::lround(60.0 * m_step_scale)));
    for (int i = 1; i <= 10; ++i) {
        double r = static_cast<double>(i) / 10.0;
        touch_event(
            static_cast<int>(InputEvent::Type::TOUCH_MOVE),
            x1 + (x2 - x1) * r,
            mid_y + (y2 - mid_y) * r);
        sleep_ms(static_cast<int>(std::lround(25.0 * m_step_scale)));
    }
    sleep_ms(static_cast<int>(std::lround(120.0 * m_step_scale)));
    cv::Mat captured;
    if (capture_before_up) {
        captured = shot();
    }
    touch_event(static_cast<int>(InputEvent::Type::TOUCH_UP), x2, y2);
    return captured;
}

// ============================================================================
// Logging & MAA GUI Task Completion / Status Reporting
// ============================================================================

int SPAFarmTaskPlugin::total_available_strategies() const
{
    int cnt = 0;
    for (const auto& s : m_strategies) {
        if (s.available) {
            ++cnt;
        }
    }
    return cnt > 0 ? cnt : 62;
}

void SPAFarmTaskPlugin::log_info(const std::string& msg)
{
    Log.info("SPAFarm |", msg);
    json::value info = basic_info();
    info["what"] = "SPAFarmStatus";
    info["details"] = json::object {
        { "message", msg },
        { "goal", m_goal },
        { "profile", m_profile },
        { "target", m_target },
        { "runs", m_runs_completed },
        { "max_runs", m_runs },
        { "attempts", m_attempts_done },
        { "max_attempts", m_attempts },
        { "medals_earned", m_medals_earned },
        { "completed_strategies", static_cast<int>(m_done_strategies.size()) },
        { "total_strategies", total_available_strategies() },
        { "current_strategy", m_current_strategy },
        { "strategy_id", m_current_strategy_id },
        { "round", m_current_round },
        { "shop_level", m_current_shop_level },
        { "gold", m_current_gold }
    };
    if (status()) {
        status()->set_number("SPAFarm.RunsCompleted", m_runs_completed);
        status()->set_number("SPAFarm.AttemptsDone", m_attempts_done);
        status()->set_number("SPAFarm.MedalsEarned", m_medals_earned);
        status()->set_number("SPAFarm.CompletedStrategies", static_cast<int64_t>(m_done_strategies.size()));
        status()->set_str("SPAFarm.LastMessage", msg);
    }
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void SPAFarmTaskPlugin::report_gui_progress(const std::string& event_type, const std::string& message)
{
    log_info(message);
    json::value info = basic_info_with_what(event_type);
    json::array strat_arr;
    for (const auto& [name, clears] : m_strategy_clears) {
        strat_arr.emplace_back(json::object {
            { "name", name },
            { "clears", clears },
            { "target", m_target },
            { "completed", clears >= m_target }
        });
    }
    info["details"] = json::object {
        { "message", message },
        { "goal", m_goal },
        { "profile", m_profile },
        { "target", m_target },
        { "runs", m_runs_completed },
        { "max_runs", m_runs },
        { "attempts", m_attempts_done },
        { "max_attempts", m_attempts },
        { "medals_earned", m_medals_earned },
        { "completed_strategies", static_cast<int>(m_done_strategies.size()) },
        { "total_strategies", total_available_strategies() },
        { "current_strategy", m_current_strategy },
        { "strategy_id", m_current_strategy_id },
        { "round", m_current_round },
        { "shop_level", m_current_shop_level },
        { "gold", m_current_gold },
        { "strategies", std::move(strat_arr) }
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void SPAFarmTaskPlugin::report_match_finished(bool won, int round_reached)
{
    ++m_attempts_done;
    int cur_clears = 0;
    bool newly_earned_medal = false;
    std::string strat_label = m_current_strategy.empty() ? "当前策略" : m_current_strategy;

    if (!m_current_strategy.empty()) {
        cur_clears = m_strategy_clears[m_current_strategy];
    }
    if (won) {
        ++m_runs_completed;
        if (!m_current_strategy.empty()) {
            bool was_done = m_done_strategies.count(m_current_strategy) > 0 || cur_clears >= m_target;
            cur_clears = ++m_strategy_clears[m_current_strategy];
            for (auto& s : m_strategies) {
                if (s.name == m_current_strategy) {
                    s.passed = cur_clears;
                    break;
                }
            }
            if (cur_clears >= m_target) {
                m_done_strategies.insert(m_current_strategy);
                if (!was_done) {
                    ++m_medals_earned;
                    newly_earned_medal = true;
                }
            }
        }
    }

    std::ostringstream oss;
    oss << "【对局结算】第 " << m_attempts_done << " 场：" << (won ? "模拟完成(胜利)" : "模拟中断(失败)")
        << " | 策略：" << strat_label;
    if (!m_current_strategy.empty()) {
        oss << " (通关 " << cur_clears << "/" << m_target << ")";
    }
    oss << " | 到达回合：" << std::max(1, round_reached)
        << " | 累计通关：" << m_runs_completed << "/" << m_runs
        << " | 策略刻章完成：" << m_done_strategies.size() << "/" << total_available_strategies();
    report_gui_progress("SPAFarmMatchResult", oss.str());

    if (newly_earned_medal) {
        std::ostringstream medal_oss;
        medal_oss << "【刻章达成】策略「" << strat_label << "」已达成 " << m_target
                  << " 次通关刻章！(本次运行新获刻章：" << m_medals_earned
                  << "，总完成策略：" << m_done_strategies.size() << "/" << total_available_strategies() << ")";
        report_gui_progress("SPAFarmMedalEarned", medal_oss.str());
    }
}

void SPAFarmTaskPlugin::report_task_summary(const std::string& stop_reason, bool completed_ok)
{
    std::ostringstream oss;
    oss << "【任务完成情况汇总】状态：" << (completed_ok ? "已完成" : "已停止")
        << " | 原因：" << stop_reason
        << " | 累计通关：" << m_runs_completed << "/" << m_runs << " 局"
        << " | 总尝试：" << m_attempts_done << " 场"
        << " | 本次新获刻章：" << m_medals_earned << " 个"
        << " | 已达标策略：" << m_done_strategies.size() << "/" << total_available_strategies();
    if (!m_strategy_clears.empty()) {
        oss << " | 策略明细：";
        int shown = 0;
        for (const auto& [name, clears] : m_strategy_clears) {
            if (shown > 0) {
                oss << "、";
            }
            oss << name << "(" << clears << "/" << m_target << ")";
            if (++shown >= 8 && m_strategy_clears.size() > 8) {
                oss << " 等共" << m_strategy_clears.size() << "项";
                break;
            }
        }
    }
    const std::string summary_msg = oss.str();
    report_gui_progress("SPAFarmTaskCompleted", summary_msg);

    json::value chain_info = basic_info_with_what("SPAFarmTaskCompleted");
    chain_info["details"]["message"] = summary_msg;
    chain_info["details"]["reason"] = stop_reason;
    chain_info["details"]["completed"] = completed_ok;
    chain_info["details"]["runs"] = m_runs_completed;
    chain_info["details"]["max_runs"] = m_runs;
    chain_info["details"]["attempts"] = m_attempts_done;
    chain_info["details"]["medals_earned"] = m_medals_earned;
    chain_info["details"]["completed_strategies"] = static_cast<int>(m_done_strategies.size());
    chain_info["details"]["total_strategies"] = total_available_strategies();
    callback(AsstMsg::TaskChainExtraInfo, chain_info);
}

// ============================================================================
// OCR & Dictionary Fuzzy Matching (`rt`, `Fi`, `vi`, `Jr`, `xr`, `yi`)
// ============================================================================

std::vector<SPAFarmTaskPlugin::OcrItem> SPAFarmTaskPlugin::ocr(
    const cv::Mat& img,
    const std::optional<std::array<int, 4>>& roi_xywh,
    bool /*pad_replicate*/) const
{
    // `bt(a) = post_recognition("OCR", s ? {roi:s} : {}, png)` —— 带检测的全图识别。
    // 参考实现从不做边界 padding：需要留白的地方（`Jn`/`Mt`/`Bi`）都是自己 extend 黑边。
    if (img.empty()) {
        return {};
    }
    cv::Mat work;
    int ox = 0;
    int oy = 0;
    if (roi_xywh.has_value()) {
        auto [rx, ry, rw, rh] = *roi_xywh;
        work = img_crop(img, rx, ry, rw, rh);
        if (work.empty()) {
            return {};
        }
        ox = std::clamp(rx, 0, img.cols);
        oy = std::clamp(ry, 0, img.rows);
    }
    else {
        work = img;
    }

    auto raw_res = WordOcr::get_instance().recognize(work, false);
    std::vector<OcrItem> out;
    out.reserve(raw_res.size());
    for (const auto& it : raw_res) {
        const int bx = ox + it.rect.x;
        const int by = oy + it.rect.y;
        out.push_back(OcrItem {
            bx + it.rect.width * 0.5,
            by + it.rect.height * 0.5,
            bx,
            by,
            it.rect.width,
            it.rect.height,
            it.text,
            it.score });
    }
    return out;
}

std::vector<SPAFarmTaskPlugin::OcrItem> SPAFarmTaskPlugin::ocr_rec_only(
    const cv::Mat& img,
    const std::array<int, 4>& roi_xywh,
    int upscale,
    int black_border) const
{
    // `yr(a, roi)` = post_recognition("OCR", {roi, only_rec:true}) —— 只跑识别、不做检测。
    // 注意：返回的 rect 必须是**文本框**（原实现错误地填成了整个 ROI）。
    if (img.empty()) {
        return {};
    }
    auto [rx, ry, rw, rh] = roi_xywh;
    cv::Mat crop = img_crop(img, rx, ry, rw, rh);
    if (crop.empty()) {
        return {};
    }
    const int x1 = std::clamp(rx, 0, img.cols);
    const int y1 = std::clamp(ry, 0, img.rows);
    cv::Mat work = crop;
    if (upscale > 1) {
        work = img_resize_cubic(work, work.cols * upscale, work.rows * upscale);
    }
    if (black_border > 0) {
        work = img_extend_black(work, black_border, black_border, black_border, black_border);
    }
    const double inv = (upscale > 1) ? (1.0 / upscale) : 1.0;
    const int pad = black_border;

    auto raw_res = WordOcr::get_instance().recognize(work, true);
    std::vector<OcrItem> out;
    out.reserve(raw_res.size());
    for (const auto& it : raw_res) {
        const double bx = x1 + (it.rect.x - pad) * inv;
        const double by = y1 + (it.rect.y - pad) * inv;
        const double bw = it.rect.width * inv;
        const double bh = it.rect.height * inv;
        out.push_back(OcrItem {
            bx + bw * 0.5,
            by + bh * 0.5,
            static_cast<int>(std::lround(bx)),
            static_cast<int>(std::lround(by)),
            static_cast<int>(std::lround(bw)),
            static_cast<int>(std::lround(bh)),
            it.text,
            it.score });
    }
    return out;
}

// `Te = (a, s) => a.texts.find(e => e.score >= .55 && s.test(e.text.replace(/\s/g, "")))`
// 语义要点：① **按 OCR 结果顺序**取第一条命中的（不是按关键字优先级）；
//           ② 只删空白，不删标点；③ score 门限 0.55。
// 因此多关键字等价于一条 `A|B` 正则，这里把 items 放外层、keywords 放内层即可等价。
std::optional<SPAFarmTaskPlugin::OcrItem> SPAFarmTaskPlugin::find_ocr(
    const std::vector<OcrItem>& items,
    std::initializer_list<std::string_view> keywords)
{
    for (const auto& it : items) {
        if (it.score < 0.55) {
            continue;
        }
        const std::string text = strip_ws(it.text);
        for (std::string_view kw : keywords) {
            if (kw.empty()) {
                continue;
            }
            if (text.find(kw) != std::string::npos) {
                return it;
            }
        }
    }
    return std::nullopt;
}

// 正则版 `Te`：对应 JS 里 `/^升级$/`、`/返回主页|模拟结束/` 这类写法
static std::optional<SPAFarmTaskPlugin::OcrItem> find_text_regex(
    const std::vector<SPAFarmTaskPlugin::OcrItem>& items,
    const std::string& pattern)
{
    std::regex re;
    try {
        re = std::regex(pattern, std::regex::ECMAScript);
    }
    catch (const std::regex_error&) {
        Log.error("SPAFarm | 非法正则", pattern);
        return std::nullopt;
    }
    for (const auto& it : items) {
        if (it.score < 0.55) {
            continue;
        }
        if (std::regex_search(strip_ws(it.text), re)) {
            return it;
        }
    }
    return std::nullopt;
}

[[maybe_unused]] static bool has_text_regex(const std::vector<SPAFarmTaskPlugin::OcrItem>& items, const std::string& pattern)
{
    return find_text_regex(items, pattern).has_value();
}

std::optional<SPAFarmTaskPlugin::OcrItem> SPAFarmTaskPlugin::find_ocr_exact(
    const std::vector<OcrItem>& items,
    std::initializer_list<std::string_view> exact_keywords)
{
    for (const auto& it : items) {
        if (it.score < 0.55) {
            continue;
        }
        const std::string text = strip_ws(it.text);
        for (std::string_view kw : exact_keywords) {
            if (kw.empty()) {
                continue;
            }
            if (text == kw) {
                return it;
            }
        }
    }
    return std::nullopt;
}

std::optional<SPAFarmTaskPlugin::OcrItem> SPAFarmTaskPlugin::find_ocr_in_roi(
    const std::vector<OcrItem>& items,
    std::initializer_list<std::string_view> keywords,
    int x0,
    int y0,
    int x1,
    int y1)
{
    for (const auto& it : items) {
        if (it.score < 0.55 || it.cx < x0 || it.cx > x1 || it.cy < y0 || it.cy > y1) {
            continue;
        }
        const std::string text = strip_ws(it.text);
        for (std::string_view kw : keywords) {
            if (kw.empty()) {
                continue;
            }
            if (text.find(kw) != std::string::npos) {
                return it;
            }
        }
    }
    return std::nullopt;
}

bool SPAFarmTaskPlugin::has_ocr(
    const std::vector<OcrItem>& items,
    std::initializer_list<std::string_view> keywords)
{
    return find_ocr(items, keywords).has_value();
}

bool SPAFarmTaskPlugin::has_ocr_exact(
    const std::vector<OcrItem>& items,
    std::initializer_list<std::string_view> exact_keywords)
{
    return find_ocr_exact(items, exact_keywords).has_value();
}

std::string SPAFarmTaskPlugin::norm_text(std::string_view sv)
{
    // `rt(a)` L5055：只删空白与标点，其余字符（含 ×、+、拉丁扩展）全部保留。
    // 注意：不做繁简/异体转换 —— 参考实现的转换只出现在 `mt`（策略名比对）里。
    return strip_punct(sv);
}

std::unordered_map<std::string, int> SPAFarmTaskPlugin::char_bag(std::string_view norm_sv)
{
    std::unordered_map<std::string, int> bag;
    for (const auto& ch : split_utf8_chars(norm_sv)) {
        bag[ch]++;
    }
    return bag;
}

double SPAFarmTaskPlugin::bag_similarity(
    const std::unordered_map<std::string, int>& a,
    int len_a,
    const std::unordered_map<std::string, int>& b,
    int len_b)
{
    if (len_a <= 0 || len_b <= 0) {
        return 0.0;
    }
    int common = 0;
    for (const auto& [ch, cnt_a] : a) {
        auto it = b.find(ch);
        if (it != b.end()) {
            common += std::min(cnt_a, it->second);
        }
    }
    return static_cast<double>(common) / static_cast<double>(std::max(len_a, len_b));
}

std::vector<SPAFarmTaskPlugin::DictEntry> SPAFarmTaskPlugin::build_dict(const std::vector<std::string>& names)
{
    // `Jr(a)` L5063：`[...new Set([...a].map(rt).filter(Boolean))]`，再按长度降序
    std::set<std::string> uniq;
    for (const auto& name : names) {
        std::string n = strip_punct(name);
        if (!n.empty()) {
            uniq.insert(n);
        }
    }
    std::vector<DictEntry> dict;
    dict.reserve(uniq.size());
    for (const auto& n : uniq) {
        dict.push_back(DictEntry { n, n, utf8_len(n), char_bag(n) });
    }
    std::sort(dict.begin(), dict.end(), [](const DictEntry& a, const DictEntry& b) {
        return a.len > b.len;
    });
    return dict;
}

std::vector<SPAFarmTaskPlugin::DictMatch> SPAFarmTaskPlugin::match_dict(
    std::string_view text,
    const std::vector<DictEntry>& dict,
    bool fuzzy)
{
    // `xr(a, s, {fuzzy:true})` —— service.pretty.mjs L5087-5133
    //
    // 原实现（逐行）：
    //   for (const w of a.matchAll(/\S+/g)) {          // 按 token 切
    //     const m = rt(w[0]); if (!m) continue;
    //     if (s.set.has(m)) { push(整段精确); continue; }
    //     let l = false, E = 0;
    //     while (E < m.length) {                       // 贪心前缀推进
    //       let d = null;
    //       for (const g of s.names) if (g.length >= 2 && m.startsWith(g, E)) { d = g; break; }
    //       d ? (push(d), E += d.length, l = true) : E++;
    //     }
    //     if (!l && fuzzy) { const d = vi(m, s); d && push(d); }
    //   }
    //
    // 差异说明：返回值 start/end 采用**归一化后的字符下标**（JS 用的是原串下标）。
    // 本项目内所有消费方（`yi` 覆盖率、`Bi` token 定位）都在归一化空间里用，等价。

    std::vector<DictMatch> out;
    if (dict.empty()) {
        return out;
    }

    // 1) 切成 token，同时去掉标点（保留 token 边界）
    struct TokenChar
    {
        std::string ch;
        int token = 0;
    };
    std::vector<TokenChar> nc;
    int token = -1;
    bool in_token = false;
    for (const auto& ch : split_utf8_chars(text)) {
        const bool ws = (ch.size() == 1
                         && (ch[0] == ' ' || ch[0] == '\t' || ch[0] == '\n' || ch[0] == '\r' || ch[0] == '\f'
                             || ch[0] == '\v'))
            || ch == "\u3000";
        if (ws) {
            in_token = false;
            continue;
        }
        if (!in_token) {
            ++token;
            in_token = true;
        }
        if (is_rt_punct(ch)) {
            continue;
        }
        nc.push_back(TokenChar { ch, token });
    }
    if (nc.empty()) {
        return out;
    }

    std::unordered_map<std::string, int> name_len;
    name_len.reserve(dict.size());
    for (const auto& d : dict) {
        name_len.emplace(d.norm, d.len);
    }

    // `vi(a, s)` L5080：整串模糊匹配，长度差 <= 1，得分 >= 0.66，并列视为歧义
    auto fuzzy_pick = [&](const std::vector<std::string>& chars) -> const DictEntry* {
        const int alen = static_cast<int>(chars.size());
        if (alen < 2) {
            return nullptr;
        }
        std::unordered_map<std::string, int> bag;
        for (const auto& c : chars) {
            ++bag[c];
        }
        const DictEntry* best = nullptr;
        double best_v = 0.0;
        bool ambiguous = false;
        for (const auto& d : dict) {
            if (std::abs(d.len - alen) > 1) {
                continue;
            }
            int common = 0;
            for (const auto& [ch, cnt] : bag) {
                auto it = d.bag.find(ch);
                if (it != d.bag.end()) {
                    common += std::min(cnt, it->second);
                }
            }
            const double v = static_cast<double>(common) / static_cast<double>(std::max(alen, d.len));
            if (v > best_v) {
                best = &d;
                best_v = v;
                ambiguous = false;
            }
            else if (v == best_v) {
                ambiguous = true;
            }
        }
        if (best != nullptr && best_v >= 0.66 && !ambiguous) {
            return best;
        }
        return nullptr;
    };

    size_t i = 0;
    while (i < nc.size()) {
        const int tk = nc[i].token;
        size_t j = i;
        while (j < nc.size() && nc[j].token == tk) {
            ++j;
        }

        std::vector<std::string> tok_chars;
        tok_chars.reserve(j - i);
        for (size_t k = i; k < j; ++k) {
            tok_chars.push_back(nc[k].ch);
        }
        std::string tok;
        for (const auto& c : tok_chars) {
            tok += c;
        }

        auto whole = name_len.find(tok);
        if (whole != name_len.end()) {
            out.push_back(DictMatch {
                tok,
                whole->second,
                static_cast<int>(i),
                static_cast<int>(j),
                1.0,
                true });
            i = j;
            continue;
        }

        bool matched = false;
        size_t pos = 0;
        while (pos < tok_chars.size()) {
            const DictEntry* hit = nullptr;
            for (const auto& d : dict) {
                if (d.len < 2) {
                    continue;
                }
                // startsWith(g, pos)：按字符比较
                bool prefix = true;
                const auto dchars = split_utf8_chars(d.norm);
                if (pos + dchars.size() > tok_chars.size()) {
                    continue;
                }
                for (size_t k = 0; k < dchars.size(); ++k) {
                    if (tok_chars[pos + k] != dchars[k]) {
                        prefix = false;
                        break;
                    }
                }
                if (prefix) {
                    hit = &d;
                    break;
                }
            }
            if (hit != nullptr) {
                const int dlen = hit->len;
                out.push_back(DictMatch {
                    hit->name,
                    dlen,
                    static_cast<int>(i + pos),
                    static_cast<int>(i + pos + dlen),
                    1.0,
                    true });
                pos += static_cast<size_t>(dlen);
                matched = true;
            }
            else {
                ++pos;
            }
        }

        if (!matched && fuzzy) {
            if (const DictEntry* fz = fuzzy_pick(tok_chars)) {
                out.push_back(DictMatch {
                    fz->name,
                    fz->len,
                    static_cast<int>(i),
                    static_cast<int>(j),
                    0.66,
                    false });
            }
        }
        i = j;
    }

    return out;
}

double SPAFarmTaskPlugin::match_coverage(std::string_view text, const std::vector<DictMatch>& matches)
{
    auto chars = split_utf8_chars(norm_text(text));
    if (chars.empty()) {
        return 0.0;
    }
    int covered = 0;
    for (const auto& m : matches) {
        covered += (m.end - m.start);
    }
    return static_cast<double>(covered) / static_cast<double>(chars.size());
}

// ============================================================================
// Data Loading (`oa`, `hr`/`pr`, `Er`)
// ============================================================================

SPAFarmTaskPlugin::StackTraitInfo SPAFarmTaskPlugin::parse_stack_trait(std::string_view trait_text)
{
    // Exact 1:1 port of `hr(a)` in service.pretty.mjs lines 5817-5904
    StackTraitInfo info;
    if (trait_text.empty()) {
        return info;
    }
    std::string text(trait_text);
    if (text.find("叠层") == std::string::npos && text.find("层数") == std::string::npos) {
        return info;
    }

    auto extract_bond = [](std::string_view clause) -> std::string {
        static const std::vector<std::string> kBondNames = {
            "迅捷", "坚城", "强攻", "精准", "奥术", "守望", "突袭", "堡垒", "不朽",
            "刚毅", "远见", "奇迹", "灵巧", "锐利", "源石", "辞岁", "不赦", "荣光",
            "萨尔贡", "谢拉格", "拉特兰", "炎", "极昼", "辅助", "医疗", "近卫"
        };
        for (const auto& b : kBondNames) {
            if (clause.find("[" + b + "]") != std::string::npos || clause.find(b) != std::string::npos) {
                return b;
            }
        }
        return "";
    };

    std::string nb = extract_bond(text);
    bool other_only = (text.find("其他") != std::string::npos);

    if (text.find("前方一格") != std::string::npos) {
        info.rules.push_back(StackGeomRule { "front1", nb, true, false });
    }
    if (text.find("身后一格") != std::string::npos) {
        info.rules.push_back(StackGeomRule { "back1", nb, true, false });
    }
    if (text.find("同一行") != std::string::npos) {
        info.rules.push_back(StackGeomRule { "sameRow", nb, other_only, false });
    }
    if (text.find("周围四格") != std::string::npos) {
        info.rules.push_back(StackGeomRule { "around4", nb, true, false });
    }
    if (text.find("周围八格") != std::string::npos) {
        info.rules.push_back(StackGeomRule { "around8", nb, true, false });
    }
    if (text.find("周围十二格") != std::string::npos) {
        info.rules.push_back(StackGeomRule { "around12", nb, true, false });
    }
    if (text.find("攻击范围内") != std::string::npos && text.find("自身不处于") == std::string::npos) {
        info.rules.push_back(StackGeomRule { "inRange", nb, other_only, false });
    }

    info.on_skill = (text.find("技能") != std::string::npos && text.find("时") != std::string::npos);
    info.on_deploy = (text.find("部署") != std::string::npos);
    info.end_of_prep = (text.find("休整阶段结束") != std::string::npos);
    info.infects_tile = (text.find("格变为感染") != std::string::npos);
    info.uses_infected_tile = (text.find("位于感染格") != std::string::npos);
    return info;
}

std::vector<std::pair<int, int>> SPAFarmTaskPlugin::default_range_for_subprof(
    std::string_view sub_prof,
    std::string_view pos)
{
    // Exact 1:1 port of `Er` in service.pretty.mjs lines 5684-5728
    auto rect_range = [](int r0, int r1, int c0, int c1) {
        std::vector<std::pair<int, int>> out;
        for (int r = r0; r <= r1; ++r) {
            for (int c = c0; c <= c1; ++c) {
                out.emplace_back(r, c);
            }
        }
        return out;
    };

    if (sub_prof == "aoesniper" || sub_prof == " artilleryman") {
        return rect_range(-1, 1, 0, 4);
    }
    if (sub_prof == "reaperrange" || sub_prof == " Longrange") {
        return rect_range(-2, 2, 0, 4);
    }
    if (sub_prof == "fastshot" || sub_prof == "closerange" || sub_prof == "corecaster"
        || sub_prof == "splashcaster" || sub_prof == "funnel" || sub_prof == "chain"
        || sub_prof == "slower" || sub_prof == "summoner" || sub_prof == "ritualist"
        || sub_prof == "alchemist" || sub_prof == "medebuff") {
        return rect_range(-1, 1, 0, 3);
    }
    if (sub_prof == "physician" || sub_prof == "ringhealer" || sub_prof == "healer") {
        return rect_range(-1, 1, 0, 3);
    }
    if (sub_prof == "wandermedic" || sub_prof == "bard" || sub_prof == "blastcaster") {
        return rect_range(-2, 2, -2, 2);
    }
    if (sub_prof == "lord") {
        auto r = rect_range(-1, 1, 0, 2);
        r.emplace_back(0, 3);
        return r;
    }
    if (sub_prof == "instructor" || sub_prof == "charger") {
        return rect_range(0, 0, 0, 2);
    }
    if (sub_prof == "protector" || sub_prof == "guardian" || sub_prof == "unyield"
        || sub_prof == "artsprotector" || sub_prof == "duelist") {
        return { { 0, 0 } };
    }
    if (pos == "RANGED") {
        return rect_range(-1, 1, 0, 3);
    }
    return rect_range(0, 0, 0, 1);
}

std::pair<int, int> SPAFarmTaskPlugin::rotate_offset(std::pair<int, int> dr_dc, std::string_view facing)
{
    // Exact 1:1 port of `Br([a, s], e)` in service.pretty.mjs lines 5661-5673
    const int a = dr_dc.first;
    const int s = dr_dc.second;
    const int t = -a;
    if (facing == "RIGHT") {
        return { t, s };
    }
    if (facing == "LEFT") {
        return { -t, -s };
    }
    if (facing == "UP") {
        return { -s, -t };
    }
    if (facing == "DOWN") {
        return { s, t };
    }
    return { t, s };
}

void SPAFarmTaskPlugin::init_builtin_fallbacks()
{
    m_collapsed_board_H = solve_homography({
        { 4, 1, 790, 308 },
        { 9, 1, 1380, 308 },
        { 4, 2, 785, 400 },
        { 9, 2, 1395, 400 },
        { 4, 5, 760, 712 },
        { 9, 5, 1450, 712 },
        { 4, 6, 748, 840 },
        { 9, 6, 1465, 840 }
    });

    if (m_calib_expanded.anchors.empty()) {
        m_calib_expanded.camera = "expanded";
        m_calib_expanded.anchors = {
            { 1, 1, 469, 174 }, { 12, 1, 1659, 174 },
            { 1, 2, 439, 272 }, { 12, 2, 1690, 272 },
            { 1, 3, 409, 376 }, { 12, 3, 1720, 376 },
            { 1, 4, 376, 488 }, { 12, 4, 1752, 488 }
        };
        m_calib_expanded.marker_E = { 597.5, 496.5 };
        m_calib_expanded.marker_S1 = { 1398.5, 211.0 };
        m_calib_expanded.marker_S2 = { 1554.0, 412.5 };
        m_calib_expanded.ref_n_E = 899;
        m_calib_expanded.ref_n_S1 = 964;
        m_calib_expanded.ref_n_S2 = 965;
        m_calib_expanded.base_H = solve_homography(m_calib_expanded.anchors);
    }

    if (m_calib_collapsed.anchors.empty()) {
        m_calib_collapsed.camera = "collapsed";
        m_calib_collapsed.anchors = {
            { 1, 1, 432, 308 }, { 12, 1, 1689, 308 },
            { 1, 2, 402, 400 }, { 12, 2, 1718, 400 },
            { 1, 3, 370, 500 }, { 12, 3, 1750, 500 },
            { 1, 4, 336, 610 }, { 12, 4, 1784, 610 }
        };
        m_calib_collapsed.marker_E = { 555.5, 651.5 };
        m_calib_collapsed.marker_S1 = { 1383.5, 344.5 };
        m_calib_collapsed.marker_S2 = { 1546.5, 560.5 };
        m_calib_collapsed.ref_n_E = 885;
        m_calib_collapsed.ref_n_S1 = 948;
        m_calib_collapsed.ref_n_S2 = 955;
        m_calib_collapsed.base_H = solve_homography(m_calib_collapsed.anchors);
    }

    if (m_stages.empty()) {
        StageInfo st;
        st.stage_id = "act3autochess_m01";
        st.name = "卫戍协议默认地图";
        st.width = 13;
        st.height = 7;
        st.rows = {
            "xxxxxxxxxxxxx",
            "xxxhhaahSxxxx",
            "xxxhaabbbxxxx",
            "xxxhbbhhbSxxx",
            "xxEbbbbbbxxxx",
            "xxxxQQQQQQxxx",
            "xxxHHHHHHHHHx"
        };
        m_stages.push_back(std::move(st));
    }
}

bool SPAFarmTaskPlugin::load_all_data()
{
    if (m_data_loaded) {
        return true;
    }

    init_builtin_fallbacks();

    std::vector<std::filesystem::path> candidate_dirs = {
        std::filesystem::current_path() / "resource" / "SPAFarm",
        std::filesystem::path("resource") / "SPAFarm",
        std::filesystem::path("..") / "resource" / "SPAFarm",
        std::filesystem::path("data")
    };

    std::filesystem::path data_dir;
    for (const auto& dir : candidate_dirs) {
        std::error_code ec;
        if (std::filesystem::exists(dir / "chess.json", ec)) {
            data_dir = dir;
            break;
        }
    }

    auto read_json_file = [](const std::filesystem::path& p) -> std::optional<json::value> {
        std::error_code ec;
        if (!std::filesystem::exists(p, ec)) {
            return std::nullopt;
        }
        return json::open(p);
    };

    if (!data_dir.empty()) {
        // 1. Load bonds.json
        if (auto j_bonds = read_json_file(data_dir / "bonds.json"); j_bonds && j_bonds->is_array()) {
            m_bonds.clear();
            m_bond_by_norm.clear();
            for (const auto& jb : j_bonds->as_array()) {
                BondInfo b;
                b.bond_id = jb.get("bondId", std::string());
                b.name = jb.get("name", std::string());
                b.norm_name = norm_text(b.name);
                b.is_core = jb.get("isCore", false);
                b.stackable = jb.get("stackable", false);
                b.inverse = jb.get("inverse", false);
                b.count_mode = jb.get("countMode", std::string("FIELD_ONLY"));
                b.max_Active = jb.get("activeCount", 6);
                b.desc = jb.get("desc", std::string());
                if (auto th = jb.find<json::array>("thresholds"); th) {
                    for (const auto& v : *th) {
                        if (v.is_number()) {
                            b.thresholds.push_back(v.as_integer());
                        }
                    }
                }
                if (b.thresholds.empty()) {
                    b.thresholds.push_back(std::max(2, b.max_Active));
                }
                if (auto ops = jb.find<json::array>("operatorNames"); ops) {
                    for (const auto& v : *ops) {
                        if (v.is_string()) {
                            b.operator_names.push_back(v.as_string());
                        }
                    }
                }
                if (!b.norm_name.empty()) {
                    m_bond_by_norm[b.norm_name] = m_bonds.size();
                    m_bonds.push_back(std::move(b));
                }
            }
        }

        // 2. Load chess.json (non-golden base entries + elite stats)
        if (auto j_chess = read_json_file(data_dir / "chess.json"); j_chess && j_chess->is_array()) {
            m_operators.clear();
            m_op_by_norm.clear();
            for (const auto& jc : j_chess->as_array()) {
                if (jc.get("golden", false)) {
                    continue;
                }
                OperatorInfo op;
                op.chess_id = jc.get("chessId", std::string());
                op.char_id = jc.get("charId", std::string());
                op.name = jc.get("name", std::string());
                op.norm_name = norm_text(op.name);
                op.tier = std::clamp(jc.get("tier", 1), 1, 6);
                op.elite = false;
                op.golden = false;
                op.profession = jc.get("profession", std::string());
                // 资源实际字段是 `subProfession`；`subProfessionId` 只是历史别名
                op.sub_profession_id = jc.get("subProfession", jc.get("subProfessionId", std::string()));
                op.position = jc.get("position", std::string("MELEE"));
                // 资源实际字段是 `price` / `soldPrice`
                op.purchase_price = jc.get("price", jc.get("purchasePrice", 2));
                op.sell_price = jc.get("soldPrice", jc.get("sellPrice", 1));
                op.self_heal = jc.get("selfHeal", false);
                op.heals_allies = jc.get("healsAllies", false);
                if (auto st = jc.find<json::object>("stats"); st) {
                    op.atk = st->get("atk", 0);
                    op.def = st->get("def", 0);
                    op.max_hp = st->get("hp", st->get("maxHp", 0));
                    op.block_cnt = st->get("block", st->get("blockCnt", 1));
                    op.res = st->get("res", 0);
                    op.bat = st->get("bat", 0.0);
                }
                // `traits[]`：文本 + tags（Nt 的 front1/back1/around4/… 全靠 tags）
                if (auto traits = jc.find<json::array>("traits"); traits) {
                    for (const auto& jt : *traits) {
                        if (!jt.is_object()) {
                            continue;
                        }
                        OperatorTrait tr;
                        tr.id = jt.get("id", std::string());
                        tr.text = jt.get("text", std::string());
                        if (auto tags = jt.find<json::array>("tags"); tags) {
                            for (const auto& tag : *tags) {
                                if (tag.is_string()) {
                                    tr.tags.push_back(tag.as_string());
                                }
                            }
                        }
                        if (op.trait_text.empty()) {
                            op.trait_text = tr.text;
                        }
                        else {
                            op.trait_text += " " + tr.text;
                        }
                        op.traits.push_back(std::move(tr));
                    }
                }
                if (op.trait_text.empty()) {
                    op.trait_text = jc.get("traitText", std::string());
                }
                if (auto rng = jc.find<json::array>("rangeGrid"); rng) {
                    for (const auto& cell : *rng) {
                        if (!cell.is_array()) {
                            continue;
                        }
                        const auto& arr = cell.as_array();
                        if (arr.size() >= 2) {
                            op.range_grid.emplace_back(
                                static_cast<int>(arr[0].as_integer()),
                                static_cast<int>(arr[1].as_integer()));
                        }
                    }
                }
                if (auto bids = jc.find<json::array>("bonds"); bids) {
                    for (const auto& v : *bids) {
                        if (v.is_string()) {
                            op.bond_ids.push_back(v.as_string());
                        }
                    }
                }
                if (auto bnames = jc.find<json::array>("bondNames"); bnames) {
                    for (const auto& v : *bnames) {
                        if (v.is_string()) {
                            op.bond_names.push_back(v.as_string());
                        }
                    }
                }
                op.stack_info = parse_stack_trait(op.trait_text);
                op.base_range = op.range_grid.empty()
                    ? default_range_for_subprof(op.sub_profession_id, op.position)
                    : op.range_grid;
                op.can_block_air = (op.name == "琉璃");
                op.is_healer = (op.profession == "MEDIC" || op.sub_profession_id == "bard" || op.sub_profession_id == "guardian");
                op.facing_independent = (
                    op.sub_profession_id == "bard" || op.sub_profession_id == "wandermedic"
                    || op.sub_profession_id == "summoner" || op.sub_profession_id == "ritualist"
                    || op.sub_profession_id == "stalker" || op.sub_profession_id == "alchemist"
                    || op.sub_profession_id == "craftsman");

                bool has_jiancheng = std::find(op.bond_names.begin(), op.bond_names.end(), "坚城") != op.bond_names.end();
                bool has_qiangdun = std::find(op.bond_names.begin(), op.bond_names.end(), "强盾") != op.bond_names.end();
                op.taunt_level = has_jiancheng ? 3 : (has_qiangdun || op.profession == "TANK") ? 2
                    : (op.profession == "PIONEER" || op.profession == "WARRIOR") ? 1 : 0;

                static constexpr double kTierWeight[7] = { 0.0, 0.7, 0.85, 1.0, 1.2, 1.45, 1.75 };
                op.dps_weight = kTierWeight[op.tier];
                if (op.profession == "MEDIC") {
                    op.dps_weight *= 0.25;
                }
                else if (op.profession == "TANK" && op.sub_profession_id != "artsprotector") {
                    op.dps_weight *= 0.45;
                }

                std::string skills_str = op.trait_text + jc.get("skill", std::string()) + jc.get("talents", std::string());
                op.is_summoner = (
                    skills_str.find("召唤") != std::string::npos
                    || skills_str.find("装置") != std::string::npos
                    || skills_str.find("无人机") != std::string::npos);

                if (!op.norm_name.empty() && m_op_by_norm.find(op.norm_name) == m_op_by_norm.end()) {
                    m_op_by_norm[op.norm_name] = m_operators.size();
                    m_operators.push_back(std::move(op));
                }
            }
        }

        // 3. Load items.json
        if (auto j_items = read_json_file(data_dir / "items.json"); j_items && j_items->is_array()) {
            m_items.clear();
            m_item_by_norm.clear();
            for (const auto& ji : j_items->as_array()) {
                if (ji.get("golden", false)) {
                    continue;
                }
                ItemInfo it;
                it.id = ji.get("chessId", std::string());
                it.name = ji.get("name", std::string());
                it.norm_name = norm_text(it.name);
                it.tier = ji.get("tier", 1);
                it.price = ji.get("price", 2);
                // 资源里的装备描述字段是 `desc`（对应 JS `An(a)` 里的 `a.desc`）
                it.effect = ji.get("desc", std::string());
                if (!it.norm_name.empty() && m_item_by_norm.find(it.norm_name) == m_item_by_norm.end()) {
                    m_item_by_norm[it.norm_name] = m_items.size();
                    m_items.push_back(std::move(it));
                }
            }
        }

        // 4. Load farm-rules.json
        if (auto j_rules = read_json_file(data_dir / "farm-rules.json"); j_rules && j_rules->is_object()) {
            if (auto shop = j_rules->find<json::object>("shop"); shop) {
                for (const auto& [key, val] : *shop) {
                    (void)key;
                    if (!val.is_object()) {
                        continue;
                    }
                    const int lv = val.get("shopLevel", 0);
                    if (lv > 0) {
                        m_shop_upgrade_costs[lv] = val.get("initialUpgradePrice", 0);
                    }
                }
            }
            if (auto strats = j_rules->find<json::array>("strategies"); strats) {
                m_strategies.clear();
                for (const auto& js : *strats) {
                    StrategyRule sr;
                    sr.id = js.get("id", std::string());
                    sr.name = js.get("name", std::string());
                    sr.norm_name = norm_text(sr.name);
                    sr.owner = js.get("owner", std::string());
                    sr.available = js.get("available", true);
                    if (!sr.name.empty()) {
                        m_strategies.push_back(std::move(sr));
                    }
                }
            }
        }

        // 5. Load stages.json
        if (auto j_stages = read_json_file(data_dir / "stages.json"); j_stages && j_stages->is_array()) {
            m_stages.clear();
            for (const auto& jst : j_stages->as_array()) {
                StageInfo st;
                st.stage_id = jst.get("stageId", std::string());
                st.name = jst.get("name", std::string());
                st.width = jst.get("width", 13);
                st.height = jst.get("height", 7);
                if (auto special = jst.find<json::array>("special"); special) {
                    for (const auto& sp : *special) {
                        if (!sp.is_object() || sp.get("tileKey", std::string()) != "tile_infection") {
                            continue;
                        }
                        st.infection_tiles.emplace_back(sp.get("r", 0), sp.get("c", 0));
                    }
                }
                if (auto r_arr = jst.find<json::array>("rows"); r_arr) {
                    for (const auto& rv : *r_arr) {
                        if (rv.is_string()) {
                            st.rows.push_back(rv.as_string());
                        }
                    }
                }
                // Apply `wr` tile overrides for act3autochess_m01 (service.pretty.mjs lines 7529-7558)
                if (st.stage_id == "act3autochess_m01" && st.rows.size() >= 5) {
                    if (st.rows[4].size() > 9) {
                        st.rows[4][9] = 'a';
                        st.rows[4][8] = 'a';
                    }
                    if (st.rows[3].size() > 10) {
                        st.rows[3][10] = 'c';
                    }
                    if (st.rows[2].size() > 10) {
                        st.rows[2][10] = 'c';
                    }
                }
                if (!st.stage_id.empty() && !st.rows.empty()) {
                    m_stages.push_back(std::move(st));
                }
            }
        }

        // 6. Load board-refs.json
        if (auto j_brefs = read_json_file(data_dir / "board-refs.json"); j_brefs && j_brefs->is_array()) {
            m_board_refs.clear();
            for (const auto& jr : j_brefs->as_array()) {
                BoardRefEntry entry;
                entry.stage_id = jr.get("stageId", std::string());
                entry.camera = jr.get("camera", std::string());
                if (auto cells_obj = jr.find<json::object>("cells"); cells_obj) {
                    for (const auto& [cell_key, arr_val] : *cells_obj) {
                        if (!arr_val.is_array()) {
                            continue;
                        }
                        std::vector<double> vals;
                        for (const auto& elem : arr_val.as_array()) {
                            if (elem.is_number()) {
                                vals.push_back(elem.as_double());
                            }
                            else {
                                vals.push_back(std::numeric_limits<double>::quiet_NaN());
                            }
                        }
                        entry.cells[cell_key] = std::move(vals);
                    }
                }
                if (!entry.stage_id.empty()) {
                    m_board_refs.push_back(std::move(entry));
                }
            }
        }

        // 6b. `he` 的 6 个 TemplateMatch 模板（badge_l1..l6.png，纯绿背景由 green_mask 掩掉）
        m_badge_templates.clear();
        for (int tier = 1; tier <= 6; ++tier) {
            const auto p = data_dir / "image" / ("badge_l" + std::to_string(tier) + ".png");
            std::error_code ec;
            if (!std::filesystem::exists(p, ec)) {
                Log.warn("SPAFarm | missing badge template", p.string());
                continue;
            }
            cv::Mat t = cv::imread(p.string(), cv::IMREAD_COLOR);
            if (t.empty()) {
                Log.warn("SPAFarm | failed to load badge template", p.string());
                continue;
            }
            m_badge_templates.push_back(std::move(t));
        }
        if (m_badge_templates.size() != 6) {
            Log.warn("SPAFarm | badge templates incomplete:", m_badge_templates.size());
        }

        // 7. Load bosses.json & enemy-types.json
        if (auto j_bosses = read_json_file(data_dir / "bosses.json"); j_bosses && j_bosses->is_array()) {
            m_bosses.clear();
            for (const auto& jb : j_bosses->as_array()) {
                BossInfo bi;
                bi.enemy_id = jb.get("enemyId", std::string());
                bi.name = jb.get("name", std::string());
                bi.norm_name = norm_text(bi.name);
                bi.description = jb.get("description", std::string());
                if (!bi.name.empty()) {
                    m_bosses.push_back(std::move(bi));
                }
            }
        }
        if (auto j_etypes = read_json_file(data_dir / "enemy-types.json"); j_etypes && j_etypes->is_array()) {
            m_enemy_types.clear();
            for (const auto& je : j_etypes->as_array()) {
                EnemyTypeInfo ei;
                ei.code = je.get("code", std::string());
                ei.name = je.get("name", std::string());
                if (!ei.name.empty()) {
                    m_enemy_types.push_back(std::move(ei));
                }
            }
        }

        // 8. Load calib/m01-expanded.json & calib/m01-collapsed.json
        auto load_calib = [&](const std::filesystem::path& p, CameraCalibData& dst) {
            auto jc = read_json_file(p);
            if (!jc || !jc->is_object()) {
                return;
            }
            dst.camera = jc->get("camera", dst.camera);
            if (auto anc = jc->find<json::array>("anchors"); anc) {
                dst.anchors.clear();
                for (const auto& a : *anc) {
                    if (a.is_array() && a.as_array().size() >= 4) {
                        const auto& arr = a.as_array();
                        dst.anchors.push_back({
                            arr[0].as_double(),
                            arr[1].as_double(),
                            arr[2].as_double(),
                            arr[3].as_double() });
                    }
                }
            }
            if (auto mref = jc->find<json::object>("markerRef"); mref) {
                auto read_pt = [&](const char* k, std::array<double, 2>& pt) {
                    if (auto arr = mref->find<json::array>(k); arr && arr->size() >= 2) {
                        pt = { (*arr)[0].as_double(), (*arr)[1].as_double() };
                    }
                };
                read_pt("E", dst.marker_E);
                read_pt("S1", dst.marker_S1);
                read_pt("S2", dst.marker_S2);
            }
            if (auto nref = jc->find<json::object>("markerRefN"); nref) {
                dst.ref_n_E = nref->get("E", dst.ref_n_E);
                dst.ref_n_S1 = nref->get("S1", dst.ref_n_S1);
                dst.ref_n_S2 = nref->get("S2", dst.ref_n_S2);
            }
            if (dst.anchors.size() >= 4) {
                dst.base_H = solve_homography(dst.anchors);
            }
        };
        load_calib(data_dir / "calib" / "m01-expanded.json", m_calib_expanded);
        load_calib(data_dir / "calib" / "m01-collapsed.json", m_calib_collapsed);
    }

    // Build fuzzy lookup dictionaries (`Jr`)
    std::vector<std::string> op_names;
    for (const auto& op : m_operators) {
        op_names.push_back(op.name);
    }
    m_chess_dict = build_dict(op_names);

    std::vector<std::string> item_names;
    for (const auto& it : m_items) {
        item_names.push_back(it.name);
    }
    m_item_dict = build_dict(item_names);

    std::vector<std::string> bond_names;
    for (const auto& b : m_bonds) {
        bond_names.push_back(b.name);
    }
    m_bond_dict = build_dict(bond_names);

    std::vector<std::string> boss_names;
    for (const auto& b : m_bosses) {
        boss_names.push_back(b.name);
    }
    m_boss_dict = build_dict(boss_names);

    std::vector<std::string> strat_names;
    for (const auto& s : m_strategies) {
        strat_names.push_back(s.name);
    }
    m_strategy_dict = build_dict(strat_names);

    m_data_loaded = true;
    return true;
}

const SPAFarmTaskPlugin::OperatorInfo* SPAFarmTaskPlugin::find_operator(std::string_view name) const
{
    std::string n = norm_text(name);
    auto it = m_op_by_norm.find(n);
    if (it != m_op_by_norm.end() && it->second < m_operators.size()) {
        return &m_operators[it->second];
    }
    return nullptr;
}

const SPAFarmTaskPlugin::ItemInfo* SPAFarmTaskPlugin::find_item(std::string_view name) const
{
    std::string n = norm_text(name);
    auto it = m_item_by_norm.find(n);
    if (it != m_item_by_norm.end() && it->second < m_items.size()) {
        return &m_items[it->second];
    }
    return nullptr;
}

const SPAFarmTaskPlugin::BondInfo* SPAFarmTaskPlugin::find_bond(std::string_view name) const
{
    std::string n = norm_text(name);
    auto it = m_bond_by_norm.find(n);
    if (it != m_bond_by_norm.end() && it->second < m_bonds.size()) {
        return &m_bonds[it->second];
    }
    return nullptr;
}

const SPAFarmTaskPlugin::StageInfo* SPAFarmTaskPlugin::find_stage(std::string_view stage_id) const
{
    for (const auto& st : m_stages) {
        if (st.stage_id == stage_id) {
            return &st;
        }
    }
    return m_stages.empty() ? nullptr : &m_stages.front();
}

// ============================================================================
// Projective Homography & Dual-Camera Calibration (`kr`, `ft`, `os`, `Un`, `pn`, `wn`, `Hn`, `Sr`)
// ============================================================================

std::array<double, 9> SPAFarmTaskPlugin::solve_homography(const std::vector<std::array<double, 4>>& anchors)
{
    // Exact 1:1 port of `kr(a)` in service.pretty.mjs lines 4274-4298 (8-DOF projective homography via normal equations + Gaussian elimination)
    if (anchors.size() < 4) {
        return { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    }
    double A[8][9] = {};
    for (const auto& anc : anchors) {
        const double gx = anc[0];
        const double gy = anc[1];
        const double px = anc[2];
        const double py = anc[3];
        const double r1[9] = { gx, gy, 1.0, 0.0, 0.0, 0.0, -gx * px, -gy * px, px };
        const double r2[9] = { 0.0, 0.0, 0.0, gx, gy, 1.0, -gx * py, -gy * py, py };
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 9; ++j) {
                A[i][j] += r1[i] * r1[j] + r2[i] * r2[j];
            }
        }
    }

    for (int i = 0; i < 8; ++i) {
        int piv = i;
        for (int r = i + 1; r < 8; ++r) {
            if (std::abs(A[r][i]) > std::abs(A[piv][i])) {
                piv = r;
            }
        }
        if (std::abs(A[piv][i]) < 1e-12) {
            continue;
        }
        if (piv != i) {
            for (int c = i; c <= 8; ++c) {
                std::swap(A[i][c], A[piv][c]);
            }
        }
        for (int r = i + 1; r < 8; ++r) {
            const double f = A[r][i] / A[i][i];
            for (int c = i; c <= 8; ++c) {
                A[r][c] -= f * A[i][c];
            }
        }
    }

    std::array<double, 9> h = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    for (int i = 7; i >= 0; --i) {
        double s = A[i][8];
        for (int c = i + 1; c < 8; ++c) {
            s -= A[i][c] * h[c];
        }
        h[i] = (std::abs(A[i][i]) > 1e-12) ? (s / A[i][i]) : 0.0;
    }
    h[8] = 1.0;
    return h;
}

std::pair<double, double> SPAFarmTaskPlugin::project_point(const std::array<double, 9>& H, double gx, double gy)
{
    // Exact 1:1 port of `ft(a, s, e)` in service.pretty.mjs lines 4300-4303
    const double w = H[6] * gx + H[7] * gy + H[8];
    if (std::abs(w) < 1e-12) {
        return { 0.0, 0.0 };
    }
    return {
        (H[0] * gx + H[1] * gy + H[2]) / w,
        (H[3] * gx + H[4] * gy + H[5]) / w
    };
}

std::array<double, 9> SPAFarmTaskPlugin::invert_homography(const std::array<double, 9>& H)
{
    // Exact 1:1 port of `os(a)` in service.pretty.mjs lines 6486-6489
    const double s = H[0], e = H[1], t = H[2];
    const double f = H[3], w = H[4], i = H[5];
    const double m = H[6], u = H[7], c = H[8];
    const double l = w * c - i * u;
    const double E = -(f * c - i * m);
    const double d = f * u - w * m;
    const double g = s * l + e * E + t * d;
    if (std::abs(g) < 1e-15) {
        return { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    }
    return {
        l / g, -(e * c - t * u) / g, (e * i - t * w) / g,
        E / g, (s * c - t * m) / g, -(s * i - t * f) / g,
        d / g, -(s * u - e * m) / g, (s * w - e * f) / g
    };
}

std::array<double, 9> SPAFarmTaskPlugin::adjust_homography(
    const std::array<double, 9>& H,
    const SimilarityFit& fit)
{
    // Exact 1:1 port of `wn(a, s)` in service.pretty.mjs lines 4551-4560
    const double a = fit.scale * fit.cos_a;
    const double b = -fit.scale * fit.sin_a;
    const double c = fit.tx;
    const double d = fit.scale * fit.sin_a;
    const double e = fit.scale * fit.cos_a;
    const double f = fit.ty;
    return {
        a * H[0] + b * H[3] + c * H[6],
        a * H[1] + b * H[4] + c * H[7],
        a * H[2] + b * H[5] + c * H[8],
        d * H[0] + e * H[3] + f * H[6],
        d * H[1] + e * H[4] + f * H[7],
        d * H[2] + e * H[5] + f * H[8],
        H[6],
        H[7],
        H[8]
    };
}

std::optional<std::tuple<double, double, int>> SPAFarmTaskPlugin::detect_marker_centroid(
    const cv::Mat& img,
    const std::array<double, 2>& ref_pt,
    int top_offset,
    std::string_view color_kind)
{
    // 1:1 port of `Un(a, s, e, t, f = 110)` in service.pretty.mjs L4444-4478.
    //
    // 旧实现错用了 goal-only 兜底分支的固定 ROI（[450,415,740,610] 等）并自拟了颜色阈值。
    // 1920x1080 实机实测：蓝点 E 只统计到 170 px（门限 0.5*2568=1284），必然被丢弃，
    // 只剩两个红门参与拟合，角度偏差 -8.8° 被判失败 -> 整段部署被静默跳过。
    // 此处按原实现重写：
    //   ROI    = 参考质心 +/- 110（E 的上边界再收 60）
    //   排除区 = di（顶部信息条、右侧面板）
    //   颜色   = qn / hi
    if (img.empty()) {
        return std::nullopt;
    }
    constexpr int kHalf = 110;
    static constexpr std::array<std::array<int, 4>, 2> kExcludeZones = {
        std::array<int, 4> { 0, 0, 1920, 110 },
        std::array<int, 4> { 1680, 0, 1920, 1080 }
    };

    const int x0 = std::clamp(static_cast<int>(std::lround(ref_pt[0] - kHalf)), 0, img.cols);
    const int x1 = std::clamp(static_cast<int>(std::lround(ref_pt[0] + kHalf)), 0, img.cols);
    const int y0 = std::clamp(static_cast<int>(std::lround(ref_pt[1] - top_offset)), 0, img.rows);
    const int y1 = std::clamp(static_cast<int>(std::lround(ref_pt[1] + kHalf)), 0, img.rows);

    const bool is_blue = (color_kind == "blue");
    double sx = 0.0;
    double sy = 0.0;
    int n = 0;
    for (int y = y0; y < y1; ++y) {
        const auto* row = img.ptr<cv::Vec3b>(y);
        for (int x = x0; x < x1; ++x) {
            bool excluded = false;
            for (const auto& zone : kExcludeZones) {
                if (x >= zone[0] && y >= zone[1] && x < zone[2] && y < zone[3]) {
                    excluded = true;
                    break;
                }
            }
            if (excluded) {
                continue;
            }
            const int B = row[x][0];
            const int G = row[x][1];
            const int R = row[x][2];
            // `qn` / `hi` in service.pretty.mjs L4441-4442
            const bool match = is_blue
                ? (B > 220 && G > 140 && R < 150 && (B - R) > 90)
                : (R > 200 && G < 110 && B < 110 && (R - G) > 120);
            if (match) {
                ++n;
                sx += x;
                sy += y;
            }
        }
    }
    if (n == 0) {
        return std::nullopt;
    }
    return std::make_tuple(sx / n, sy / n, n);
}

SPAFarmTaskPlugin::SimilarityFit SPAFarmTaskPlugin::fit_similarity(
    const std::vector<std::pair<double, double>>& src_pts,
    const std::vector<std::pair<double, double>>& dst_pts)
{
    // Exact 1:1 port of `pn(a, s)` in service.pretty.mjs lines 4509-4549
    SimilarityFit res;
    const size_t n = std::min(src_pts.size(), dst_pts.size());
    if (n < 2) {
        return res;
    }
    double mx = 0.0, my = 0.0, nx = 0.0, ny = 0.0;
    for (size_t i = 0; i < n; ++i) {
        mx += src_pts[i].first;
        my += src_pts[i].second;
        nx += dst_pts[i].first;
        ny += dst_pts[i].second;
    }
    mx /= static_cast<double>(n);
    my /= static_cast<double>(n);
    nx /= static_cast<double>(n);
    ny /= static_cast<double>(n);

    double M = 0.0, C = 0.0, T = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const double dx = src_pts[i].first - mx;
        const double dy = src_pts[i].second - my;
        const double ux = dst_pts[i].first - nx;
        const double uy = dst_pts[i].second - ny;
        M += dx * dx + dy * dy;
        C += dx * ux + dy * uy;
        T += dx * uy - dy * ux;
    }
    if (M < 1e-6) {
        return res;
    }
    const double scale = std::hypot(C, T) / M;
    if (scale < 1e-6) {
        return res;
    }
    const double cos_a = C / (M * scale);
    const double sin_a = T / (M * scale);
    const double tx = nx - scale * (cos_a * mx - sin_a * my);
    const double ty = ny - scale * (sin_a * mx + cos_a * my);

    double max_err = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const double px = scale * (cos_a * src_pts[i].first - sin_a * src_pts[i].second) + tx;
        const double py = scale * (sin_a * src_pts[i].first + cos_a * src_pts[i].second) + ty;
        max_err = std::max(max_err, std::hypot(px - dst_pts[i].first, py - dst_pts[i].second));
    }

    res.valid = true;
    res.matched_markers = static_cast<int>(n);
    res.scale = scale;
    res.cos_a = cos_a;
    res.sin_a = sin_a;
    res.tx = tx;
    res.ty = ty;
    res.err = max_err;
    res.angle_deg = std::atan2(sin_a, cos_a) * 180.0 / 3.14159265358979323846;
    return res;
}

SPAFarmTaskPlugin::CameraDetectResult SPAFarmTaskPlugin::detect_camera(
    const cv::Mat& img,
    bool allow_goal_only) const
{
    // 1:1 port of `Hn` (service.pretty.mjs L4526-4557) + `Sr` (L4584-4684).
    //
    // 旧实现与原实现的三处偏离，均已改正：
    //   1. marker 可用条件严格等于 `n >= 0.5 * markerRefN`（`Hn` L4531）；JS 没有 150 的下限，
    //      也没有 [0.35, 2.5] 区间，也没有 allow_goal_only 时把 E 放宽到 0.2 的特例；
    //   2. 三个 marker 都可用且残差 > 15 时，剔除残差最大的一个再重拟合；
    //   3. 接受条件是 `|scale - 1| <= 0.025` 且 `|平移| <= 45px`
    //      （不是 scale in [0.85,1.15] + |angle| <= 8 度 + 残差 <= 10）；
    //      两个镜头都评估，取 |scale - 1| 最小的那个。
    struct MarkerSlot
    {
        std::string_view id;
        std::array<double, 2> ref = { 0.0, 0.0 };
        int ref_n = 0;
        int top = 110;
        bool blue = false;
    };

    auto eval_calib = [&](const CameraCalibData& calib) -> CameraDetectResult {
        CameraDetectResult out;
        out.camera = calib.camera;
        const MarkerSlot slots[3] = {
            { "E", calib.marker_E, calib.ref_n_E, 60, true },
            { "S1", calib.marker_S1, calib.ref_n_S1, 110, false },
            { "S2", calib.marker_S2, calib.ref_n_S2, 110, false }
        };

        std::vector<std::pair<double, double>> src_pts;
        std::vector<std::pair<double, double>> dst_pts;
        std::string detail = calib.camera + "[";

        for (const auto& slot : slots) {
            auto det = detect_marker_centroid(img, slot.ref, slot.top, slot.blue ? "blue" : "red");
            const int n = det ? std::get<2>(*det) : 0;
            const double need = 0.5 * static_cast<double>(slot.ref_n); // `Hn` L4531
            const bool usable = det.has_value() && static_cast<double>(n) >= need;
            detail += std::string(slot.id) + "=";
            if (det) {
                detail += "(" + std::to_string(static_cast<int>(std::lround(std::get<0>(*det)))) + ","
                        + std::to_string(static_cast<int>(std::lround(std::get<1>(*det)))) + ")n"
                        + std::to_string(n) + "/" + std::to_string(static_cast<int>(std::lround(need)));
            }
            else {
                detail += "n0/" + std::to_string(static_cast<int>(std::lround(need)));
            }
            detail += usable ? " OK " : " XX ";
            if (usable) {
                src_pts.emplace_back(slot.ref[0], slot.ref[1]);
                dst_pts.emplace_back(std::get<0>(*det), std::get<1>(*det));
            }
        }

        if (src_pts.size() < 2) {
            out.detail = detail + "] 可用标记不足(" + std::to_string(src_pts.size()) + ")";
            return out;
        }

        SimilarityFit fit = fit_similarity(src_pts, dst_pts);
        if (src_pts.size() >= 3 && fit.err > 15.0) {
            size_t worst = 0;
            double worst_err = -1.0;
            for (size_t i = 0; i < src_pts.size(); ++i) {
                const double px = fit.scale * (fit.cos_a * src_pts[i].first - fit.sin_a * src_pts[i].second) + fit.tx;
                const double py = fit.scale * (fit.sin_a * src_pts[i].first + fit.cos_a * src_pts[i].second) + fit.ty;
                const double err_i = std::hypot(px - dst_pts[i].first, py - dst_pts[i].second);
                if (err_i > worst_err) {
                    worst_err = err_i;
                    worst = i;
                }
            }
            const auto at = static_cast<std::ptrdiff_t>(worst);
            src_pts.erase(src_pts.begin() + at);
            dst_pts.erase(dst_pts.begin() + at);
            fit = fit_similarity(src_pts, dst_pts);
            detail += "剔除残差点 ";
        }

        if (!fit.valid) {
            out.detail = detail + "] 拟合失败";
            return out;
        }

        out.fit = fit;
        out.H = adjust_homography(calib.base_H, fit);
        out.detail = detail + "] 用" + std::to_string(fit.matched_markers) + "点 scale="
            + std::to_string(fit.scale) + " 平移="
            + std::to_string(static_cast<int>(std::lround(std::hypot(fit.tx, fit.ty)))) + " 残差="
            + std::to_string(static_cast<int>(std::lround(fit.err)));
        return out;
    };

    CameraDetectResult exp_res = eval_calib(m_calib_expanded);
    CameraDetectResult col_res = eval_calib(m_calib_collapsed);

    const CameraDetectResult* best = nullptr;
    for (const auto* cand : { &exp_res, &col_res }) {
        if (!cand->fit.valid) {
            continue;
        }
        if (best == nullptr || std::abs(cand->fit.scale - 1.0) < std::abs(best->fit.scale - 1.0)) {
            best = cand;
        }
    }

    CameraDetectResult out;
    const std::string all = exp_res.detail + " || " + col_res.detail;

    // `Sr` 的 goal-only 兜底分支（service.pretty.mjs L4591-4664）：
    // 主路径失败时，用蓝色出生点的连通域像素块重做一次相似变换。
    if (allow_goal_only) {
        const bool need_fallback = (best == nullptr)
            || std::abs(best->fit.scale - 1.0) > 0.025
            || std::hypot(best->fit.tx, best->fit.ty) > 45.0;
        if (need_fallback) {
            if (auto fb = detect_goal_only_fallback(img); fb.has_value()) {
                fb->detail = all + " -> goal-only " + fb->camera;
                return *fb;
            }
        }
    }

    if (best == nullptr) {
        out.detail = all + " -> 找不到蓝点/红门标记";
        return out;
    }

    const double dev = std::abs(best->fit.scale - 1.0);
    const double shift = std::hypot(best->fit.tx, best->fit.ty);
    if (dev > 0.025 || shift > 45.0) {
        out.detail = all + " -> 与所有已标定镜头都不符";
        return out;
    }

    out.camera = best->camera;
    out.H = best->H;
    out.fit = best->fit;
    out.detail = all + " -> " + best->camera;
    if (allow_goal_only) {
        out.detail += " (goal-only)";
    }
    return out;
}

std::optional<SPAFarmTaskPlugin::CameraDetectResult> SPAFarmTaskPlugin::detect_goal_only_fallback(
    const cv::Mat& img) const
{
    // 1:1 port of the `allowGoalOnly` branch inside `Sr` (service.pretty.mjs L4591-4664).
    //
    // 只在「Boss 回合 / 最后一回合」且本局地图不是默认图时启用：此时画面可能只剩出生点
    // 附近的棋盘，红门不在视野内。做法是在固定 ROI 里取蓝色像素，按 5x5 邻域做连通域标记，
    // 丢掉碎块，用保留像素的 1% / 99% 分位点围成的矩形与标定 box 拟合相似变换。
    struct GoalOnlySpec
    {
        std::string_view camera;
        std::array<int, 4> roi;
        std::array<double, 4> box;
    };
    static constexpr GoalOnlySpec kSpecs[2] = {
        { "expanded",
          std::array<int, 4> { 450, 415, 740, 610 },
          std::array<double, 4> { 493.8044, 430.6554, 621.3322, 560.1759 } },
        { "collapsed",
          std::array<int, 4> { 410, 520, 690, 770 },
          std::array<double, 4> { 459.5028, 544.5004, 615.0266, 700.0242 } }
    };

    if (img.empty()) {
        return std::nullopt;
    }

    for (const auto& spec : kSpecs) {
        const int roi_x0 = spec.roi[0];
        const int roi_y0 = spec.roi[1];
        const int roi_w = spec.roi[2] - spec.roi[0];
        const int roi_h = spec.roi[3] - spec.roi[1];
        if (roi_w <= 0 || roi_h <= 0
            || spec.roi[3] > img.rows || spec.roi[2] > img.cols) {
            continue;
        }

        const size_t total = static_cast<size_t>(roi_w) * static_cast<size_t>(roi_h);
        std::vector<unsigned char> mask(total, 0);
        int blue_count = 0;
        for (int row = 0; row < roi_h; ++row) {
            const auto* src = img.ptr<cv::Vec3b>(roi_y0 + row);
            for (int col = 0; col < roi_w; ++col) {
                const int B = src[roi_x0 + col][0];
                const int G = src[roi_x0 + col][1];
                const int R = src[roi_x0 + col][2];
                if (B > 220 && G > 140 && R < 150 && (B - R) > 90) {
                    mask[static_cast<size_t>(row) * static_cast<size_t>(roi_w) + static_cast<size_t>(col)] = 1;
                    ++blue_count;
                }
            }
        }
        if (blue_count == 0) {
            continue;
        }

        std::vector<int> labels(total, 0);
        std::vector<unsigned char> keep; // 按 label 索引，0 = 背景
        keep.push_back(0);
        int next_label = 0;
        std::vector<int> stack;
        for (size_t seed = 0; seed < total; ++seed) {
            if (mask[seed] == 0 || labels[seed] != 0) {
                continue;
            }
            ++next_label;
            keep.push_back(0);
            labels[seed] = next_label;
            stack.clear();
            stack.push_back(static_cast<int>(seed));
            int min_c = roi_w;
            int max_c = 0;
            int min_r = roi_h;
            int max_r = 0;
            while (!stack.empty()) {
                const int cur = stack.back();
                stack.pop_back();
                const int col = cur % roi_w;
                const int row = (cur - col) / roi_w;
                min_c = std::min(min_c, col);
                max_c = std::max(max_c, col);
                min_r = std::min(min_r, row);
                max_r = std::max(max_r, row);
                for (int dr = -2; dr <= 2; ++dr) {
                    for (int dc = -2; dc <= 2; ++dc) {
                        const int nc = col + dc;
                        const int nr = row + dr;
                        if (nc < 0 || nr < 0 || nc >= roi_w || nr >= roi_h) {
                            continue;
                        }
                        const int ni = nr * roi_w + nc;
                        if (mask[ni] != 0 && labels[ni] == 0) {
                            labels[ni] = next_label;
                            stack.push_back(ni);
                        }
                    }
                }
            }
            keep[static_cast<size_t>(next_label)] =
                (max_c - min_c > 48 || max_r - min_r > 48) ? 1 : 0;
        }

        std::vector<int> xs;
        std::vector<int> ys;
        for (int row = 0; row < roi_h; ++row) {
            for (int col = 0; col < roi_w; ++col) {
                const size_t idx = static_cast<size_t>(row) * static_cast<size_t>(roi_w)
                    + static_cast<size_t>(col);
                const int lb = labels[idx];
                if (lb > 0 && keep[static_cast<size_t>(lb)] != 0) {
                    xs.push_back(col + roi_x0);
                    ys.push_back(row + roi_y0);
                }
            }
        }
        if (xs.size() < 1000 || xs.size() > 7000) {
            continue;
        }
        std::sort(xs.begin(), xs.end());
        std::sort(ys.begin(), ys.end());

        const size_t lo_x = static_cast<size_t>(static_cast<double>(xs.size()) * 0.01);
        const size_t hi_x = static_cast<size_t>(static_cast<double>(xs.size()) * 0.99);
        const size_t lo_y = static_cast<size_t>(static_cast<double>(ys.size()) * 0.01);
        const size_t hi_y = static_cast<size_t>(static_cast<double>(ys.size()) * 0.99);
        const std::array<double, 4> box_pts = {
            static_cast<double>(xs[lo_x]),
            static_cast<double>(ys[lo_y]),
            static_cast<double>(xs[hi_x]),
            static_cast<double>(ys[hi_y])
        };

        const double sx = (box_pts[2] - box_pts[0]) / (spec.box[2] - spec.box[0]);
        const double sy = (box_pts[3] - box_pts[1]) / (spec.box[3] - spec.box[1]);
        if (std::abs(sx - 1.0) > 0.04 || std::abs(sy - 1.0) > 0.04 || std::abs(sx - sy) > 0.04) {
            continue;
        }

        const std::vector<std::pair<double, double>> src_pts = {
            { spec.box[0], spec.box[1] },
            { spec.box[2], spec.box[3] }
        };
        const std::vector<std::pair<double, double>> dst_pts = {
            { box_pts[0], box_pts[1] },
            { box_pts[2], box_pts[3] }
        };
        const SimilarityFit fit = fit_similarity(src_pts, dst_pts);
        if (!fit.valid) {
            continue;
        }
        if (std::abs(fit.scale - 1.0) <= 0.025 && std::abs(fit.tx) <= 85.0
            && std::abs(fit.ty) <= 45.0 && fit.err <= 4.0) {
            const CameraCalibData& base =
                (spec.camera == "collapsed") ? m_calib_collapsed : m_calib_expanded;
            CameraDetectResult out;
            out.camera = std::string(spec.camera);
            out.fit = fit;
            out.fit.matched_markers = 0;
            out.H = adjust_homography(base.base_H, fit);
            return out;
        }
    }
    return std::nullopt;
}

bool SPAFarmTaskPlugin::goal_only_allowed() const
{
    // `(!!info.boss || round === lastRound) && !!stageSource && stageSource !== "default"`
    // `turns` 表共 9 回合，第 9 回合为 Boss 回合（resource/SPAFarm/farm-rules.json）。
    constexpr int kLastRound = 9;
    const bool boss_round = !m_current_boss.empty() || m_current_round == kLastRound; // `N(I) = I === lastRound`
    return boss_round && !m_stage_source.empty() && m_stage_source != "default";
}

std::array<double, 9> SPAFarmTaskPlugin::active_board_homography(const CameraDetectResult& cam) const
{
    if (cam.camera == "collapsed") {
        return adjust_homography(m_collapsed_board_H, cam.fit);
    }
    return cam.H;
}

std::optional<SPAFarmTaskPlugin::StableBoardState> SPAFarmTaskPlugin::wait_stable_board(
    int timeout_ms,
    const std::optional<std::array<double, 4>>& prev_pts)
{
    // Exact 1:1 port of `Dt(j = 5e3, re = null)` in service.pretty.mjs lines 7363-7385:
    // Loops taking screenshots until `!is_detail_panel_open(Me)`, detects camera `Sr`,
    // projects board corners `(2.5, 1.5)` and `(9.5, 6.5)`, and requires 2 consecutive frames
    // to have all 4 coordinates within `< 4.0` px displacement. Returns `std::nullopt` on timeout.
    const auto t0 = std::chrono::steady_clock::now();
    std::optional<std::array<double, 4>> ce = prev_pts;
    while (!need_exit()) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - t0)
                                 .count();
        if (elapsed >= timeout_ms) {
            break;
        }
        cv::Mat Me = shot();
        if (!Me.empty() && !is_detail_panel_open(Me)) {
            const bool allow_goal_only = goal_only_allowed();
            CameraDetectResult we = detect_camera(Me, allow_goal_only);
            if (!we.camera.empty()) {
                auto [x1, y1] = project_point(we.H, 2.5, 1.5);
                auto [x2, y2] = project_point(we.H, 9.5, 6.5);
                std::array<double, 4> je = { x1, y1, x2, y2 };
                if (ce.has_value()) {
                    bool stable = true;
                    for (size_t i = 0; i < 4; ++i) {
                        if (std::abs(je[i] - (*ce)[i]) >= 4.0) {
                            stable = false;
                            break;
                        }
                    }
                    if (stable) {
                        return StableBoardState {
                            active_board_homography(we),
                            we.H,
                            we.camera,
                            we.fit,
                            je,
                            Me
                        };
                    }
                }
                ce = je;
            }
            else {
                ce.reset();
            }
        }
        sleep_ms(static_cast<int>(std::lround(100.0 * m_step_scale)));
    }
    return std::nullopt;
}

// ============================================================================
// Stage Identification & Info Screen (`Ri`, `ji`, `Ni`, `ki`, `lr`, `F`)
// ============================================================================

std::unordered_map<std::string, std::vector<double>> SPAFarmTaskPlugin::sample_board_cells(
    const cv::Mat& img,
    const std::array<double, 9>& H) const
{
    // Exact 1:1 port of `Ri(a, s)` in service.pretty.mjs lines 5539-5568
    static constexpr double kOffsets[8][2] = {
        { -0.3, -0.3 }, { 0.0, -0.3 }, { 0.3, -0.3 },
        { -0.3, 0.0 },                 { 0.3, 0.0 },
        { -0.3, 0.3 },  { 0.0, 0.3 },  { 0.3, 0.3 }
    };
    std::unordered_map<std::string, std::vector<double>> out;
    if (img.empty()) {
        return out;
    }
    for (int r = 1; r <= 4; ++r) {
        for (int c = 0; c < 13; ++c) {
            std::vector<double> vec;
            vec.reserve(24);
            for (const auto& off : kOffsets) {
                auto [px, py] = project_point(H, c + 0.5 + off[0], r + 0.5 + off[1]);
                double sr = 0.0, sg = 0.0, sb = 0.0;
                int cnt = 0;
                for (int dy = -3; dy <= 3; ++dy) {
                    for (int dx = -3; dx <= 3; ++dx) {
                        int x = static_cast<int>(std::lround(px + dx));
                        int y = static_cast<int>(std::lround(py + dy));
                        if (x < 0 || y < 0 || x >= img.cols || y >= img.rows) {
                            continue;
                        }
                        const auto& p = img.at<cv::Vec3b>(y, x);
                        const int B = p[0], G = p[1], R = p[2];
                        if ((R > 200 && G < 80 && B > 200)
                            || (R < 80 && G > 200 && B > 200)
                            || (R + G + B < 30)) {
                            continue;
                        }
                        sr += R;
                        sg += G;
                        sb += B;
                        ++cnt;
                    }
                }
                if (cnt > 0) {
                    vec.push_back(sr / cnt);
                    vec.push_back(sg / cnt);
                    vec.push_back(sb / cnt);
                }
                else {
                    vec.push_back(std::numeric_limits<double>::quiet_NaN());
                    vec.push_back(std::numeric_limits<double>::quiet_NaN());
                    vec.push_back(std::numeric_limits<double>::quiet_NaN());
                }
            }
            out[std::to_string(r) + "," + std::to_string(c)] = std::move(vec);
        }
    }
    return out;
}

void SPAFarmTaskPlugin::update_stage_from_board(
    const cv::Mat& img,
    const std::array<double, 9>& H,
    std::string_view camera,
    const std::set<std::string>& occupied_cells)
{
    // Exact 1:1 port of `ji` + `Ni` + `Tt` in service.pretty.mjs lines 5570-5596 & 7097-7128
    if (m_stage_source == "thumbnail" || m_board_refs.empty()) {
        return;
    }
    auto sampled = sample_board_cells(img, H);
    struct CandScore
    {
        std::string stage_id;
        double score = 1e9;
    };
    std::vector<CandScore> scores;
    for (const auto& entry : m_board_refs) {
        if (entry.camera != camera) {
            continue;
        }
        std::vector<double> cell_dists;
        for (const auto& [cell_key, sample_vec] : sampled) {
            if (occupied_cells.count(cell_key) > 0) {
                continue;
            }
            auto it = entry.cells.find(cell_key);
            if (it == entry.cells.end()) {
                continue;
            }
            const auto& ref_vec = it->second;
            std::vector<double> pt_dists;
            for (size_t i = 0; i + 2 < sample_vec.size() && i + 2 < ref_vec.size(); i += 3) {
                if (std::isnan(sample_vec[i]) || std::isnan(ref_vec[i])) {
                    continue;
                }
                const double dr = sample_vec[i] - ref_vec[i];
                const double dg = sample_vec[i + 1] - ref_vec[i + 1];
                const double db = sample_vec[i + 2] - ref_vec[i + 2];
                pt_dists.push_back(std::sqrt(dr * dr + dg * dg + db * db));
            }
            if (!pt_dists.empty()) {
                std::sort(pt_dists.begin(), pt_dists.end());
                cell_dists.push_back(pt_dists[pt_dists.size() / 2]);
            }
        }
        if (!cell_dists.empty()) {
            std::sort(cell_dists.begin(), cell_dists.end());
            scores.push_back(CandScore { entry.stage_id, cell_dists[cell_dists.size() / 2] });
        }
    }
    if (scores.empty()) {
        return;
    }
    std::sort(scores.begin(), scores.end(), [](const CandScore& a, const CandScore& b) {
        return a.score < b.score;
    });
    const bool confident = (scores[0].score < 20.0)
        && (scores.size() < 2 || (scores[1].score - scores[0].score) >= 6.0);
    if (!confident) {
        m_stage_vote = { "", 0 };
        return;
    }
    if (m_stage_vote.first == scores[0].stage_id) {
        m_stage_vote.second++;
    }
    else {
        m_stage_vote = { scores[0].stage_id, 1 };
    }
    if (scores[0].stage_id == m_stage_id) {
        if (m_stage_source == "default") {
            m_stage_source = "board";
        }
        return;
    }
    if (m_stage_vote.second >= 2) {
        m_stage_id = scores[0].stage_id;
        m_stage_source = "board";
        m_empty_cell_refs.clear();
        log_info("识别到当前关卡地图: " + m_stage_id);
    }
}

std::optional<SPAFarmTaskPlugin::InfoScreenData> SPAFarmTaskPlugin::read_info_screen(const cv::Mat& img) const
{
    // Exact 1:1 port of `ki` in service.pretty.mjs lines 5404-5492
    if (img.empty()) {
        return std::nullopt;
    }
    auto left_items = ocr(img, std::array<int, 4> { 0, 160, 1350, 860 });
    auto right_items = ocr(img, std::array<int, 4> { 1340, 400, 580, 460 });

    InfoScreenData out;
    for (const auto& it : left_items) {
        auto matches = match_dict(it.text, m_bond_dict);
        if (matches.empty() || match_coverage(it.text, matches) < 0.6) {
            continue;
        }
        for (const auto& m : matches) {
            double mid_x = it.x + it.w * (m.start + m.end) * 0.5 / std::max(1, m.len);
            double gr = green_hp_bar_ratio(img, mid_x, it.y, it.h);
            bool exists = false;
            for (const auto& b : out.bonds) {
                if (b.name == m.name) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                const auto* binfo = find_bond(m.name);
                out.bonds.push_back(InfoScreenData::BondStatus {
                    m.name,
                    gr >= 0.15,
                    gr,
                    binfo ? binfo->is_core : false });
            }
        }
    }

    auto leader_hit = find_ocr(right_items, { "敌方领袖" });
    if (leader_hit) {
        std::string boss_concat;
        for (const auto& it : right_items) {
            if (it.y > leader_hit->y + leader_hit->h * 0.5 && it.x >= leader_hit->x - 120) {
                if (it.text.find("特训") != std::string::npos) {
                    for (const auto& et : m_enemy_types) {
                        if (it.text.find(et.name) != std::string::npos) {
                            out.enemy_types.push_back(et.name);
                        }
                    }
                }
                else if (out.enemy_types.empty()) {
                    boss_concat += it.text;
                }
            }
        }
        auto boss_matches = match_dict(boss_concat, m_boss_dict);
        if (!boss_matches.empty()) {
            out.boss = boss_matches.front().name;
        }
    }

    if (out.bonds.empty() && !leader_hit.has_value()) {
        return std::nullopt;
    }
    return out;
}

std::pair<std::optional<int>, bool> SPAFarmTaskPlugin::read_round_and_prep(
    const cv::Mat& img,
    const std::vector<OcrItem>& full_items) const
{
    // Exact 1:1 port of `lr(j)` in service.pretty.mjs lines 7214-7264
    std::optional<int> round_num;
    auto parse_round_from = [&](const std::vector<OcrItem>& items) -> std::optional<int> {
        for (const auto& it : items) {
            auto pos = it.text.find("回合");
            if (pos != std::string::npos) {
                std::string d = extract_digits_mapped(it.text.substr(pos));
                if (!d.empty()) {
                    int v = std::stoi(d);
                    if (v >= 1 && v <= 30) {
                        return v;
                    }
                }
            }
        }
        for (const auto& it : items) {
            std::string d = extract_digits_mapped(it.text);
            if (!d.empty() && d.size() <= 2 && it.score >= 0.7) {
                int v = std::stoi(d);
                if (v >= 1 && v <= 30) {
                    return v;
                }
            }
        }
        return std::nullopt;
    };

    auto r1 = ocr(img, std::array<int, 4> { 690, 15, 130, 75 });
    round_num = parse_round_from(r1);
    if (!round_num) {
        auto r2 = ocr_rec_only(img, { 690, 15, 130, 75 }, 3, 0);
        round_num = parse_round_from(r2);
    }
    if (!round_num) {
        round_num = parse_round_from(full_items);
    }

    auto phase_items = ocr(img, std::array<int, 4> { 900, 15, 210, 75 });
    bool is_prep = has_ocr(phase_items, { "休息", "休整" })
        || (has_ocr(full_items, { "准备就绪" })
            && has_ocr(full_items, { "刷新", "升级", "已满级", "满级", "收起", "展开" }));
    return { round_num, is_prep };
}

std::optional<int> SPAFarmTaskPlugin::read_remaining_deploy(
    const cv::Mat& img,
    const std::vector<OcrItem>& items) const
{
    // `Jn(a)` L4817-4825 + `Xn(a)` L4762-4785 —— 先走 OCR 回读，失败再退回纯文本解析
    auto to_digit = [](const std::string& s) -> std::optional<int> {
        if (s.size() != 1) {
            return std::nullopt;
        }
        const char c = s[0];
        if (c >= '0' && c <= '8') {
            return c - '0';
        }
        if (c == 'O' || c == 'o' || c == '0') {
            return 0;
        }
        return std::nullopt;
    };

    // ---- Jn ----
    for (const auto& it : items) {
        const std::string t = strip_ws(it.text);
        if (t.find("剩余可放置") == std::string::npos) {
            continue;
        }
        const int w = std::max(0, static_cast<int>(std::lround(static_cast<double>(it.x) - 10.0)));
        const int y0 = std::max(0, static_cast<int>(std::lround(static_cast<double>(it.y) - 4.0)));
        const int width = 1918 - w;
        const int height = static_cast<int>(std::lround(static_cast<double>(it.h) + 8.0));
        cv::Mat crop = img_crop(img, w, y0, width, height);
        if (crop.empty()) {
            continue;
        }
        cv::Mat work = img_resize_cubic(img_gray(crop), width * 4, height * 4);
        work = img_extend_black(work, 40, 40, 40, 40);
        auto rec = ocr_rec_only(work, { 0, 0, work.cols, work.rows }, 1, 0);
        std::string joined;
        for (const auto& r : rec) {
            joined += strip_ws(r.text);
        }
        // /剩余可放置角色[:：]?([0-8OoＯ〇])/
        const std::string key = "剩余可放置角色";
        const size_t pos = joined.find(key);
        if (pos != std::string::npos) {
            size_t i = pos + key.size();
            if (i < joined.size() && (joined[i] == ':' || joined[i] == '：')) {
                ++i;
            }
            if (i < joined.size()) {
                const std::string one = joined.substr(i, 1);
                if (one == "Ｏ" || one == "〇") {
                    return 0;
                }
                if (auto d = to_digit(one)) {
                    return d;
                }
            }
        }
        break;
    }

    // ---- Xn(a.texts) ----
    for (const auto& it : items) {
        const std::string t = strip_ws(it.text);
        const std::string key = "剩余可放置角色";
        if (t.rfind(key, 0) != 0) {
            continue;
        }
        std::string rest = t.substr(key.size());
        if (!rest.empty() && (rest[0] == ':' || rest[0] == '：')) {
            rest = rest.substr(1);
        }
        if (rest.size() == 1) {
            if (rest == "Ｏ" || rest == "〇") {
                return 0;
            }
            if (auto d = to_digit(rest)) {
                return d;
            }
            continue;
        }
        if (!rest.empty()) {
            continue;
        }
        // 标签与数字分开识别：数字在标签右边 40..60px 内，且垂直居中对齐
        for (const auto& other : items) {
            if (&other == &it) {
                continue;
            }
            if (other.x < it.x + it.w - 40 || other.x > it.x + it.w + 60) {
                continue;
            }
            if (std::abs((other.y + other.h * 0.5) - (it.y + it.h * 0.5)) >= it.h * 0.6) {
                continue;
            }
            const std::string nt = strip_ws(other.text);
            if (nt.size() != 1) {
                continue;
            }
            if (nt == "Ｏ" || nt == "〇") {
                return 0;
            }
            if (auto d = to_digit(nt)) {
                return d;
            }
        }
    }
    return std::nullopt;
}

// ============================================================================
// Shop Reader (`Kr`, `Kn`, `Rt`, `Bi`, `Ci`, `Di`, `Mt`)
// ============================================================================

std::optional<int> SPAFarmTaskPlugin::read_shop_digits_4x(
    const cv::Mat& img,
    const std::array<int, 4>& roi_xywh) const
{
    // `Kr(a, s)` —— service.pretty.mjs L4817-4825
    //   [e,t,f,w] = s.map(round)
    //   i = extract(...).resize(f*4, w*4)
    //   m = yr(i, [0,0,f*4,w*4])
    //        .filter(u => u.score > .7 || (u.score > .55 && /^[0OoＯ〇]$/.test(u.text.trim())))
    //        .map(u => u.text.replace(/\s/g,"").replace(/^[OoＯ〇]$/,"0")).join("")
    //   return /^\d{1,3}$/.test(m) ? Number(m) : null
    const int x = static_cast<int>(std::lround(static_cast<double>(roi_xywh[0])));
    const int y = static_cast<int>(std::lround(static_cast<double>(roi_xywh[1])));
    const int w = static_cast<int>(std::lround(static_cast<double>(roi_xywh[2])));
    const int h = static_cast<int>(std::lround(static_cast<double>(roi_xywh[3])));
    cv::Mat crop = img_crop(img, x, y, w, h);
    if (crop.empty()) {
        return std::nullopt;
    }
    cv::Mat big = img_resize_cubic(crop, crop.cols * 4, crop.rows * 4);
    auto items = ocr_rec_only(big, { 0, 0, big.cols, big.rows }, 1, 0);

    std::string joined;
    for (const auto& it : items) {
        const std::string t = strip_ws(it.text);
        const bool single_zero = (t == "O" || t == "o" || t == "Ｏ" || t == "〇");
        if (!(it.score > 0.7 || (it.score > 0.55 && single_zero))) {
            continue;
        }
        joined += single_zero ? std::string("0") : t;
    }
    if (joined.empty() || joined.size() > 3) {
        return std::nullopt;
    }
    for (char c : joined) {
        if (c < '0' || c > '9') {
            return std::nullopt;
        }
    }
    return std::stoi(joined);
}

std::optional<int> SPAFarmTaskPlugin::recognize_shop_level_roman_cc(const cv::Mat& img, int up_x)
{
    // `Kn(a, s)` —— service.pretty.mjs L4945-5032，逐行对齐
    //   f = round(s-55); extract({left:f, top:845, width:110, height:100}).greyscale().raw()
    //   Otsu 求阈值 d；g = count(pixel > d)；n = g < len/2；n && (d = max(d,140))
    //   y = X => n ? i[X] > d : i[X] <= d
    //   4 邻域连通域，过滤 x0>1 && y0>1 && x1<108 && y1<98 && (y1-y0) >= (n?28:20) && 像素数>=80
    //   按 x0 排序；空或 >3 → {null,null}
    //   T(X): 统计 y0+2 .. y0+round((y1-y0)*.3) 里“至少两段游程”的行数，>=3 记为 V
    //   拼成 I/II/... 查表
    constexpr int W = 110;
    constexpr int H = 100;
    constexpr int TOP = 845;

    const int left = static_cast<int>(std::lround(static_cast<double>(up_x) - 55.0));
    cv::Mat crop = img_crop(img, left, TOP, W, H);
    if (crop.empty()) {
        return std::nullopt;
    }
    cv::Mat gray = img_gray(crop);
    if (gray.cols != W || gray.rows != H) {
        cv::resize(gray, gray, cv::Size(W, H), 0.0, 0.0, cv::INTER_AREA);
    }

    std::vector<int> pix(static_cast<size_t>(W) * H);
    int hist[256] = { 0 };
    for (int y = 0; y < H; ++y) {
        const auto* row = gray.ptr<uint8_t>(y);
        for (int x = 0; x < W; ++x) {
            const int v = row[x];
            pix[static_cast<size_t>(y) * W + x] = v;
            ++hist[v];
        }
    }
    const int total = W * H;

    // Otsu（与 JS 的类间方差写法一致）
    double sum_all = 0.0;
    for (int i = 0; i < 256; ++i) {
        sum_all += static_cast<double>(i) * hist[i];
    }
    double sum_b = 0.0;
    int w_b = 0;
    double max_var = 0.0;
    int thr = 128;
    for (int t = 0; t < 256; ++t) {
        w_b += hist[t];
        if (w_b == 0) {
            continue;
        }
        const int w_f = total - w_b;
        if (w_f == 0) {
            break;
        }
        sum_b += static_cast<double>(t) * hist[t];
        const double m_b = sum_b / w_b;
        const double m_f = (sum_all - sum_b) / w_f;
        const double var = static_cast<double>(w_b) * w_f * (m_b - m_f) * (m_b - m_f);
        if (var > max_var) {
            max_var = var;
            thr = t;
        }
    }

    int bright = 0;
    for (int v : pix) {
        if (v > thr) {
            ++bright;
        }
    }
    const bool dark_fg = bright < total / 2;
    if (dark_fg) {
        thr = std::max(thr, 140);
    }
    auto is_fg = [&](int idx) {
        return dark_fg ? pix[static_cast<size_t>(idx)] > thr : pix[static_cast<size_t>(idx)] <= thr;
    };

    struct Comp
    {
        int x0 = W;
        int x1 = 0;
        int y0 = H;
        int y1 = 0;
        int n = 0;
    };

    std::vector<uint8_t> vis(static_cast<size_t>(W) * H, 0);
    std::vector<Comp> comps;
    std::vector<int> stack;
    for (int idx = 0; idx < total; ++idx) {
        if (!is_fg(idx) || vis[idx]) {
            continue;
        }
        Comp c;
        vis[idx] = 1;
        stack.clear();
        stack.push_back(idx);
        while (!stack.empty()) {
            const int cur = stack.back();
            stack.pop_back();
            const int q = cur % W;
            const int p = (cur - q) / W;
            ++c.n;
            c.x0 = std::min(c.x0, q);
            c.x1 = std::max(c.x1, q);
            c.y0 = std::min(c.y0, p);
            c.y1 = std::max(c.y1, p);
            for (int z : { -1, 1, -W, W }) {
                const int nxt = cur + z;
                if (nxt < 0 || nxt >= total) {
                    continue;
                }
                if (z == -1 && q == 0) {
                    continue;
                }
                if (z == 1 && q == W - 1) {
                    continue;
                }
                if (is_fg(nxt) && !vis[nxt]) {
                    vis[nxt] = 1;
                    stack.push_back(nxt);
                }
            }
        }
        comps.push_back(c);
    }

    std::vector<Comp> hits;
    for (const auto& c : comps) {
        if (c.x0 > 1 && c.y0 > 1 && c.x1 < 108 && c.y1 < 98
            && (c.y1 - c.y0) >= (dark_fg ? 28 : 20) && c.n >= 80) {
            hits.push_back(c);
        }
    }
    std::sort(hits.begin(), hits.end(), [](const Comp& a, const Comp& b) {
        return a.x0 < b.x0;
    });
    if (hits.empty() || hits.size() > 3) {
        return std::nullopt;
    }

    auto is_v = [&](const Comp& c) {
        int rows = 0;
        const int end = c.y0 + static_cast<int>(std::lround(static_cast<double>(c.y1 - c.y0) * 0.3));
        for (int y = c.y0 + 2; y <= end; ++y) {
            int runs = 0;
            bool prev = false;
            for (int x = c.x0; x <= c.x1; ++x) {
                const bool cur = is_fg(y * W + x);
                if (cur && !prev) {
                    ++runs;
                }
                prev = cur;
            }
            if (runs >= 2) {
                ++rows;
            }
        }
        return rows >= 3;
    };

    std::string name;
    for (const auto& c : hits) {
        name += is_v(c) ? "V" : "I";
    }

    static const std::unordered_map<std::string, int> kLevel = {
        { "I", 1 },   { "II", 2 },  { "III", 3 },
        { "IV", 4 },  { "V", 5 },   { "VI", 6 },
    };
    auto it = kLevel.find(name);
    if (it == kLevel.end()) {
        return std::nullopt;
    }
    return it->second;
}

SPAFarmTaskPlugin::ShopHeaderState SPAFarmTaskPlugin::read_shop_header(
    const cv::Mat& img,
    const std::vector<OcrItem>& full_items) const
{
    // `Rt(a)` —— service.pretty.mjs L4826-4890，逐行对齐（`pc` 恒为 false，见里程碑文件）
    //   gold  = Kr([1767,776,66,32]) ?? Kr([1782,777,36,30])
    //   f     = Te(/^升级$/)
    //   !f && !Te(/^已满级$/) && Te(/收起/) → 对 x ∈ {337,580} 试 Kn，命中则合成升级按钮 (E,985)
    //   !f → return {gold, upgradeCost:null, level: Te(/^已满级$/)?6:null}
    //   i = 升级按钮中心 x；m = Kr([i-13,786,28,30])；u = Kn(i)
    //   u.level 为空 → 取 [i-60,870,120,80] 放大 3x + 60 黑边做 rec，查 {IV,V,VI} 表
    ShopHeaderState st;

    st.gold = read_shop_digits_4x(img, { 1767, 776, 66, 32 });
    if (!st.gold.has_value()) {
        st.gold = read_shop_digits_4x(img, { 1782, 777, 36, 30 });
    }

    const bool max_level = has_text_regex(full_items, "^已满级$");
    std::optional<OcrItem> up = find_text_regex(full_items, "^升级$");

    if (!up.has_value() && !max_level && has_text_regex(full_items, "收起")) {
        for (int probe : { 337, 580 }) {
            if (recognize_shop_level_roman_cc(img, probe).has_value()) {
                // JS: f = {text:"升级", score:.5, box:[E-26, 985-15, 52, 30]}，中心即 (E, 985)
                OcrItem synth;
                synth.text = "升级";
                synth.score = 0.5;
                synth.w = 52;
                synth.h = 30;
                synth.x = probe - 26;
                synth.y = 985 - 15;
                synth.cx = probe;
                synth.cy = 985;
                up = synth;
                break;
            }
        }
    }

    if (!up.has_value()) {
        st.level = max_level ? std::optional<int>(6) : std::nullopt;
        return st;
    }

    const int btn_x = static_cast<int>(std::lround(up->cx));
    const int btn_y = static_cast<int>(std::lround(up->cy));
    st.upgrade_btn = std::pair<int, int> { btn_x, btn_y };
    st.upgrade_cost = read_shop_digits_4x(img, { btn_x - 13, 786, 28, 30 });

    auto strokes = recognize_shop_level_roman_cc(img, btn_x);
    if (strokes.has_value()) {
        st.level = strokes;
        st.rawLevel = "strokes:" + std::to_string(*strokes);
        return st;
    }

    cv::Mat crop = img_crop(img, btn_x - 60, 870, 120, 80);
    if (!crop.empty()) {
        crop = img_resize_cubic(crop, 360, 240);
        crop = img_extend_black(crop, 60, 60, 60, 60);
        auto items = ocr_rec_only(crop, { 0, 0, crop.cols, crop.rows }, 1, 0);
        std::string joined;
        for (const auto& it : items) {
            if (it.score > 0.6) {
                joined += strip_ws(it.text);
            }
        }
        st.rawLevel = joined;
        static const std::unordered_map<std::string, int> kRoman = {
            { "IV", 4 }, { "Ⅳ", 4 }, { "V", 5 }, { "Ⅴ", 5 }, { "VI", 6 }, { "Ⅵ", 6 },
        };
        auto it = kRoman.find(joined);
        if (it != kRoman.end()) {
            st.level = it->second;
        }
    }
    return st;
}

std::optional<SPAFarmTaskPlugin::CardGridFit> SPAFarmTaskPlugin::detect_card_gaps(const cv::Mat& img)
{
    // `Ci(a)` L5325-5351 + `Di(a)` L5352-5388，两段合并在一个函数里，避免多一次截图裁剪
    //   Ci: extract({0,800,1920,240}).grayscale() → 列求和 → ±2 均值 / height
    //   Di: 340..1719 找局部极大（±12 内无更大），且 (a[f-30]+a[f+30])/2 - a[f] > 18
    //       p ∈ [205,250] step 0.5，phase 遍历 gaps，votes/err 取最优（votes 大优先，其次 err 小）
    //       t 归一到 [330, 330+p)
    std::vector<double> prof;
    {
        cv::Mat crop = img_crop(img, 0, 800, 1920, 240);
        if (crop.empty()) {
            return std::nullopt;
        }
        cv::Mat gray = img_gray(crop);
        if (gray.cols != 1920 || gray.rows != 240) {
            cv::resize(gray, gray, cv::Size(1920, 240), 0.0, 0.0, cv::INTER_AREA);
        }
        std::vector<double> col(1920, 0.0);
        for (int y = 0; y < gray.rows; ++y) {
            const auto* row = gray.ptr<uint8_t>(y);
            for (int x = 0; x < 1920; ++x) {
                col[x] += row[x];
            }
        }
        prof.resize(1920);
        for (int x = 0; x < 1920; ++x) {
            double sum = 0.0;
            int cnt = 0;
            for (int d = -2; d <= 2; ++d) {
                const int idx = x + d;
                if (idx >= 0 && idx < 1920) {
                    sum += col[idx];
                    ++cnt;
                }
            }
            prof[x] = (cnt > 0) ? (sum / cnt / gray.rows) : 0.0;
        }
    }

    std::vector<int> gaps;
    for (int f = 340; f < 1720; ++f) {
        bool is_max = true;
        for (int i = -12; i <= 12; ++i) {
            const int idx = f + i;
            if (idx < 0 || idx >= 1920) {
                continue;
            }
            if (prof[idx] < prof[f]) {
                is_max = false;
                break;
            }
        }
        if (!is_max) {
            continue;
        }
        const int lo = std::max(0, f - 30);
        const int hi = std::min(1919, f + 30);
        if ((prof[lo] + prof[hi]) / 2.0 - prof[f] > 18.0) {
            gaps.push_back(f);
        }
    }
    if (gaps.empty()) {
        return std::nullopt;
    }

    struct Fit
    {
        double p = 0.0;
        int phase = 0;
        int votes = 0;
        double err = 0.0;
        bool valid = false;
    };
    Fit best;
    for (double p = 205.0; p <= 250.0; p += 0.5) {
        for (int w : gaps) {
            int votes = 0;
            double err = 0.0;
            for (int u : gaps) {
                const int c = static_cast<int>(std::lround((u - w) / p));
                const double diff = std::abs(w + c * p - u);
                if (diff <= 10.0) {
                    ++votes;
                    err += diff;
                }
            }
            if (votes < 2) {
                continue;
            }
            if (!best.valid || votes > best.votes || (votes == best.votes && err < best.err)) {
                best = Fit { p, w, votes, err, true };
            }
        }
    }
    if (!best.valid) {
        return std::nullopt;
    }

    double t = best.phase;
    while (t - best.p >= 330.0) {
        t -= best.p;
    }
    while (t < 330.0) {
        t += best.p;
    }
    CardGridFit out;
    out.p = best.p;
    out.first = static_cast<int>(std::lround(t));
    out.votes = best.votes;
    return out;
}

std::vector<SPAFarmTaskPlugin::ShopCard> SPAFarmTaskPlugin::read_shop_cards(const cv::Mat& img)
{
    // `Bi(a)` —— service.pretty.mjs L5155-5334，逐行对齐
    //   jt = {x:330, y:925, w:1330, h:135}, Ut = 2, Qr = 120
    //   d = extract(jt).resize(2660, 270, cubic)
    //   n = d.extend(120,120,0,0,#000)                              → 通道 1（原图）
    //   y = d.extractChannel("red").linear(5, -1.3*255).extend(...)  → 通道 2
    //   b = d.grayscale().clahe(24,24,6).extend(...)                 → 通道 3（懒加载）
    //   u(box) = {x: box0/2 + 330, y: (box1-120)/2 + 925, w: box2/2}
    //   c(list)：先按盟约匹配（覆盖率 >= len-1），否则按干员名匹配
    //   按 x 聚类（<90 同组）→ 用 Di 的卡位周期补空位 → 输出 name/kind/source
    const cv::Rect jt(330, 925, 1330, 135);
    constexpr int Ut = 2;
    constexpr int Qr = 120;

    if (img.empty()) {
        return {};
    }
    cv::Mat crop = img_crop(img, jt.x, jt.y, jt.width, jt.height);
    if (crop.empty()) {
        return {};
    }
    cv::Mat scaled = img_resize_cubic(crop, jt.width * Ut, jt.height * Ut);
    cv::Mat ch_orig = img_extend_black(scaled, Qr, Qr, 0, 0);
    cv::Mat ch_red = img_extend_black(img_red_linear(scaled, 5.0, -1.3 * 255.0), Qr, Qr, 0, 0);
    cv::Mat ch_clahe;

    // ---- 词典与映射 ----
    std::vector<std::string> all_names;
    all_names.reserve(m_operators.size() + m_items.size());
    std::unordered_map<std::string, std::string> norm_to_orig;
    std::unordered_set<std::string> operator_norms;
    for (const auto& op : m_operators) {
        all_names.push_back(op.name);
        const std::string n = strip_punct(op.name);
        norm_to_orig.emplace(n, op.name);
        operator_norms.insert(n);
    }
    for (const auto& it : m_items) {
        all_names.push_back(it.name);
        norm_to_orig.emplace(strip_punct(it.name), it.name);
    }
    std::vector<std::string> bond_names_raw;
    bond_names_raw.reserve(m_bonds.size());
    for (const auto& b : m_bonds) {
        bond_names_raw.push_back(b.name);
    }
    const std::vector<DictEntry> name_dict = build_dict(all_names);
    const std::vector<DictEntry> bond_dict = build_dict(bond_names_raw);

    // `i`：盟约组合 → 干员名
    std::unordered_map<std::string, std::vector<std::string>> bond_combo;
    for (const auto& op : m_operators) {
        std::vector<std::string> nb;
        nb.reserve(op.bond_names.size());
        for (const auto& b : op.bond_names) {
            nb.push_back(strip_punct(b));
        }
        std::sort(nb.begin(), nb.end());
        std::string key;
        for (size_t i = 0; i < nb.size(); ++i) {
            if (i > 0) {
                key += "+";
            }
            key += nb[i];
        }
        auto& vec = bond_combo[key];
        if (std::find(vec.begin(), vec.end(), op.name) == vec.end()) {
            vec.push_back(op.name);
        }
    }

    struct Hit
    {
        std::string kind; // "bond" / "name"
        std::string value;
        double x = 0.0;
        double y = 0.0;
    };

    struct LocalBox
    {
        double x = 0.0;
        double y = 0.0;
        double w = 0.0;
        double h = 0.0;
    };
    auto to_local = [&](const OcrItem& it) {
        return LocalBox {
            it.x / static_cast<double>(Ut) + jt.x,
            (it.y - Qr) / static_cast<double>(Ut) + jt.y,
            it.w / static_cast<double>(Ut),
            it.h / static_cast<double>(Ut),
        };
    };

    auto collect = [&](const std::vector<OcrItem>& items) {
        std::vector<Hit> hits;
        for (const auto& it : items) {
            const LocalBox box = to_local(it);
            const std::string norm = strip_punct(it.text);
            if (norm.empty()) {
                continue;
            }
            const double nlen = static_cast<double>(utf8_len(norm));

            const auto bond_matches = match_dict(norm, bond_dict, false);
            size_t covered = 0;
            for (const auto& b : bond_matches) {
                covered += static_cast<size_t>(b.end - b.start);
            }
            if (!bond_matches.empty() && static_cast<double>(covered) >= nlen - 1.0) {
                for (const auto& b : bond_matches) {
                    hits.push_back(Hit {
                        "bond",
                        b.name,
                        box.x + box.w * static_cast<double>(b.start) / nlen,
                        box.y,
                    });
                }
                continue;
            }
            for (const auto& m : match_dict(norm, name_dict, true)) {
                hits.push_back(Hit {
                    "name",
                    m.name,
                    box.x + box.w * static_cast<double>(m.start) / nlen,
                    box.y,
                });
            }
        }
        return hits;
    };

    std::vector<Hit> all_hits = collect(ocr(ch_orig));
    const auto hits_red = collect(ocr(ch_red));
    all_hits.insert(all_hits.end(), hits_red.begin(), hits_red.end());

    std::vector<Hit> name_hits;
    for (const auto& h : all_hits) {
        if (h.kind == "name") {
            name_hits.push_back(h);
        }
    }
    std::sort(name_hits.begin(), name_hits.end(), [](const Hit& a, const Hit& b) {
        return a.x < b.x;
    });

    // ---- 按 x 聚类（`X` 距离 <90） ----
    struct Group
    {
        double x = 0.0;
        std::map<std::string, int> names;
        std::set<std::string> bonds;
    };
    std::vector<Group> groups;
    auto group_at = [&](double x) -> Group& {
        for (auto& g : groups) {
            if (std::abs(g.x - x) < 90.0) {
                return g;
            }
        }
        groups.push_back(Group { x, {}, {} });
        return groups.back();
    };
    for (const auto& h : name_hits) {
        ++group_at(h.x).names[h.value];
    }
    for (const auto& h : all_hits) {
        if (h.kind == "bond") {
            group_at(h.x).bonds.insert(h.value);
        }
    }
    std::sort(groups.begin(), groups.end(), [](const Group& a, const Group& b) {
        return a.x < b.x;
    });

    // `N = me.length ? [Ai(me.map(q=>q.y))] : [1000, 1018]`
    std::vector<double> y_candidates;
    if (!name_hits.empty()) {
        std::vector<double> ys;
        ys.reserve(name_hits.size());
        for (const auto& h : name_hits) {
            ys.push_back(h.y);
        }
        std::sort(ys.begin(), ys.end());
        const size_t n = ys.size();
        y_candidates.push_back((n % 2 == 1) ? ys[(n - 1) / 2] : (ys[n / 2 - 1] + ys[n / 2]) / 2.0);
    }
    else {
        y_candidates = { 1000.0, 1018.0 };
    }

    // `U`：卡位区间
    std::vector<std::pair<int, int>> intervals;
    if (auto grid = detect_card_gaps(img)) {
        const int step = static_cast<int>(std::lround(grid->p));
        for (int q = grid->first; static_cast<double>(q) + grid->p <= 1700.0; q += step) {
            const int end = q + step;
            if (end > jt.x + 60 && q < 1640) {
                intervals.emplace_back(q, end);
            }
        }
    }

    // `l(channels, d, g)`
    auto read_name_at = [&](int d, double g) -> std::string {
        if (ch_clahe.empty()) {
            ch_clahe = img_extend_black(img_clahe(img_gray(scaled), 24, 24, 6), Qr, Qr, 0, 0);
        }
        const std::vector<const cv::Mat*> channels = { &ch_red, &ch_clahe, &ch_orig };
        for (const cv::Mat* ch : channels) {
            for (int b : { 190, 120 }) {
                const int left = std::max(0, static_cast<int>(std::lround((d - jt.x - 6) * Ut)));
                const int top = static_cast<int>(std::lround((g - jt.y - 6) * Ut + Qr));
                for (const auto& it : ocr_rec_only(*ch, { left, top, b * Ut, 40 * Ut }, 1, 0)) {
                    const auto found = match_dict(it.text, name_dict, true);
                    if (found.size() == 1) {
                        return found.front().name;
                    }
                }
            }
        }
        return {};
    };

    for (const auto& [q, p] : intervals) {
        Group* target = nullptr;
        for (auto& g : groups) {
            if (g.x > q - 10 && g.x < p - 30) {
                target = &g;
                break;
            }
        }
        if (target != nullptr && !target->names.empty()) {
            continue;
        }
        std::string found;
        for (double gy : y_candidates) {
            found = read_name_at(q + 22, gy);
            if (!found.empty()) {
                break;
            }
        }
        if (found.empty()) {
            continue;
        }
        if (target != nullptr) {
            target->names[found] = 1;
        }
        else {
            Group g;
            g.x = q + 40;
            g.names[found] = 1;
            groups.push_back(g);
        }
    }

    std::sort(groups.begin(), groups.end(), [](const Group& a, const Group& b) {
        return a.x < b.x;
    });

    std::vector<ShopCard> cards;
    cards.reserve(groups.size());
    for (const auto& g : groups) {
        std::vector<std::string> bonds;
        bonds.reserve(g.bonds.size());
        for (const auto& b : g.bonds) {
            bonds.push_back(b);
        }
        std::sort(bonds.begin(), bonds.end());

        // `[...q.names.entries()].sort((a,b)=> b[1]-a[1] || b[0].length-a[0].length)[0]`
        std::string best;
        int best_cnt = -1;
        size_t best_len = 0;
        for (const auto& [name, cnt] : g.names) {
            const size_t len = static_cast<size_t>(utf8_len(name));
            if (cnt > best_cnt || (cnt == best_cnt && len > best_len)) {
                best = name;
                best_cnt = cnt;
                best_len = len;
            }
        }

        ShopCard card;
        card.x = static_cast<int>(std::lround(g.x));
        card.bonds = bonds;
        if (!best.empty()) {
            const auto it = norm_to_orig.find(best);
            card.name = (it != norm_to_orig.end()) ? it->second : best;
            card.kind = (operator_norms.count(best) > 0) ? "operator" : "item";
            card.source = "ocr";
            cards.push_back(card);
            continue;
        }

        std::vector<std::string> candidates;
        if (!bonds.empty()) {
            std::vector<std::string> keys;
            keys.reserve(bonds.size());
            for (const auto& b : bonds) {
                keys.push_back(strip_punct(b));
            }
            std::sort(keys.begin(), keys.end());
            std::string key;
            for (size_t i = 0; i < keys.size(); ++i) {
                if (i > 0) {
                    key += "+";
                }
                key += keys[i];
            }
            auto it = bond_combo.find(key);
            if (it != bond_combo.end()) {
                candidates = it->second;
            }
        }
        if (candidates.size() == 1) {
            card.name = candidates.front();
            card.kind = "operator";
            card.source = "bonds";
        }
        else {
            card.name.clear();
            card.kind = bonds.empty() ? "" : "operator";
            card.source.clear();
        }
        cards.push_back(card);
    }

    // 注意：`Mt`（免费卡）不是 `Bi` 的一部分，参考实现在 `je` 的批处理里单独调用。
    return cards;
}

void SPAFarmTaskPlugin::detect_free_cards(const cv::Mat& img, std::vector<ShopCard>& cards) const
{
    // `Mt(I, k)` —— service.pretty.mjs L8908-8929
    //   left = round(x + 40)；越界跳过
    //   extract({left, 760, 130, 40}).resize(520, 160).extend(40 全边黑) → bt
    //   命中 /^[0OoＯ〇]$/ 且 score >= .6 即为免费卡
    for (auto& card : cards) {
        if (card.name.empty()) {
            continue;
        }
        const int left = static_cast<int>(std::lround(static_cast<double>(card.x) + 40.0));
        if (left < 0 || left + 130 > 1920) {
            continue;
        }
        cv::Mat crop = img_crop(img, left, 760, 130, 40);
        if (crop.empty()) {
            continue;
        }
        cv::Mat big = img_resize_cubic(crop, 520, 160);
        big = img_extend_black(big, 40, 40, 40, 40);
        for (const auto& it : ocr(big)) {
            const std::string t = strip_ws(it.text);
            if (it.score >= 0.6 && (t == "0" || t == "O" || t == "o" || t == "Ｏ" || t == "〇")) {
                card.free_card = true;
                card.cost = 0;
                break;
            }
        }
    }
}

// ============================================================================
// Board Unit Detection, Sprite Histogram & Detail Panel (`he`, `yn`, `ta`, `Oi`, `Gt`, `Ce`, `Pt`, `zr`)
// ============================================================================

cv::Rect SPAFarmTaskPlugin::unit_sprite_box(
    const std::array<double, 9>& H,
    int r,
    int c,
    std::string_view camera)
{
    // `ta(a, s, e, t)` —— service.pretty.mjs L5598-5606
    //   u = 相邻格中心投影的 x 差；框 = (fx - 0.45u, y_top, 0.9u, fy + 0.1u - y_top)
    //   注意：JS 没有任何下限钳制，这里也不加。
    auto [fx, fy] = project_point(H, c + 0.5, r + 0.5);
    auto [ix, iy] = project_point(H, c, r + 0.5);
    auto [mx, my] = project_point(H, c + 1.0, r + 0.5);
    (void)iy;
    (void)my;
    const double u = mx - ix;
    const double top_y = project_point(H, c + 0.5, r + 0.5 + (camera == "expanded" ? -1.3 : -0.95)).second;
    const int rx = static_cast<int>(std::lround(fx - 0.45 * u));
    const int ry = static_cast<int>(std::lround(top_y));
    const int rw = static_cast<int>(std::lround(0.9 * u));
    const int rh = static_cast<int>(std::lround(fy + 0.1 * u - top_y));
    return cv::Rect(rx, ry, rw, rh);
}

double SPAFarmTaskPlugin::green_hp_bar_ratio(
    const cv::Mat& img,
    double cx,
    double top_y,
    double box_h)
{
    // Exact 1:1 port of `Si(a, s, e, t)` in service.pretty.mjs lines 5506-5523
    if (img.empty()) {
        return 0.0;
    }
    const double f = std::max(16.0, box_h);
    const int y1 = std::clamp(static_cast<int>(std::lround(top_y - f * 5.0)), 0, img.rows);
    const int y2 = std::clamp(static_cast<int>(std::lround(top_y - f * 0.2)), 0, img.rows);
    const int x1 = std::clamp(static_cast<int>(std::lround(cx - 36.0)), 0, img.cols);
    const int x2 = std::clamp(static_cast<int>(std::lround(cx + 36.0)), 0, img.cols);
    int total = 0;
    int green = 0;
    for (int y = y1; y < y2; ++y) {
        const auto* row = img.ptr<cv::Vec3b>(y);
        for (int x = x1; x < x2; ++x) {
            const int B = row[x][0];
            const int G = row[x][1];
            const int R = row[x][2];
            ++total;
            if (G > 120 && G > R + 40 && G > B + 10) {
                ++green;
            }
        }
    }
    return total > 0 ? static_cast<double>(green) / static_cast<double>(total) : 0.0;
}

std::vector<double> SPAFarmTaskPlugin::compute_hsv_hist(
    const cv::Mat& img,
    const cv::Rect& roi,
    const cv::Mat* bg_ref)
{
    // Exact 1:1 port of `Li` + `Oi` in service.pretty.mjs lines 5626-5650 (108-bin HSV histogram with background subtraction)
    if (img.empty()) {
        return {};
    }
    cv::Rect clipped = roi & cv::Rect(0, 0, img.cols, img.rows);
    if (clipped.width <= 4 || clipped.height <= 4) {
        return {};
    }
    cv::Mat patch = img(clipped);
    const bool has_bg = (bg_ref != nullptr && !bg_ref->empty()
                         && bg_ref->cols == patch.cols && bg_ref->rows == patch.rows);
    std::vector<double> hist(108, 0.0);
    int fg_count = 0;
    const int total_px = patch.cols * patch.rows;
    for (int y = 0; y < patch.rows; ++y) {
        const auto* row = patch.ptr<cv::Vec3b>(y);
        const auto* bg_row = has_bg ? bg_ref->ptr<cv::Vec3b>(y) : nullptr;
        for (int x = 0; x < patch.cols; ++x) {
            const int B = row[x][0];
            const int G = row[x][1];
            const int R = row[x][2];
            if (has_bg) {
                const int d = std::abs(R - bg_row[x][2]) + std::abs(G - bg_row[x][1]) + std::abs(B - bg_row[x][0]);
                if (d < 60) {
                    continue;
                }
            }
            const int mx = std::max({ R, G, B });
            const int mn = std::min({ R, G, B });
            const int diff = mx - mn;
            const double val = mx / 255.0;
            const double sat = (mx > 0) ? (static_cast<double>(diff) / mx) : 0.0;
            double hue = 0.0;
            if (diff > 0) {
                if (mx == R) {
                    hue = std::fmod(static_cast<double>(G - B) / diff + 6.0, 6.0);
                }
                else if (mx == G) {
                    hue = static_cast<double>(B - R) / diff + 2.0;
                }
                else {
                    hue = static_cast<double>(R - G) / diff + 4.0;
                }
            }
            const int h_bin = (sat < 0.2) ? 0 : (static_cast<int>(std::floor(hue * 2.0)) % 12);
            const int s_bin = std::min(2, static_cast<int>(std::floor(sat * 3.0)));
            const int v_bin = std::min(2, static_cast<int>(std::floor(val * 3.0)));
            const int bin = h_bin * 9 + s_bin * 3 + v_bin;
            if (bin >= 0 && bin < 108) {
                hist[bin] += 1.0;
                ++fg_count;
            }
        }
    }
    if (fg_count < total_px * 0.08) {
        return {};
    }
    for (double& v : hist) {
        v /= static_cast<double>(fg_count);
    }
    return hist;
}

double SPAFarmTaskPlugin::hist_intersection(const std::vector<double>& a, const std::vector<double>& b)
{
    // Exact 1:1 port of `Ti(a, s)` in service.pretty.mjs lines 5652-5656
    if (a.size() != 108 || b.size() != 108) {
        return 0.0;
    }
    double s = 0.0;
    for (size_t i = 0; i < 108; ++i) {
        s += std::min(a[i], b[i]);
    }
    return s;
}

std::vector<std::pair<double, double>> SPAFarmTaskPlugin::detect_unit_badges(
    const cv::Mat& img,
    const std::array<double, 9>& H,
    std::string_view camera,
    const std::vector<std::string>& rows) const
{
    // Exact 1:1 port of `he(j, J)` —— service.pretty.mjs L6773-6791:
    //   qe(j).resize(1280, 720) → post_recognition("TemplateMatch", {
    //       template: ["badge_l1.png".."badge_l6.png"], threshold: .7,
    //       method: 5 (TM_CCOEFF_NORMED), green_mask: !0,
    //       order_by: "Score", roi: [80, 120, 1100, 440] })
    //   → reco.detail.filtered 的 box 中心 ×1.5 还原到 1920 空间、<15px 去重。
    // `he` 本身不依赖棋盘几何；H / camera / rows 是下游 `yn`（map_badges_to_units）的输入。
    (void)H;
    (void)camera;
    (void)rows;

    std::vector<std::pair<double, double>> badges;
    if (img.empty() || m_badge_templates.empty()) {
        return badges;
    }

    // sharp 的 resize 默认核是 lanczos3，OpenCV 用 LANCZOS4 对应
    cv::Mat small;
    cv::resize(img, small, cv::Size(1280, 720), 0.0, 0.0, cv::INTER_LANCZOS4);
    cv::Rect roi(80, 120, 1100, 440); // `[x, y, w, h]`，1280 空间
    roi &= cv::Rect(0, 0, small.cols, small.rows);
    if (roi.width <= 0 || roi.height <= 0) {
        return badges;
    }
    cv::Mat scene = small(roi).clone();
    cv::Mat scene_rgb;
    cv::cvtColor(scene, scene_rgb, cv::COLOR_BGR2RGB);

    struct Hit
    {
        double x = 0.0;
        double y = 0.0;
        float score = 0.0f;
    };
    std::vector<Hit> hits;

    for (size_t ti = 0; ti < m_badge_templates.size(); ++ti) {
        const cv::Mat& templ = m_badge_templates[ti];
        if (templ.cols > scene_rgb.cols || templ.rows > scene_rgb.rows) {
            continue;
        }
        cv::Mat templ_rgb;
        cv::cvtColor(templ, templ_rgb, cv::COLOR_BGR2RGB);

        // `green_mask: !0` == VisionHelper::create_mask(templ, true)：
        //   inRange 纯绿 (0,255,0) 后取反，绿色背景不参与匹配
        cv::Mat green;
        cv::inRange(
            templ_rgb,
            cv::Scalar(0, 255, 0),
            cv::Scalar(0, 255, 0),
            green);
        cv::Mat mask;
        cv::bitwise_not(green, mask);
        const int mask_pixels = cv::countNonZero(mask);
        if (mask_pixels == 0) {
            continue;
        }

        // MAA 自带 masked TM_CCOEFF_NORMED（与 MaaFramework 的 green_mask + method:5 等价）
        cv::Mat matched = MaskedCcoeffMatcher::get_instance().match(
            scene_rgb,
            templ_rgb,
            mask,
            "spa_badge_l" + std::to_string(ti + 1),
            mask_pixels);
        if (matched.empty()) {
            // OpenCV 只有 TM_SQDIFF / TM_CCORR_NORMED 支持 mask，FFT 路径不可用时兜底
            cv::matchTemplate(scene_rgb, templ_rgb, matched, cv::TM_CCORR_NORMED, mask);
        }
        if (matched.empty()) {
            continue;
        }

        const int min_distance = std::min(templ.cols, templ.rows) / 2; // MAA MultiMatcher 默认 NMS 半径
        for (int y = 0; y < matched.rows; ++y) {
            for (int x = 0; x < matched.cols; ++x) {
                const float v = matched.at<float>(y, x);
                if (v < 0.7f || std::isnan(v) || std::isinf(v)) {
                    continue;
                }
                Hit h;
                h.x = roi.x + x + templ.cols / 2.0;
                h.y = roi.y + y + templ.rows / 2.0;
                h.score = v;
                bool need_push = true;
                for (auto it = hits.rbegin(); it != hits.rend(); ++it) {
                    if (std::abs(h.x - it->x) >= min_distance || std::abs(h.y - it->y) >= min_distance) {
                        continue;
                    }
                    if (it->score < v) {
                        *it = h; // 同位只留高分
                    }
                    need_push = false;
                    break;
                }
                if (need_push) {
                    hits.push_back(h);
                }
            }
        }
    }

    // `order_by: "Score"` + JS 末尾的 `<15px`（1920 空间）去重
    std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) {
        return a.score > b.score;
    });
    for (const auto& h : hits) {
        const double px = h.x * 1.5;
        const double py = h.y * 1.5;
        bool duplicate = false;
        for (const auto& b : badges) {
            if (std::hypot(b.first - px, b.second - py) < 15.0) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            badges.emplace_back(px, py);
        }
    }
    return badges;
}

std::vector<SPAFarmTaskPlugin::UnitSlot> SPAFarmTaskPlugin::map_badges_to_units(
    const std::vector<std::pair<double, double>>& badges,
    const std::array<double, 9>& H,
    std::string_view camera,
    const std::vector<std::string>& rows) const
{
    // Exact 1:1 port of `yn(a, s, e, t)` in service.pretty.mjs lines 6491-6527
    const double w = (camera == "expanded") ? -1.25 : -0.86;
    const auto inv_H = invert_homography(H);

    struct MatchCand
    {
        size_t b = 0;
        int r = 0;
        int c = 0;
        double d = 0.0;
    };
    std::vector<MatchCand> cands;
    for (size_t i = 0; i < badges.size(); ++i) {
        auto [gx, gy] = project_point(inv_H, badges[i].first, badges[i].second);
        const double y = gx - (-0.27);
        const double b = gy - w;
        for (int r = 0; r < static_cast<int>(rows.size()); ++r) {
            for (int c = 0; c < static_cast<int>(rows[r].size()); ++c) {
                char ch = rows[r][c];
                if (ch != 'a' && ch != 'b' && ch != 'h' && ch != 'H') {
                    continue;
                }
                const double dist = std::hypot(y - (c + 0.5), b - (r + 0.5));
                if (dist < 0.8) {
                    cands.push_back(MatchCand { i, r, c, dist });
                }
            }
        }
    }
    std::sort(cands.begin(), cands.end(), [](const MatchCand& a, const MatchCand& b) {
        return a.d < b.d;
    });

    std::set<size_t> used_b;
    std::set<std::string> used_cell;
    std::vector<UnitSlot> units;
    for (const auto& m : cands) {
        std::string cell_key = std::to_string(m.r) + "," + std::to_string(m.c);
        if (used_b.count(m.b) > 0 || used_cell.count(cell_key) > 0) {
            continue;
        }
        used_b.insert(m.b);
        used_cell.insert(cell_key);
        const bool is_bench = (rows[m.r][m.c] == 'H');
        UnitSlot u;
        u.slot = is_bench
            ? ("bench:" + std::to_string(m.c + 1))
            : ("board:" + std::to_string(m.r) + "," + std::to_string(m.c));
        u.area = is_bench ? "bench" : "board";
        u.r = m.r;
        u.c = m.c;
        units.push_back(std::move(u));
    }
    return units;
}

bool SPAFarmTaskPlugin::is_detail_panel_open(const cv::Mat& img) const
{
    // Exact 1:1 port of `Ce(j)` in service.pretty.mjs lines 7303-7306
    if (img.empty()) {
        return false;
    }
    auto left_tabs = ocr(img, std::array<int, 4> { 20, 530, 640, 50 });
    if (has_ocr(left_tabs, { "特质", "技能", "特性", "天赋" })) {
        return true;
    }
    auto buy_bar = ocr(img, std::array<int, 4> { 250, 960, 1450, 110 });
    return has_ocr(buy_bar, { "确认购买" });
}

bool SPAFarmTaskPlugin::close_detail_panel()
{
    // Exact 1:1 port of `ge()` in service.pretty.mjs lines 7280-7292 (`vn = [1450, 300]`, fallback `[300, 950]`)
    for (int i = 0; i < 4 && !need_exit(); ++i) {
        tap(1450, 300, 180);
        if (!is_detail_panel_open(shot())) {
            return true;
        }
        tap(300, 950, 220);
        if (!is_detail_panel_open(shot())) {
            return true;
        }
    }
    return !is_detail_panel_open(shot());
}

std::pair<std::string, std::string> SPAFarmTaskPlugin::read_unit_detail_panel(const cv::Mat& img) const
{
    // Exact 1:1 port of `Pt(j)` + `us(a, s, e)` in service.pretty.mjs lines 6529-6559 & 7406-7445
    if (img.empty()) {
        return { "", "" };
    }
    auto panel_ocr = ocr(img, std::array<int, 4> { 20, 270, 560, 220 });
    std::string resolved_name;

    // 1. Top title line (`box[1] < 270 + 45`) exact/fuzzy match against operators & items
    std::string top_text;
    for (const auto& it : panel_ocr) {
        if (it.y < 315) {
            top_text += it.text;
            std::string nt = norm_text(it.text);
            if (const auto* op = find_operator(nt)) {
                resolved_name = op->name;
                break;
            }
            if (const auto* item = find_item(nt)) {
                resolved_name = item->name;
                break;
            }
        }
    }
    if (resolved_name.empty() && !top_text.empty()) {
        auto cm = match_dict(top_text, m_chess_dict);
        if (!cm.empty()) {
            resolved_name = cm.front().name;
        }
        else {
            auto im = match_dict(top_text, m_item_dict);
            if (!im.empty()) {
                resolved_name = im.front().name;
            }
        }
    }

    // 2. Bond + ATK/DEF disambiguation (`us` lines 6536-6558)
    if (resolved_name.empty()) {
        std::vector<std::string> seen_bonds;
        std::optional<int> atk_val;
        std::optional<int> def_val;
        for (const auto& it : panel_ocr) {
            for (const auto& b : m_bonds) {
                if (it.text.find(b.name) != std::string::npos) {
                    seen_bonds.push_back(b.name);
                }
            }
            auto pos_a = it.text.find("攻击");
            if (pos_a != std::string::npos) {
                std::string d = extract_digits_mapped(it.text.substr(pos_a));
                if (!d.empty()) {
                    atk_val = std::stoi(d);
                }
            }
            auto pos_d = it.text.find("防御");
            if (pos_d != std::string::npos) {
                std::string d = extract_digits_mapped(it.text.substr(pos_d));
                if (!d.empty()) {
                    def_val = std::stoi(d);
                }
            }
        }
        if (!seen_bonds.empty() || atk_val.has_value()) {
            std::vector<std::string> cands;
            for (const auto& op : m_operators) {
                bool bonds_ok = true;
                for (const auto& sb : seen_bonds) {
                    if (std::find(op.bond_names.begin(), op.bond_names.end(), sb) == op.bond_names.end()) {
                        bonds_ok = false;
                        break;
                    }
                }
                if (!bonds_ok) {
                    continue;
                }
                if (atk_val.has_value() && op.atk != *atk_val) {
                    continue;
                }
                if (def_val.has_value() && op.def != *def_val) {
                    continue;
                }
                cands.push_back(op.name);
            }
            if (cands.size() == 1) {
                resolved_name = cands.front();
            }
        }
    }

    // 3. Detect orange facing cone in `[640, 450, 960, 630]` (`Pt` lines 7416-7435)
    std::string facing;
    double sx = 0.0, sy = 0.0;
    int cone_n = 0;
    for (int y = 450; y < 1080 && y < img.rows; ++y) {
        const auto* row = img.ptr<cv::Vec3b>(y);
        for (int x = 640; x < 1600 && x < img.cols; ++x) {
            const int B = row[x][0], G = row[x][1], R = row[x][2];
            if (R > 220 && G > 110 && G < 175 && B < 70) {
                sx += (x - 640);
                sy += (y - 450);
                ++cone_n;
            }
        }
    }
    if (cone_n > 500) {
        const double cx = sx / cone_n;
        const double cy = sy / cone_n;
        double mx = 0.0, my = 0.0;
        int outer_n = 0;
        for (int y = 450; y < 1080 && y < img.rows; ++y) {
            const auto* row = img.ptr<cv::Vec3b>(y);
            for (int x = 640; x < 1600 && x < img.cols; ++x) {
                const int B = row[x][0], G = row[x][1], R = row[x][2];
                if (R > 220 && G > 110 && G < 175 && B < 70) {
                    const double dx = (x - 640) - cx;
                    const double dy = (y - 450) - cy;
                    if (dx * dx + dy * dy > 1600.0) {
                        mx += dx;
                        my += dy;
                        ++outer_n;
                    }
                }
            }
        }
        if (outer_n > 100) {
            if (std::abs(mx) > std::abs(my)) {
                facing = (mx > 0.0) ? "RIGHT" : "LEFT";
            }
            else {
                facing = (my > 0.0) ? "DOWN" : "UP";
            }
        }
    }

    return { resolved_name, facing };
}

void SPAFarmTaskPlugin::check_triple_merge(const std::string& op_name, bool force)
{
    // `mt(j, J)` —— service.pretty.mjs L6829-6845
    //   1) 取**全部**同名单位（不看 elite）
    //   2) 只有一个：直接标 elite
    //   3) 多个：按 (ghost, area==board) 排序取第一个保留并标 elite，其余从列表与 remembered 中删除
    std::vector<size_t> idx;
    for (size_t k = 0; k < m_units.size(); ++k) {
        if (m_units[k].name == op_name) {
            idx.push_back(k);
        }
    }
    if (idx.empty()) {
        return;
    }
    if (idx.size() <= 1) {
        m_units[idx.front()].elite = true;
        auto it = m_remembered_units.find(m_units[idx.front()].slot);
        if (it != m_remembered_units.end()) {
            it->second.elite = true;
        }
        return;
    }
    // JS 只在 `p`（待合成）里出现过才走到这里；非待合成时数量 >1 属正常多张，不做合并
    if (!force && m_pending_merge.count(op_name) == 0) {
        return;
    }

    auto rank = [](const UnitSlot& u) {
        return std::pair<int, int> { u.ghost ? 1 : 0, u.area == "board" ? 0 : 1 };
    };
    size_t keep = idx.front();
    for (size_t k : idx) {
        if (rank(m_units[k]) < rank(m_units[keep])) {
            keep = k;
        }
    }
    m_units[keep].elite = true;
    const std::string keep_slot = m_units[keep].slot;
    auto kit = m_remembered_units.find(keep_slot);
    if (kit != m_remembered_units.end()) {
        kit->second.elite = true;
    }
    std::vector<size_t> drop;
    for (size_t k : idx) {
        if (k != keep) {
            drop.push_back(k);
        }
    }
    std::sort(drop.rbegin(), drop.rend());
    for (size_t k : drop) {
        m_remembered_units.erase(m_units[k].slot);
        m_units.erase(m_units.begin() + static_cast<std::ptrdiff_t>(k));
    }
    log_info("三合一升精: " + op_name + " -> 保留于 " + keep_slot);
}

void SPAFarmTaskPlugin::refresh_board_snapshot(
    const cv::Mat& img,
    const CameraDetectResult& cam,
    bool force_shop)
{
    // Exact 1:1 port of `Tr` + `Gt` + `rr` in service.pretty.mjs lines 6888-7205
    if (img.empty() || cam.camera.empty()) {
        return;
    }
    const auto* st = find_stage(m_stage_id);
    if (!st) {
        return;
    }
    auto badges = detect_unit_badges(img, cam.H, cam.camera, st->rows);
    std::set<std::string> occ_cells;
    auto detected = map_badges_to_units(badges, cam.H, cam.camera, st->rows);
    for (const auto& u : detected) {
        occ_cells.insert(std::to_string(u.r) + "," + std::to_string(u.c));
        occ_cells.insert(std::to_string(u.r - 1) + "," + std::to_string(u.c));
    }
    update_stage_from_board(img, cam.H, cam.camera, occ_cells);

    // Record empty cell background references & compute unit sprite histograms (`rr` lines 7084-7095)
    for (int r = 1; r < static_cast<int>(st->rows.size()); ++r) {
        for (int c = 0; c < static_cast<int>(st->rows[r].size()); ++c) {
            char ch = st->rows[r][c];
            if (ch != 'a' && ch != 'b' && ch != 'h' && ch != 'H') {
                continue;
            }
            std::string ckey = std::to_string(r) + "," + std::to_string(c);
            std::string ckey_below = std::to_string(r + 1) + "," + std::to_string(c);
            if (occ_cells.count(ckey) == 0 && occ_cells.count(ckey_below) == 0) {
                cv::Rect box = unit_sprite_box(cam.H, r, c, cam.camera) & cv::Rect(0, 0, img.cols, img.rows);
                if (box.width > 4 && box.height > 4) {
                    m_empty_cell_refs[cam.camera + ":" + ckey] = img(box).clone();
                }
            }
        }
    }

    for (auto& u : detected) {
        std::string ckey = std::to_string(u.r) + "," + std::to_string(u.c);
        std::string ckey_above = std::to_string(u.r - 1) + "," + std::to_string(u.c);
        auto bg_it = m_empty_cell_refs.find(cam.camera + ":" + ckey);
        if (bg_it != m_empty_cell_refs.end() && occ_cells.count(ckey_above) == 0) {
            u.hist = compute_hsv_hist(img, unit_sprite_box(cam.H, u.r, u.c, cam.camera), &bg_it->second);
        }
    }

    // Reconcile identity from panel cache, previous units, histogram bank, and recent shop purchases (`Gt`)
    std::unordered_map<std::string, UnitSlot> prev_by_slot;
    for (const auto& u : m_units) {
        prev_by_slot[u.slot] = u;
    }

    for (auto& u : detected) {
        auto pv = m_panel_verified_slots.find(u.slot);
        if (pv != m_panel_verified_slots.end()) {
            u.name = pv->second;
            u.source = "panel";
            u.verified = true;
            u.confirmed = true;
            m_panel_verified_slots.erase(pv);
            continue;
        }
        auto prev_it = prev_by_slot.find(u.slot);
        if (prev_it != prev_by_slot.end() && !prev_it->second.name.empty()) {
            u.name = prev_it->second.name;
            u.source = "carried";
            u.facing = prev_it->second.facing;
            u.elite = prev_it->second.elite;
            u.verified = prev_it->second.verified;
            u.confirmed = prev_it->second.confirmed;
            u.auxiliary = prev_it->second.auxiliary;
            u.item = prev_it->second.item;
            continue;
        }
        auto rem_it = m_remembered_units.find(u.slot);
        if (rem_it != m_remembered_units.end() && !rem_it->second.name.empty()) {
            u.name = rem_it->second.name;
            u.source = "carried";
            u.facing = rem_it->second.facing;
            u.elite = rem_it->second.elite;
            u.verified = rem_it->second.verified;
        }
    }

    // Assign newly appeared bench units from `m_recent_bought_names` (right-to-left on bench)
    std::vector<size_t> unnamed_bench;
    for (size_t i = 0; i < detected.size(); ++i) {
        if (detected[i].area == "bench" && detected[i].name.empty()) {
            unnamed_bench.push_back(i);
        }
    }
    std::sort(unnamed_bench.begin(), unnamed_bench.end(), [&](size_t a, size_t b) {
        return detected[a].c > detected[b].c;
    });
    while (!unnamed_bench.empty() && !m_recent_bought_names.empty()) {
        size_t idx = unnamed_bench.front();
        unnamed_bench.erase(unnamed_bench.begin());
        detected[idx].name = m_recent_bought_names.front();
        detected[idx].source = "shop";
        detected[idx].verified = true;
        m_recent_bought_names.erase(m_recent_bought_names.begin());
    }

    // Mark auxiliary/summon/item units (`line 7187`)
    for (auto& u : detected) {
        if (u.name.empty()) {
            continue;
        }
        std::string nt = norm_text(u.name);
        for (const auto& aux : kAuxiliaryNames) {
            if (nt == norm_text(aux)) {
                u.auxiliary = true;
                break;
            }
        }
        if (!find_operator(u.name) && find_item(u.name)) {
            u.auxiliary = true;
            u.item = true;
        }
        m_remembered_units[u.slot] = u;
        if (!u.hist.empty() && (u.confirmed || u.source == "panel")) {
            auto& bank = m_op_hist_bank[u.name];
            bank.push_back(u.hist);
            if (bank.size() > 6) {
                bank.erase(bank.begin());
            }
        }
    }

    // 被遮挡的已记住单位：`ghost` 保留 / 20s 后丢弃（`Ut` L7053-7071）
    //   `if (!v.aux && v.area === "board" && v.r != null && v.c != null)` → 直接补 ghost
    //   否则记 `missingSince`，超过 `W = 2e4` 才从 `h` 里删
    const double now_s = now_sec();
    for (const auto& [slot, rem] : m_remembered_units) {
        bool present = false;
        for (const auto& u : detected) {
            if (u.slot == slot) {
                present = true;
                break;
            }
        }
        if (present) {
            m_missing_since.erase(slot);
            continue;
        }
        if (!rem.auxiliary && !rem.item && rem.area == "board" && !rem.name.empty()) {
            UnitSlot ghost_u = rem;
            ghost_u.ghost = true;
            ghost_u.source = "carried";
            ghost_u.verified = true;
            ghost_u.confirmed = true;
            detected.push_back(std::move(ghost_u));
            continue;
        }
        if (m_missing_since.count(slot) == 0) {
            m_missing_since[slot] = now_s;
        }
        else if (now_s - m_missing_since[slot] > 20.0) {
            m_missing_since.erase(slot);
        }
    }
    // 清掉已不在 remembered 里的计时
    for (auto it = m_missing_since.begin(); it != m_missing_since.end();) {
        it = (m_remembered_units.count(it->first) == 0) ? m_missing_since.erase(it) : std::next(it);
    }

    m_units = std::move(detected);

    // `Date.now() - Dr > 5e3` → 同名非精锐 >= 3 张就地三合一（节流 5s）
    if (now_s - m_last_merge_check > 5.0) {
        m_last_merge_check = now_s;
        std::set<std::string> names;
        for (const auto& u : m_units) {
            if (!u.name.empty() && !u.elite) {
                names.insert(u.name);
            }
        }
        for (const auto& nm : names) {
            int cnt = 0;
            for (const auto& u : m_units) {
                if (u.name == nm && !u.elite) {
                    ++cnt;
                }
            }
            if (cnt >= 3) {
                check_triple_merge(nm, true);
            }
        }
    }

    for (auto& u : m_units) {
        if (!u.name.empty() && m_elite_units.count(u.name) > 0) {
            u.elite = true; // `for (const re of L.units) re.name && Et.has(re.name) && (re.elite = !0)`
        }
    }

    // Update deployCap from `剩余可部署 N` (`line 7189-7190`)
    if (auto rem = read_remaining_deploy(img, ocr(img))) {
        int board_cnt = 0;
        for (const auto& u : m_units) {
            if (u.area == "board" && !u.auxiliary) {
                ++board_cnt;
            }
        }
        m_deploy_cap = std::max(m_deploy_cap, board_cnt + *rem);
    }

    if (force_shop && cam.camera == "expanded") {
        m_shop = read_shop_cards(img);
    }
}

void SPAFarmTaskPlugin::inspect_unknown_units(
    const CameraDetectResult& cam,
    bool skip_bench_unknown,
    bool force_all)
{
    // Exact 1:1 port of `zr` in service.pretty.mjs lines 7447-7528 (including waitStableBoard L7463, L7464, L7476)
    (void)cam;
    std::optional<std::array<double, 4>> prev_pts;
    for (size_t idx = 0; idx < m_units.size(); ++idx) {
        if (need_exit()) {
            return;
        }
        auto& u = m_units[idx];
        if (skip_bench_unknown && u.area == "bench" && u.name.empty()) {
            continue;
        }
        const auto* op = u.name.empty() ? nullptr : find_operator(u.name);
        const bool need_facing = (u.area == "board" && u.facing.empty() && (!op || !op->facing_independent));
        // `zr` 的 `onlyUnknown`：为真时「已命名且已确认」的一律跳过；
        // 为假时（`!identifyUnknown && !B` 中的 B，即金干员合计）全量重读。
        const bool need_name = u.name.empty() || (!u.confirmed && u.source == "guess");
        if (!force_all && !need_name && !need_facing) {
            continue;
        }

        auto st = wait_stable_board(2500, prev_pts);
        if (!st.has_value()) {
            close_detail_panel();
            st = wait_stable_board(4000, prev_pts);
        }
        if (!st.has_value()) {
            log_info("镜头一直没有停稳，已停止自动点选");
            break;
        }
        prev_pts = st->pts;

        auto [px, py] = project_point(st->H, u.c + 0.5, u.r + 0.5);
        tap(px, py, 240);
        cv::Mat panel_img = shot();
        if (!is_detail_panel_open(panel_img)) {
            continue;
        }
        auto [det_name, det_facing] = read_unit_detail_panel(panel_img);

        // Duplicate board operator name re-verify (`service.pretty.mjs` lines 7474-7483)
        if (!det_name.empty() && u.area == "board") {
            bool dup_on_board = false;
            for (size_t j = 0; j < m_units.size(); ++j) {
                if (j != idx && m_units[j].area == "board" && m_units[j].confirmed && m_units[j].name == det_name) {
                    dup_on_board = true;
                    break;
                }
            }
            if (dup_on_board) {
                close_detail_panel();
                if (auto retry_st = wait_stable_board(4000)) {
                    prev_pts = retry_st->pts;
                    auto [rx, ry] = project_point(retry_st->H, u.c + 0.5, u.r + 0.5);
                    tap(rx, ry, 280);
                    cv::Mat retry_img = shot();
                    if (is_detail_panel_open(retry_img)) {
                        std::tie(det_name, det_facing) = read_unit_detail_panel(retry_img);
                    }
                }
            }
        }

        if (!det_name.empty()) {
            u.name = det_name;
            u.source = "panel";
            u.verified = true;
            u.confirmed = true;
            m_panel_verified_slots[u.slot] = det_name;
        }
        if (!det_facing.empty()) {
            u.facing = det_facing;
        }
        m_remembered_units[u.slot] = u;
        close_detail_panel();
    }
}

// ============================================================================
// Stage Traffic & Simulated Annealing Placement Solver (`Rr`, `Nt`, `Zr`, `is`, `ss`, `cr`, `ws`)
// ============================================================================


// ===========================================================================
// `Rr` 的原语（service.pretty.mjs L5681-5746）
// ===========================================================================

std::vector<std::pair<int, int>> SPAFarmTaskPlugin::cells_of(
    const std::vector<std::string>& rows,
    char ch)
{
    std::vector<std::pair<int, int>> out;
    for (int r = 0; r < static_cast<int>(rows.size()); ++r) {
        const std::string& line = rows[r];
        for (int c = 0; c < static_cast<int>(line.size()); ++c) {
            if (line[c] == ch) {
                out.emplace_back(r, c);
            }
        }
    }
    return out;
}

std::vector<std::pair<int, int>> SPAFarmTaskPlugin::deploy_cells(const std::vector<std::string>& rows)
{
    // `Pr(a)`：可部署格 a/b/h
    std::vector<std::pair<int, int>> out;
    for (int r = 0; r < static_cast<int>(rows.size()); ++r) {
        const std::string& line = rows[r];
        for (int c = 0; c < static_cast<int>(line.size()); ++c) {
            const char ch = line[c];
            if (ch == 'a' || ch == 'b' || ch == 'h') {
                out.emplace_back(r, c);
            }
        }
    }
    return out;
}

SPAFarmTaskPlugin::BfsResult SPAFarmTaskPlugin::bfs_from(
    const std::vector<std::string>& rows,
    const std::vector<std::pair<int, int>>& starts)
{
    // `aa(a, s)` L5691-5716：4 邻域，可走 b/c/S/E/I/O，同时累计最短路条数
    const int R = static_cast<int>(rows.size());
    const int C = R > 0 ? static_cast<int>(rows[0].size()) : 0;
    constexpr int kInf = 1000000;
    BfsResult out;
    out.dist.assign(R, std::vector<int>(C, kInf));
    out.ways.assign(R, std::vector<int>(C, 0));

    auto passable = [](char ch) {
        return ch == 'b' || ch == 'c' || ch == 'S' || ch == 'E' || ch == 'I' || ch == 'O';
    };

    std::vector<std::pair<int, int>> queue;
    for (const auto& st : starts) {
        if (st.first < 0 || st.second < 0 || st.first >= R || st.second >= C) {
            continue;
        }
        out.dist[st.first][st.second] = 0;
        out.ways[st.first][st.second] = 1;
        queue.push_back(st);
    }

    static const int dirs[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    for (size_t i = 0; i < queue.size(); ++i) {
        const auto [r, c] = queue[i];
        for (const auto& d : dirs) {
            const int nr = r + d[0];
            const int nc = c + d[1];
            if (nr < 0 || nc < 0 || nr >= R || nc >= C) {
                continue;
            }
            if (!passable(rows[nr][nc])) {
                continue;
            }
            if (out.dist[nr][nc] == kInf) {
                out.dist[nr][nc] = out.dist[r][c] + 1;
                queue.emplace_back(nr, nc);
            }
            if (out.dist[nr][nc] == out.dist[r][c] + 1) {
                out.ways[nr][nc] += out.ways[r][c];
            }
        }
    }
    return out;
}

std::vector<std::pair<int, int>> SPAFarmTaskPlugin::build_path(
    const std::vector<std::string>& rows,
    std::pair<int, int> from,
    std::pair<int, int> to)
{
    // `Hi(a, s, e)` L5724-5746：沿列推进，遇边界折返，最后对齐到目标
    int min_r = 0;
    int max_r = 0;
    {
        std::vector<int> rs;
        for (const auto& c : cells_of(rows, 'S')) {
            rs.push_back(c.first);
        }
        for (const auto& c : cells_of(rows, 'E')) {
            rs.push_back(c.first);
        }
        for (const auto& c : deploy_cells(rows)) {
            rs.push_back(c.first);
        }
        if (!rs.empty()) {
            min_r = *std::min_element(rs.begin(), rs.end());
            max_r = *std::max_element(rs.begin(), rs.end());
        }
    }

    const int step = (to.second < from.second) ? -1 : 1;
    const int stop_col = to.second - step;
    std::vector<std::pair<int, int>> path { from };
    int r = from.first;
    int c = from.second;

    auto advance_two = [&]() {
        for (int g = 0; g < 2 && c != stop_col; ++g) {
            c += step;
            path.emplace_back(r, c);
        }
    };

    int dir = (r >= max_r) ? -1 : 1;
    if (r >= max_r) {
        advance_two();
    }
    for (int g = 0; g < 400 && c != stop_col && c != to.second; ++g) {
        if ((dir == 1 && r >= max_r) || (dir == -1 && r <= min_r)) {
            advance_two();
            dir = (dir == 1) ? -1 : 1;
            continue;
        }
        r += dir;
        path.emplace_back(r, c);
    }
    while (r != to.first) {
        r += (r < to.first) ? 1 : -1;
        path.emplace_back(r, c);
    }
    if (c != to.second) {
        c = to.second;
        path.emplace_back(r, c);
    }
    return path;
}

SPAFarmTaskPlugin::StageTrafficMap SPAFarmTaskPlugin::build_stage_traffic(std::string_view stage_id, bool bottom_only) const
{
    // Exact 1:1 port of `Rr(a, s)` in service.pretty.mjs lines 5729-5847
    StageTrafficMap tm;
    const auto* st = find_stage(stage_id);
    if (!st || st->rows.empty()) {
    
    return tm;
    }
    tm.stage_id = st->stage_id;
    tm.grid = st->rows;
    tm.rows = static_cast<int>(tm.grid.size());
    tm.cols = static_cast<int>(tm.grid[0].size());

    // 地块覆盖（`wr`）已在 `load_all_data` 里按 JS L7529-7543 应用（只有 4 处：4,9 / 4,8 → a，3,10 / 2,10 → c）
    for (int r = 0; r < tm.rows; ++r) {
        for (int c = 0; c < tm.cols; ++c) {
            if (tm.grid[r][c] == 'S') {
                tm.spawns.emplace_back(r, c);
            }
            else if (tm.grid[r][c] == 'E') {
                tm.goals.emplace_back(r, c);
            }
        }
    }

    auto is_road = [](char ch) {
        return ch == 'b' || ch == 'c' || ch == 'S' || ch == 'E' || ch == 'I' || ch == 'O';
    };

    const int INF = 1000000;
    auto bfs_dist = [&](bool ground_only) {
        std::vector<std::vector<int>> dist(tm.rows, std::vector<int>(tm.cols, INF));
        std::queue<std::pair<int, int>> q;
        for (const auto& g : tm.goals) {
            dist[g.first][g.second] = 0;
            q.push(g);
        }
        const int dirs[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
        while (!q.empty()) {
            auto [r, c] = q.front();
            q.pop();
            for (const auto& d : dirs) {
                int nr = r + d[0];
                int nc = c + d[1];
                if (nr >= 0 && nr < tm.rows && nc >= 0 && nc < tm.cols) {
                    if (ground_only && !is_road(tm.grid[nr][nc])) {
                        continue;
                    }
                    if (dist[nr][nc] > dist[r][c] + 1) {
                        dist[nr][nc] = dist[r][c] + 1;
                        q.push({ nr, nc });
                    }
                }
            }
        }
        return dist;
    };

    tm.ground_dist = bfs_dist(true);
    tm.air_dist = bfs_dist(false);
    tm.ground_traffic.assign(tm.rows, std::vector<double>(tm.cols, 0.0));
    tm.air_traffic.assign(tm.rows, std::vector<double>(tm.cols, 0.0));
    tm.early_ground_traffic.assign(tm.rows, std::vector<double>(tm.cols, 0.0));
    tm.ground_first_step.assign(tm.rows, std::vector<double>(tm.cols, 0.0));

    const int dirs[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
    for (const auto& sp : tm.spawns) {
        SpawnPathInfo pinfo;
        pinfo.spawn = sp;
        if (tm.ground_dist[sp.first][sp.second] < INF) {
            std::vector<std::vector<double>> flow(tm.rows, std::vector<double>(tm.cols, 0.0));
            flow[sp.first][sp.second] = 1.0;
            std::vector<std::pair<int, int>> order;
            for (int r = 0; r < tm.rows; ++r) {
                for (int c = 0; c < tm.cols; ++c) {
                    if (tm.ground_dist[r][c] <= tm.ground_dist[sp.first][sp.second]) {
                        order.emplace_back(r, c);
                    }
                }
            }
            std::sort(order.begin(), order.end(), [&](const auto& a, const auto& b) {
                return tm.ground_dist[a.first][a.second] > tm.ground_dist[b.first][b.second];
            });
            const int start_d = tm.ground_dist[sp.first][sp.second];
            int road_tile_cnt = 0;
            for (const auto& [r, c] : order) {
                double f = flow[r][c];
                if (f <= 0.0) {
                    continue;
                }
                int step_from_spawn = start_d - tm.ground_dist[r][c];
                std::string key = std::to_string(r) + "," + std::to_string(c);
                pinfo.ground_flow[key] = f;
                pinfo.ground_progress[key] = step_from_spawn;
                if (tm.grid[r][c] != 'S' && tm.grid[r][c] != 'E') {
                    ++road_tile_cnt;
                    if (pinfo.first_tile.empty()) {
                        pinfo.first_tile = key;
                    }
                }
                tm.ground_traffic[r][c] += f;
                if (step_from_spawn == 1) {
                    tm.ground_first_step[r][c] += f;
                }
                if (step_from_spawn >= 1 && step_from_spawn <= 3) {
                    tm.early_ground_traffic[r][c] += f * (4.0 - step_from_spawn) / 3.0;
                }
                std::vector<std::pair<int, int>> nexts;
                for (const auto& d : dirs) {
                    int nr = r + d[0], nc = c + d[1];
                    if (nr >= 0 && nr < tm.rows && nc >= 0 && nc < tm.cols
                        && tm.ground_dist[nr][nc] == tm.ground_dist[r][c] - 1) {
                        nexts.emplace_back(nr, nc);
                    }
                }
                for (const auto& nx : nexts) {
                    flow[nx.first][nx.second] += f / static_cast<double>(nexts.size());
                }
            }
            pinfo.is_single_tile = (road_tile_cnt <= 1);
        }

        if (!tm.goals.empty()) {
            auto g = tm.goals.front();
            int steps = std::max(1, std::max(std::abs(g.first - sp.first), std::abs(g.second - sp.second)) * 2);
            for (int k = 0; k <= steps; ++k) {
                double t = static_cast<double>(k) / steps;
                int r = std::clamp(static_cast<int>(std::lround(sp.first + (g.first - sp.first) * t)), 0, tm.rows - 1);
                int c = std::clamp(static_cast<int>(std::lround(sp.second + (g.second - sp.second) * t)), 0, tm.cols - 1);
                tm.air_traffic[r][c] += 1.0 / (steps + 1);
            }
        }
        tm.paths.push_back(std::move(pinfo));
    }

    const int n_spawns = std::max(1, static_cast<int>(tm.spawns.size()));
    for (int r = 0; r < tm.rows; ++r) {
        for (int c = 0; c < tm.cols; ++c) {
            tm.ground_traffic[r][c] /= n_spawns;
            tm.air_traffic[r][c] /= n_spawns;
            tm.early_ground_traffic[r][c] /= n_spawns;
            tm.ground_first_step[r][c] /= n_spawns;
            tm.max_ground_traffic = std::max(tm.max_ground_traffic, tm.ground_traffic[r][c]);
            tm.max_air_traffic = std::max(tm.max_air_traffic, tm.air_traffic[r][c]);
            std::string key = std::to_string(r) + "," + std::to_string(c);
            if (tm.ground_traffic[r][c] >= 0.9 && tm.grid[r][c] == 'b') {
                tm.chokepoints.insert(key);
                tm.road_importance[key] = 1.5;
            }
            else if (tm.ground_traffic[r][c] > 0.0) {
                tm.road_importance[key] = 1.0;
            }
        }
    }
    // ===== `Rr(a, s)` —— service.pretty.mjs L5748-5822 =====
    {
        const int R = tm.rows;
        const int C = tm.cols;
        if (R > 0 && C > 0) {
            const auto& rows = tm.grid;
            const auto goals = cells_of(rows, 'E');
            const BfsResult from_goal = bfs_from(rows, goals);

            std::vector<std::pair<int, int>> starts;
            for (const auto& c : cells_of(rows, 'S')) {
                if (from_goal.dist[c.first][c.second] < INF) {
                    starts.push_back(c);
                }
            }
            if (bottom_only && starts.size() > 1) {
                int max_r = -1;
                for (const auto& c : starts) {
                    max_r = std::max(max_r, c.first);
                }
                std::vector<std::pair<int, int>> filtered;
                for (const auto& c : starts) {
                    if (c.first == max_r) {
                        filtered.push_back(c);
                    }
                }
                starts.swap(filtered);
            }
            if (starts.empty()) {
                const auto all = cells_of(rows, 'S');
                int min_c = std::numeric_limits<int>::max();
                for (const auto& c : all) {
                    min_c = std::min(min_c, c.second);
                }
                for (const auto& c : all) {
                    if (c.second == min_c) {
                        starts.push_back(c);
                    }
                }
            }

            tm.ground.assign(R, std::vector<double>(C, 0.0));
            tm.ground_progress.assign(R, std::vector<double>(C, 0.0));
            tm.air.assign(R, std::vector<double>(C, 0.0));
            tm.air_progress.assign(R, std::vector<double>(C, 0.0));
            tm.ground_paths.clear();
            tm.air_paths.clear();
            tm.ground_by_start.clear();

            for (const auto& sp : starts) {
                const int b = from_goal.dist[sp.first][sp.second];
                if (b < INF) {
                    const BfsResult from_start = bfs_from(rows, { sp });
                    const int total_ways = from_goal.ways[sp.first][sp.second];
                    std::vector<std::vector<double>> me(R, std::vector<double>(C, 0.0));
                    for (int r = 0; r < R; ++r) {
                        for (int c = 0; c < C; ++c) {
                            if (from_start.dist[r][c] >= INF || from_goal.dist[r][c] >= INF) {
                                continue;
                            }
                            if (from_start.dist[r][c] + from_goal.dist[r][c] != b || total_ways <= 0) {
                                continue;
                            }
                            me[r][c] = static_cast<double>(from_start.ways[r][c])
                                * from_goal.ways[r][c] / static_cast<double>(total_ways);
                            tm.ground[r][c] += me[r][c];
                            tm.ground_progress[r][c] = std::max(
                                tm.ground_progress[r][c],
                                static_cast<double>(from_start.dist[r][c]) / static_cast<double>(b));
                        }
                    }
                    tm.ground_by_start.push_back(me);

                    // groundPaths：从 S 沿 dist 递减走到 E
                    std::vector<std::pair<int, int>> path { sp };
                    int cr = sp.first;
                    int cc = sp.second;
                    while (from_goal.dist[cr][cc] > 0) {
                        static const int kDirs[4][2] = { { 0, -1 }, { 1, 0 }, { -1, 0 }, { 0, 1 } };
                        bool moved = false;
                        for (const auto& d : kDirs) {
                            const int nr = cr + d[0];
                            const int nc = cc + d[1];
                            if (nr < 0 || nc < 0 || nr >= R || nc >= C) {
                                continue;
                            }
                            if (from_goal.dist[nr][nc] == from_goal.dist[cr][cc] - 1) {
                                cr = nr;
                                cc = nc;
                                path.emplace_back(cr, cc);
                                moved = true;
                                break;
                            }
                        }
                        if (!moved) {
                            break;
                        }
                    }
                    tm.ground_paths.push_back(path);
                }

                // airPaths：最近的 E 用 `Hi` 生成路径
                if (!goals.empty()) {
                    auto nearest = goals.front();
                    for (const auto& g : goals) {
                        const int d1 = std::abs(g.first - sp.first) + std::abs(g.second - sp.second);
                        const int d0 = std::abs(nearest.first - sp.first) + std::abs(nearest.second - sp.second);
                        if (d1 < d0) {
                            nearest = g;
                        }
                    }
                    const auto air_path = build_path(rows, sp, nearest);
                    if (!air_path.empty()) {
                        const double denom = static_cast<double>(air_path.size() - 1);
                        for (size_t i = 0; i < air_path.size(); ++i) {
                            const auto [r, c] = air_path[i];
                            if (r < 0 || c < 0 || r >= R || c >= C) {
                                continue;
                            }
                            tm.air[r][c] += 1.0;
                            if (denom > 0) {
                                tm.air_progress[r][c] =
                                    std::max(tm.air_progress[r][c], static_cast<double>(i) / denom);
                            }
                        }
                        tm.air_paths.push_back(air_path);
                    }
                }
            }

            // grazeFrom：路径拐角外扩
            tm.graze_from.assign(R, std::vector<std::optional<std::pair<int, int>>>(C, std::nullopt));
            for (const auto& path : tm.ground_paths) {
                for (size_t i = 1; i + 1 < path.size(); ++i) {
                    const auto prev = path[i - 1];
                    const auto cur = path[i];
                    const auto next = path[i + 1];
                    if (prev.first == next.first || prev.second == next.second) {
                        continue;
                    }
                    const int gr = prev.first + next.first - cur.first;
                    const int gc = prev.second + next.second - cur.second;
                    if (gr < 0 || gc < 0 || gr >= R || gc >= C) {
                        continue;
                    }
                    if (tm.ground[gr][gc] > 0.0) {
                        continue;
                    }
                    tm.graze_from[gr][gc] = cur;
                }
            }
        }
    }

    return tm;
}

SPAFarmTaskPlugin::GrazeStats SPAFarmTaskPlugin::compute_cell_graze(
    const StageTrafficMap& traffic,
    const OperatorInfo& op,
    int r,
    int c,
    std::string_view facing) const
{
    GrazeStats gs;
    for (const auto& off : op.base_range) {
        auto [dr, dc] = rotate_offset(off, facing);
        int tr = r + dr;
        int tc = c + dc;
        if (tr >= 0 && tr < traffic.rows && tc >= 0 && tc < traffic.cols) {
            gs.ground += traffic.ground_traffic[tr][tc];
            if (op.position == "RANGED" || op.base_range.size() > 2) {
                gs.air += traffic.air_traffic[tr][tc];
            }
            gs.early_ground += traffic.early_ground_traffic[tr][tc];
            gs.first_step += traffic.ground_first_step[tr][tc];
        }
    }
    return gs;
}

SPAFarmTaskPlugin::PlacementEvalResult SPAFarmTaskPlugin::evaluate_placement_nt(
    const std::vector<PlacementAssign>& placement,
    const std::vector<UnitSlot>& bench_units,
    const StageTrafficMap& traffic,
    const PlacementEvalOptions& opts) const
{
    // `Nt(a, s, e)` —— service.pretty.mjs L5862-6301，逐项对齐
    // 变量名沿用 JS：f=focusBonds, w=supportWeight.defense, i=supportWeight.heal,
    //                M=defense, C=exposed, T=advanceScore, me=gate, Z=stacking
    PlacementEvalResult out;
    const auto& rows = traffic.grid;
    const int R = traffic.rows;
    const int Cn = traffic.cols;
    if (R <= 0 || Cn <= 0 || placement.empty()) {
        return out;
    }

    constexpr double kGi = 0.15;   // `Gi`
    constexpr double kWi = 0.3;    // `Wi`
    constexpr double kXi = 2.0;    // `Xi`
    constexpr double kVi = 0.5;    // `Vi`
    constexpr double kInfDmg = 1.44; // `sa.dmg`
    constexpr double kInfDrain = 70.0; // `sa.drain`
    constexpr double kKi = 45.0;
    constexpr double kJi = 0.25;
    constexpr double kQi = 12.0;
    constexpr double kYi = 60.0;
    constexpr double kZi = 30.0;
    [[maybe_unused]] constexpr double kEs = 1.5;
    constexpr double kFn = 12.0;
    constexpr double kRs = 1500.0;
    static constexpr double kTs[5] = { 1.0, 0.75, 0.5, 0.25, 0.1 };

    const double w = opts.support_defense_weight;
    const double heal_w = opts.support_heal_weight;
    const std::unordered_set<std::string>& focus = opts.focus_bonds;

    // ---- 单位视图（`a.units` / `s`）----
    struct UnitView
    {
        int uid = -1;
        const OperatorInfo* op = nullptr;
        int r = -1;
        int c = -1;
        std::string facing = "RIGHT";
        bool elite = false;
    };
    std::vector<UnitView> all_units;
    std::vector<int> placed_idx;
    for (const auto& pa : placement) {
        UnitView uv;
        uv.uid = static_cast<int>(all_units.size());
        uv.op = find_operator(pa.name);
        uv.r = pa.r;
        uv.c = pa.c;
        uv.facing = pa.facing.empty() ? "RIGHT" : pa.facing;
        uv.elite = pa.elite;
        if (uv.op != nullptr) {
            placed_idx.push_back(uv.uid);
        }
        all_units.push_back(uv);
    }
    for (const auto& bu : bench_units) {
        UnitView uv;
        uv.uid = static_cast<int>(all_units.size());
        uv.op = find_operator(bu.name);
        uv.r = -1;
        uv.c = -1;
        if (uv.op != nullptr) {
            all_units.push_back(uv);
        }
    }
    auto d = [&](int uid) -> const OperatorInfo* {
        for (const auto& u : all_units) {
            if (u.uid == uid) {
                return u.op;
            }
        }
        return nullptr;
    };

    // ---- 基础工具 ----
    auto is_ranged_class = [](const OperatorInfo& op) {
        if (op.position == "RANGED") {
            return true;
        }
        return op.profession == "SNIPER" || op.profession == "CASTER"
            || op.profession == "SUPPORT" || op.profession == "MEDIC";
    };
    auto trait_cap = [](const OperatorTrait& tr) -> int {
        if (auto m = regex_first_group(tr.text, "至多叠加([0-9]+)层")) {
            return std::stoi(*m);
        }
        return 0;
    };
    auto trait_per_battle = [](const OperatorTrait& tr) {
        return tr.text.find("每场战斗") != std::string::npos;
    };
    auto tankiness = [](const OperatorInfo& op) {
        return std::max(0.4, std::min(3.0, op.max_hp / 1500.0 * (1.0 + op.def / 400.0)));
    };
    auto sell_value = [&](const OperatorInfo& op) {
        for (const auto& tr : op.traits) {
            if (tr.text.find("自身售出价格+1") != std::string::npos) {
                return 8;
            }
        }
        return op.sell_price > 0 ? op.sell_price : 1;
    };
    auto chess_value = [](const OperatorInfo& op) {
        return 200.0 * (1.0 + 0.25 * (std::max(1, op.tier) - 1)) * (op.golden ? 1.4 : 1.0);
    };
    auto has_focus_bond = [&](const OperatorInfo& op) {
        for (const auto& b : op.bond_ids) {
            if (focus.count(b) > 0) {
                return true;
            }
        }
        return false;
    };
    auto leak_factor = [&](int uid) {
        const auto* op = d(uid);
        const int block = op != nullptr ? op->block_cnt : 1;
        return kTs[std::min(4, std::max(0, block))];
    };
    auto at = [&](int r, int c) -> char {
        if (r < 0 || c < 0 || r >= R || c >= Cn) {
            return '\0';
        }
        return rows[r][c];
    };

    double M = 0.0;
    double C = 0.0;
    double T = 0.0;
    double me = 0.0;

    // `Be`：站在地面路线上的近战
    std::vector<int> Be;
    for (int uid : placed_idx) {
        const auto& u = all_units[uid];
        if (at(u.r, u.c) == 'b' && traffic.ground[u.r][u.c] > 0.0) {
            Be.push_back(uid);
        }
    }

    // ---- advance：输出位比保护它的近战更靠前 ----
    if (opts.advance && (opts.round <= 0 ? 1 : opts.round) >= 4) {
        bool has_melee = false;
        for (int uid : placed_idx) {
            const auto* op = d(uid);
            if (op != nullptr && !is_ranged_class(*op)) {
                has_melee = true;
                break;
            }
        }
        if (has_melee) {
            std::vector<std::pair<int, int>> starts;
            for (const auto& p : traffic.ground_paths) {
                if (!p.empty()) {
                    starts.push_back(p.front());
                }
            }
            auto near_start = [&](const UnitView& u) {
                return opts.avoid_right_col
                    && std::any_of(starts.begin(), starts.end(), [&](const std::pair<int, int>& s) {
                           return std::abs(s.first - u.r) + std::abs(s.second - u.c) == 1;
                       });
            };
            std::vector<int> melee_ids;
            for (int uid : placed_idx) {
                const auto* op = d(uid);
                if (op != nullptr && !is_ranged_class(*op) && !near_start(all_units[uid])) {
                    melee_ids.push_back(uid);
                }
            }
            for (int uid : placed_idx) {
                const auto& u = all_units[uid];
                const auto* op = d(uid);
                if (op == nullptr || !is_ranged_class(*op) || at(u.r, u.c) == 'h') {
                    continue;
                }
                if (starts.empty()) {
                    continue;
                }
                auto nearest = *std::min_element(starts.begin(), starts.end(), [&](const auto& a, const auto& b) {
                    return std::abs(a.first - u.r) + std::abs(a.second - u.c)
                        < std::abs(b.first - u.r) + std::abs(b.second - u.c);
                });
                const int dist = std::abs(nearest.first - u.r) + std::abs(nearest.second - u.c);
                const bool shielded = std::any_of(melee_ids.begin(), melee_ids.end(), [&](int mid) {
                    const auto& mu = all_units[mid];
                    return std::abs(mu.r - u.r) <= 2
                        && std::abs(nearest.first - mu.r) + std::abs(nearest.second - mu.c) < dist;
                });
                if (!shielded) {
                    C += 0.5;
                }
            }
        }
    }

    // ---- N：某格被近战堵住后的有效流量 ----
    auto block_value = [&](int r, int c, int self_uid) {
        if (r < 0 || c < 0 || r >= R || c >= Cn) {
            return 0.0;
        }
        const double base = traffic.ground[r][c];
        if (base <= 0.0) {
            return 0.0;
        }
        const char ch = rows[r][c];
        if (ch == 'S' || ch == 'E') {
            return 0.0;
        }
        double v = base;
        const double prog = traffic.ground_progress[r][c];
        for (int uid : Be) {
            if (uid == self_uid) {
                continue;
            }
            const auto& bu = all_units[uid];
            if (bu.r == r && bu.c == c) {
                v *= 2.0;
                continue;
            }
            if (traffic.ground_progress[bu.r][bu.c] < prog) {
                v *= 1.0 - std::min(1.0, traffic.ground[bu.r][bu.c]) * (1.0 - leak_factor(uid));
            }
        }
        return v;
    };

    // ---- U：路口承担的分流压力 ----
    std::unordered_map<std::string, double> U;
    for (int uid : Be) {
        const auto& u = all_units[uid];
        const auto* op = d(uid);
        if (op == nullptr || is_ranged_class(*op)) {
            continue;
        }
        const double prog = traffic.ground_progress[u.r][u.c];
        std::vector<std::pair<int, int>> ups;
        static const int kDirs[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
        for (const auto& dir : kDirs) {
            const int nr = u.r + dir[0];
            const int nc = u.c + dir[1];
            if (nr < 0 || nc < 0 || nr >= R || nc >= Cn) {
                continue;
            }
            if (traffic.ground[nr][nc] <= 0.0 || traffic.ground_progress[nr][nc] >= prog) {
                continue;
            }
            const char ch = rows[nr][nc];
            if (ch == 'S' || ch == 'E') {
                continue;
            }
            ups.emplace_back(nr, nc);
        }
        auto add = [&](int r, int c, double v) {
            U[std::to_string(r) + "," + std::to_string(c)] += v;
        };
        add(u.r, u.c, ups.empty() ? 1.0 : 0.0);
        for (const auto& p : ups) {
            add(p.first, p.second, kXi / static_cast<double>(ups.size()));
        }
    }

    // ---- q：按「是否在某条路线的接敌点之后」给格加权 ----
    std::unordered_map<std::string, double> q;
    {
        auto near_any = [&](int r, int c) {
            for (int uid : placed_idx) {
                const auto& u = all_units[uid];
                if (std::abs(u.r - r) + std::abs(u.c - c) <= 1) {
                    return true;
                }
            }
            return false;
        };
        std::unordered_map<std::string, std::vector<double>> acc;
        for (const auto& path : traffic.ground_paths) {
            int engage = -1;
            for (size_t i = 0; i < path.size(); ++i) {
                if (near_any(path[i].first, path[i].second)) {
                    engage = static_cast<int>(i);
                }
            }
            for (size_t i = 0; i < path.size(); ++i) {
                const std::string key = std::to_string(path[i].first) + "," + std::to_string(path[i].second);
                acc[key].push_back((engage >= 0 && static_cast<int>(i) > engage) ? kWi : 1.0);
            }
        }
        for (const auto& [key, vec] : acc) {
            double sum = 0.0;
            for (double v : vec) {
                sum += v;
            }
            q[key] = sum / static_cast<double>(vec.size());
        }
    }

    // ---- z/P：单格价值（带缓存）----
    std::unordered_map<int, double> z_cache;
    auto cell_value_raw = [&](int r, int c) -> double {
        const double base = traffic.ground[r][c];
        if (base <= 0.0) {
            return 0.0;
        }
        const char ch = rows[r][c];
        if (ch == 'S' || ch == 'E') {
            return 0.0;
        }
        double factor = 1.0;
        const double prog = traffic.ground_progress[r][c];
        for (int uid : Be) {
            const auto& bu = all_units[uid];
            if (bu.r == r && bu.c == c) {
                continue;
            }
            if (traffic.ground_progress[bu.r][bu.c] < prog) {
                factor *= 1.0 - std::min(1.0, traffic.ground[bu.r][bu.c]) * (1.0 - leak_factor(uid));
            }
        }
        const std::string key = std::to_string(r) + "," + std::to_string(c);
        auto uit = U.find(key);
        auto qit = q.find(key);
        return base * std::max(kGi, factor)
            * (1.0 + (uit != U.end() ? uit->second : 0.0))
            * (qit != q.end() ? qit->second : 1.0);
    };
    auto z = [&](int r, int c) -> double {
        const int key = r * 100 + c;
        auto it = z_cache.find(key);
        if (it != z_cache.end()) {
            return it->second;
        }
        const double v = cell_value_raw(r, c);
        z_cache[key] = v;
        return v;
    };

    // ---- 治疗覆盖表 Ee + 派生判定 ----
    std::unordered_map<std::string, std::vector<int>> heal_cover;
    for (int uid : placed_idx) {
        const auto& u = all_units[uid];
        const auto* op = d(uid);
        if (op == nullptr) {
            continue;
        }
        if (op->profession != "MEDIC" && !op->heals_allies) {
            continue;
        }
        for (const auto& off : op->base_range) {
            const auto [dr, dc] = rotate_offset(off, u.facing);
            heal_cover[std::to_string(u.r + dr) + "," + std::to_string(u.c + dc)].push_back(uid);
        }
    }
    auto covered_by_other = [&](int uid) {
        const auto& u = all_units[uid];
        const std::string key = std::to_string(u.r) + "," + std::to_string(u.c);
        auto it = heal_cover.find(key);
        if (it == heal_cover.end()) {
            return false;
        }
        for (int other : it->second) {
            if (other != uid) {
                return true;
            }
        }
        return false;
    };
    std::set<std::string> infection;
    if (const auto* stage = find_stage(traffic.stage_id)) {
        for (const auto& t : stage->infection_tiles) {
            infection.insert(std::to_string(t.first) + "," + std::to_string(t.second));
        }
    }
    auto on_infection = [&](int uid) {
        const auto& u = all_units[uid];
        return u.r >= 0 && infection.count(std::to_string(u.r) + "," + std::to_string(u.c)) > 0;
    };
    auto has_stack_trait4 = [](const OperatorInfo& op) {
        for (const auto& tr : op.traits) {
            bool on_deploy = false;
            for (const auto& tag : tr.tags) {
                if (tag == "onDeploy") {
                    on_deploy = true;
                }
            }
            if (on_deploy && tr.text.find("每场战斗") != std::string::npos) {
                if (auto m = regex_first_group(tr.text, "至多叠加([0-9]+)层")) {
                    if (std::stoi(*m) >= 4) {
                        return true;
                    }
                }
            }
        }
        return false;
    };
    auto has_medic_cover = [&](int uid) {
        const auto& u = all_units[uid];
        auto it = heal_cover.find(std::to_string(u.r) + "," + std::to_string(u.c));
        if (it == heal_cover.end()) {
            return false;
        }
        for (int other : it->second) {
            const auto* op = d(other);
            if (other != uid && op != nullptr && op->profession == "MEDIC") {
                return true;
            }
        }
        return false;
    };
    auto survives_infection = [&](int uid) {
        const auto* op = d(uid);
        if (op == nullptr) {
            return false;
        }
        return op->self_heal || has_stack_trait4(*op) || has_medic_cover(uid);
    };
    auto infection_uptime = [&](int uid) {
        if (!on_infection(uid)) {
            return 1.0;
        }
        if (survives_infection(uid)) {
            return 1.0;
        }
        const auto* op = d(uid);
        const double hp = op != nullptr && op->max_hp > 0 ? op->max_hp : 1200.0;
        return std::min(1.0, hp / (kInfDrain * kKi));
    };
    std::unordered_map<int, double> dmg_taken;
    auto damage_factor = [&](int uid) {
        auto it = dmg_taken.find(uid);
        if (it != dmg_taken.end()) {
            return it->second;
        }
        double v = 1.0;
        if (on_infection(uid)) {
            v = kInfDmg * infection_uptime(uid);
        }
        dmg_taken[uid] = v;
        return v;
    };

    // ---- 阻挡收益 ----
    std::vector<std::vector<double>> S(R, std::vector<double>(Cn, 0.0));
    std::vector<std::vector<double>> ae(R, std::vector<double>(Cn, 0.0));
    std::unordered_map<int, double> stage_cover;

    for (int uid : Be) {
        const auto& u = all_units[uid];
        const auto* op = d(uid);
        if (op == nullptr) {
            continue;
        }
        const double bval = block_value(u.r, u.c, uid) * (1.0 - leak_factor(uid));
        stage_cover[uid] = bval;
        if (is_ranged_class(*op)) {
            double on_road = 0.0;
            double behind_melee = 0.0;
            for (const auto& start_mat : traffic.ground_by_start) {
                const double here = std::min(1.0, start_mat[u.r][u.c]);
                if (here <= 0.0) {
                    continue;
                }
                const bool shielded = std::any_of(Be.begin(), Be.end(), [&](int other) {
                    if (other == uid) {
                        return false;
                    }
                    const auto& ou = all_units[other];
                    const auto* oop = d(other);
                    return oop != nullptr && !is_ranged_class(*oop)
                        && start_mat[ou.r][ou.c] > 0.0
                        && traffic.ground_progress[ou.r][ou.c] < traffic.ground_progress[u.r][u.c];
                });
                if (shielded) {
                    behind_melee += here;
                }
                else {
                    on_road += here;
                }
            }
            M += bval * 70.0 * 0.3 - 110.0 * on_road - 20.0 * behind_melee;
            C += on_road;
            continue;
        }
        M += bval * 70.0 * std::min(1.8, 0.4 + 0.6 * tankiness(*op) * w) * damage_factor(uid);
    }

    // ---- 身前的近战更耐打？ ----
    for (const auto& start_mat : traffic.ground_by_start) {
        std::vector<int> on_route;
        for (int uid : Be) {
            const auto& u = all_units[uid];
            if (start_mat[u.r][u.c] > 0.0) {
                on_route.push_back(uid);
            }
        }
        std::sort(on_route.begin(), on_route.end(), [&](int a, int b) {
            const auto& ua = all_units[a];
            const auto& ub = all_units[b];
            return traffic.ground_progress[ua.r][ua.c] < traffic.ground_progress[ub.r][ub.c];
        });
        for (size_t i = 0; i < on_route.size(); ++i) {
            for (size_t j = i + 1; j < on_route.size(); ++j) {
                const int front = on_route[i];
                const int back = on_route[j];
                const auto* of = d(front);
                const auto* ob = d(back);
                if (of == nullptr || ob == nullptr) {
                    continue;
                }
                int back_range = 0;
                for (const auto& off : ob->base_range) {
                    back_range = std::max(back_range, off.first);
                }
                if (back_range >= 2 || has_stack_trait4(*of)) {
                    continue;
                }
                const double diff = ob->block_cnt * tankiness(*ob) - of->block_cnt * tankiness(*of);
                if (diff > 0.0) {
                    M -= 25.0 * std::min(4.0, diff);
                }
            }
        }
    }

    // ---- 感染地块 / 叠层 ----
    double stacking = 0.0;
    for (int uid : placed_idx) {
        const auto* op = d(uid);
        if (op == nullptr) {
            continue;
        }
        const double mul = has_focus_bond(*op) ? 2.0 : 1.0;
        if (has_stack_trait4(*op) && covered_by_other(uid)) {
            stacking -= 50.0 * mul;
        }
        if (!on_infection(uid)) {
            continue;
        }
        if (has_stack_trait4(*op)) {
            stacking += 100.0 * mul;
            continue;
        }
        if (survives_infection(uid)) {
            continue;
        }
        (void)damage_factor(uid);
    }

    // ---- 近战范围极小且不在路线上 ----
    for (int uid : placed_idx) {
        const auto* op = d(uid);
        if (op == nullptr) {
            continue;
        }
        if (op->position == "MELEE" && op->profession != "TANK"
            && std::find(Be.begin(), Be.end(), uid) == Be.end()
            && op->base_range.size() <= 2) {
            M -= 60.0;
        }
    }
    // ---- 重装不在路线上 ----
    for (int uid : placed_idx) {
        const auto* op = d(uid);
        if (op != nullptr && op->profession == "TANK" && std::find(Be.begin(), Be.end(), uid) == Be.end()) {
            M -= 80.0 * w;
        }
    }

    // ---- 推进分 / 覆盖 ----
    double heal_sum = 0.0;
    for (int uid : placed_idx) {
        const auto& u = all_units[uid];
        const auto* op = d(uid);
        if (op == nullptr) {
            continue;
        }
        // `T`：朝向最近出生点
        if (!traffic.ground_paths.empty()) {
            std::vector<std::pair<int, int>> starts;
            for (const auto& p : traffic.ground_paths) {
                if (!p.empty()) {
                    starts.push_back(p.front());
                }
            }
            if (!starts.empty()) {
                auto nearest = *std::min_element(starts.begin(), starts.end(), [&](const auto& a, const auto& b) {
                    return std::abs(a.first - u.r) + std::abs(a.second - u.c)
                        < std::abs(b.first - u.r) + std::abs(b.second - u.c);
                });
                const int lane = (nearest.second > u.c) ? std::min(u.c, Cn - 4) : std::max(u.c, 0 + 3);
                const int dr = nearest.first - u.r;
                const int dc = nearest.second - lane;
                const int dist = std::abs(dr) + std::abs(dc);
                static const std::unordered_map<std::string, std::pair<int, int>> kDir = {
                    { "RIGHT", { 0, 1 } }, { "LEFT", { 0, -1 } }, { "UP", { -1, 0 } }, { "DOWN", { 1, 0 } },
                };
                std::pair<int, int> fdir = { 0, 1 };
                if (auto it = kDir.find(u.facing); it != kDir.end()) {
                    fdir = it->second;
                }
                T += (20 - dist) * 4.0
                    + (fdir.first * dr + fdir.second * dc) / static_cast<double>(std::max(1, dist)) * 6.0;
            }
        }
        // 面朝方向还有更靠近 E 的格
        if (traffic.ground[u.r][u.c] > 0.0) {
            const double prog = traffic.ground_progress[u.r][u.c];
            static const std::unordered_map<std::string, std::pair<int, int>> kDir2 = {
                { "RIGHT", { 0, 1 } }, { "LEFT", { 0, -1 } }, { "UP", { -1, 0 } }, { "DOWN", { 1, 0 } },
            };
            if (auto it = kDir2.find(u.facing); it != kDir2.end()) {
                const int nr = u.r + it->second.first;
                const int nc = u.c + it->second.second;
                if (nr >= 0 && nc >= 0 && nr < R && nc < Cn && traffic.ground[nr][nc] > 0.0
                    && traffic.ground_progress[nr][nc] < prog) {
                    M += 1.0;
                }
            }
        }

        int cover_cnt = 0;
        int heal_cnt = 0;
        for (const auto& off : op->base_range) {
            const auto [dr, dc] = rotate_offset(off, u.facing);
            const int tr = u.r + dr;
            const int tc = u.c + dc;
            if (tr < 0 || tc < 0 || tr >= R || tc >= Cn) {
                continue;
            }
            if (op->profession == "MEDIC" || op->heals_allies) {
                bool counted = false;
                for (int other : placed_idx) {
                    const auto& ou = all_units[other];
                    if (other == uid || ou.r != tr || ou.c != tc) {
                        continue;
                    }
                    const auto* oop = d(other);
                    if (oop == nullptr || has_stack_trait4(*oop)) {
                        continue;
                    }
                    auto cit = stage_cover.find(other);
                    if (cit != stage_cover.end()) {
                        heal_sum += cit->second;
                        ++heal_cnt;
                        counted = true;
                    }
                    break;
                }
                (void)counted;
                if (op->profession == "MEDIC") {
                    continue;
                }
            }
            if (z(tr, tc) > 0.0) {
                S[tr][tc] += chess_value(*op) * damage_factor(uid);
                ++cover_cnt;
            }
            else if (tr < R && tc < Cn && traffic.graze_from[tr][tc].has_value()) {
                ae[tr][tc] += chess_value(*op) * damage_factor(uid);
            }
        }
    }

    // ---- 汇总阻挡收益 ----
    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < Cn; ++c) {
            if (S[r][c] != 0.0) {
                M += S[r][c] / 20.0 * z(r, c) * (1.1 - 0.2 * traffic.ground_progress[r][c]);
            }
            const auto& graze = traffic.graze_from[r][c];
            if (ae[r][c] != 0.0 && graze.has_value()) {
                M += kVi * (ae[r][c] / 20.0) * z(graze->first, graze->second)
                    * (1.1 - 0.2 * traffic.ground_progress[graze->first][graze->second]);
            }
        }
    }
    M += heal_sum * 70.0 * heal_w;

    // ---- 治疗者覆盖队友 ----
    for (int uid : placed_idx) {
        const auto& u = all_units[uid];
        const auto* op = d(uid);
        if (op == nullptr) {
            continue;
        }
        if (op->profession != "MEDIC" && !op->heals_allies) {
            continue;
        }
        const double mul = (op->profession == "MEDIC") ? 1.0 : kJi;
        int cnt = 0;
        for (int other : placed_idx) {
            if (other == uid) {
                continue;
            }
            const auto& ou = all_units[other];
            if (has_stack_trait4(*d(other))) {
                continue;
            }
            for (const auto& off : op->base_range) {
                const auto [dr, dc] = rotate_offset(off, u.facing);
                if (ou.r == u.r + dr && ou.c == u.c + dc) {
                    ++cnt;
                    break;
                }
            }
        }
        if (cnt > 0) {
            M += kQi * mul * cnt * heal_w;
        }
    }

    // ---- 路线无人防守 ----
    std::vector<int> unguarded;
    for (size_t si = 0; si < traffic.ground_by_start.size(); ++si) {
        const auto& mat = traffic.ground_by_start[si];
        double survive = 1.0;
        double covered = 0.0;
        for (int uid : Be) {
            const auto& u = all_units[uid];
            if (mat[u.r][u.c] > 0.0) {
                survive *= 1.0 - std::min(1.0, mat[u.r][u.c]) * (1.0 - leak_factor(uid));
            }
        }
        for (int r = 0; r < R; ++r) {
            for (int c = 0; c < Cn; ++c) {
                if (mat[r][c] > 0.0) {
                    covered += S[r][c] * std::min(1.0, mat[r][c]);
                }
            }
        }
        const double risk = survive * std::max(0.0, 1.0 - covered / kRs);
        if (risk > 0.3) {
            unguarded.push_back(static_cast<int>(si));
        }
        M -= 400.0 * risk;
    }
    if (traffic.ground_by_start.size() > 1) {
        for (size_t si = 0; si < traffic.ground_by_start.size(); ++si) {
            const auto& mat = traffic.ground_by_start[si];
            const bool has_melee = std::any_of(Be.begin(), Be.end(), [&](int uid) {
                const auto& u = all_units[uid];
                const auto* op = d(uid);
                return mat[u.r][u.c] > 0.0 && op != nullptr && !is_ranged_class(*op);
            });
            if (!has_melee) {
                M -= 250.0;
                if (std::find(unguarded.begin(), unguarded.end(), static_cast<int>(si)) == unguarded.end()) {
                    unguarded.push_back(static_cast<int>(si));
                }
            }
        }
        for (int uid : placed_idx) {
            const auto& u = all_units[uid];
            const auto* op = d(uid);
            if (op == nullptr || op->profession == "MEDIC") {
                continue;
            }
            std::set<int> routes;
            for (size_t si = 0; si < traffic.ground_by_start.size(); ++si) {
                const auto& mat = traffic.ground_by_start[si];
                if (std::find(Be.begin(), Be.end(), uid) != Be.end() && mat[u.r][u.c] > 0.0) {
                    routes.insert(static_cast<int>(si));
                }
                for (const auto& off : op->base_range) {
                    const auto [dr, dc] = rotate_offset(off, u.facing);
                    const int tr = u.r + dr;
                    const int tc = u.c + dc;
                    if (tr >= 0 && tc >= 0 && tr < R && tc < Cn && mat[tr][tc] > 0.0) {
                        routes.insert(static_cast<int>(si));
                        break;
                    }
                }
            }
            if (routes.size() >= 2) {
                M += 20.0 * (static_cast<double>(routes.size()) - 1.0);
            }
        }
    }

    // ---- 盟约激活加成 ----
    std::set<std::string> active_bonds;
    for (const auto& bond : m_bonds) {
        if (!bond.stackable) {
            continue;
        }
        std::set<std::string> names;
        for (int uid : placed_idx) {
            const auto* op = d(uid);
            if (op != nullptr && std::find(op->bond_ids.begin(), op->bond_ids.end(), bond.bond_id) != op->bond_ids.end()) {
                names.insert(op->name);
            }
        }
        const int threshold = bond.thresholds.empty() ? 99 : bond.thresholds.front();
        if (static_cast<int>(names.size()) >= threshold) {
            active_bonds.insert(bond.bond_id);
        }
    }
    double stack_total = stacking;
    for (const auto& bond : m_bonds) {
        std::set<std::string> names;
        for (int uid : placed_idx) {
            const auto* op = d(uid);
            if (op != nullptr && std::find(op->bond_ids.begin(), op->bond_ids.end(), bond.bond_id) != op->bond_ids.end()) {
                names.insert(op->name);
            }
        }
        const int cnt = static_cast<int>(names.size());
        int tier = 0;
        if (bond.inverse) {
            tier = (!bond.thresholds.empty() && cnt == bond.thresholds.front()) ? 1 : 0;
        }
        else {
            for (int th : bond.thresholds) {
                if (cnt >= th) {
                    ++tier;
                }
            }
        }
        if (tier == 0) {
            continue;
        }
        const double focus_mul = focus.count(bond.bond_id) > 0 ? opts.focus_bond_mul : 1.0;
        const double core_mul = bond.is_core ? opts.core_bond_mul : 1.0;
        M += tier * kYi * core_mul * focus_mul;
        if (bond.stackable) {
            stack_total += tier * kZi * core_mul * focus_mul;
        }
    }


    // ---- `Ft`：首个接敌格（路径上第一个紧邻近战的格）----
    std::set<std::string> first_contact;
    for (const auto& path : traffic.ground_paths) {
        int engage = -1;
        for (size_t i = 0; i < path.size(); ++i) {
            for (int uid : Be) {
                const auto& u = all_units[uid];
                const auto* op = d(uid);
                if (u.r == path[i].first && u.c == path[i].second && op != nullptr && !is_ranged_class(*op)) {
                    engage = static_cast<int>(i);
                }
            }
        }
        if (engage > 0) {
            first_contact.insert(std::to_string(path[engage - 1].first) + "," + std::to_string(path[engage - 1].second));
        }
    }
    auto in_range_of = [&](int uid, int r, int c) {
        const auto* op = d(uid);
        if (op == nullptr) {
            return false;
        }
        const auto& u = all_units[uid];
        for (const auto& off : op->base_range) {
            const auto [dr, dc] = rotate_offset(off, u.facing);
            if (u.r + dr == r && u.c + dc == c) {
                return true;
            }
        }
        return false;
    };
    for (const auto& key : first_contact) {
        const auto comma = key.find(',');
        const int r = std::stoi(key.substr(0, comma));
        const int c = std::stoi(key.substr(comma + 1));
        int blocker = -1;
        for (int uid : Be) {
            const auto& u = all_units[uid];
            if (std::abs(u.r - r) + std::abs(u.c - c) == 1) {
                blocker = uid;
                break;
            }
        }
        int others = 0;
        for (int uid : placed_idx) {
            if (uid == blocker) {
                continue;
            }
            if (in_range_of(uid, r, c)) {
                ++others;
            }
            else if (blocker >= 0) {
                const auto& bu = all_units[blocker];
                if (in_range_of(uid, bu.r, bu.c)) {
                    ++others;
                }
            }
        }
        if (others == 0) {
            M -= 120.0;
        }
        else {
            M += 15.0 * std::min(3, others);
        }
    }

    // ---- avoidRightCol：贴着红门的干员 ----
    if (opts.avoid_right_col) {
        std::set<int> handled;
        std::set<std::string> start_cells;
        for (const auto& path : traffic.ground_paths) {
            if (!path.empty()) {
                start_cells.insert(std::to_string(path.front().first) + "," + std::to_string(path.front().second));
            }
        }
        for (const auto& key : start_cells) {
            const auto comma = key.find(',');
            const int r = std::stoi(key.substr(0, comma));
            const int c = std::stoi(key.substr(comma + 1));
            for (int uid : placed_idx) {
                const auto& u = all_units[uid];
                if (handled.count(uid) > 0 || std::abs(u.r - r) + std::abs(u.c - c) != 1) {
                    continue;
                }
                handled.insert(uid);
                int others = 0;
                for (int other : placed_idx) {
                    if (other == uid) {
                        continue;
                    }
                    if (in_range_of(other, u.r, u.c) || in_range_of(other, r, c)) {
                        ++others;
                    }
                }
                M -= 600.0;
                me += 1.0;
            }
        }
    }

    // ---- `V`：已激活的可叠层盟约（供 `front1/back1` 与休息期共用）----
    std::set<std::string> stackable_active;
    for (const auto& bond : m_bonds) {
        if (!bond.stackable) {
            continue;
        }
        std::set<std::string> names;
        for (int uid : placed_idx) {
            const auto* op = d(uid);
            if (op != nullptr
                && std::find(op->bond_ids.begin(), op->bond_ids.end(), bond.bond_id) != op->bond_ids.end()) {
                names.insert(op->name);
            }
        }
        const int th = bond.thresholds.empty() ? 99 : bond.thresholds.front();
        if (static_cast<int>(names.size()) >= th) {
            stackable_active.insert(bond.bond_id);
        }
    }
    // `f`：聚焦盟约的 bondId 集合（用于 `+1` 倍判定）
    std::set<std::string> focus_ids;
    for (const auto& name : opts.focus_bonds) {
        for (const auto& bond : m_bonds) {
            if (bond.name == name) {
                focus_ids.insert(bond.bond_id);
            }
        }
    }
    auto bond_id_of = [&](const std::string& name) {
        for (const auto& bond : m_bonds) {
            if (bond.name == name) {
                return bond.bond_id;
            }
        }
        return std::string();
    };
    auto bond_name_of = [&](const std::string& id) {
        for (const auto& bond : m_bonds) {
            if (bond.bond_id == id) {
                return bond.name;
            }
        }
        return id;
    };

    // `St(F)`：攻击范围覆盖值 + 首接格数
    auto range_cover = [&](int uid, double& v_out, int& engage_out) {
        v_out = 0.0;
        engage_out = 0;
        const auto* op = d(uid);
        if (op == nullptr) {
            return;
        }
        const auto& u = all_units[uid];
        const bool ranged = is_ranged_class(*op);
        for (const auto& off : op->base_range) {
            const auto [dr, dc] = rotate_offset(off, u.facing);
            const int rr = u.r + dr;
            const int cc = u.c + dc;
            if (rr >= 0 && cc >= 0 && rr < R && cc < Cn && traffic.ground[rr][cc] > 0.0) {
                const char ch = rows[rr][cc];
                if (ch != 'S' && ch != 'E') {
                    const double qv = [&]() {
                        auto it = q.find(std::to_string(rr) + "," + std::to_string(cc));
                        return it == q.end() ? 1.0 : it->second;
                    }();
                    v_out += traffic.ground[rr][cc]
                        * (ranged ? 0.5 : 1.2 - traffic.ground_progress[rr][cc]) * qv;
                }
            }
            if (first_contact.count(std::to_string(rr) + "," + std::to_string(cc)) > 0) {
                ++engage_out;
            }
        }
    };

    // `Tt`：场上所有干员的售出价格之和（`ur` 的「售出价格」分支要用）
    int total_sell = 0;
    for (int uid : placed_idx) {
        const auto* op = d(uid);
        if (op != nullptr) {
            total_sell += static_cast<int>(sell_value(*op));
        }
    }
    // `ur(text)`：休息期结束时叠层数（供 `front1` 的「特质与其相同」与休息期块共用）
    auto rest_layers = [&](const std::string& text) -> int {
        if (text.find("休整期结束时") == std::string::npos) {
            return 0;
        }
        if (text.find("身前一格") != std::string::npos || text.find("身后一格") != std::string::npos
            || text.find("同一行") != std::string::npos || text.find("本回合每") != std::string::npos
            || text.find("场上每") != std::string::npos || text.find("整备区每") != std::string::npos) {
            return 0;
        }
        std::vector<std::string> bonds;
        size_t pos = 0;
        while (true) {
            const size_t a = text.find("【", pos);
            if (a == std::string::npos) {
                break;
            }
            const size_t b = text.find("】", a + 3);
            if (b == std::string::npos) {
                break;
            }
            const std::string name = text.substr(a + 3, b - (a + 3));
            const std::string id = bond_id_of(name);
            if (!id.empty() && stackable_active.count(id) > 0) {
                bonds.push_back(id);
            }
            pos = b + 3;
        }
        if (text.find("售出价格") != std::string::npos && text.find("场上所有干员") != std::string::npos) {
            if (bonds.empty()) {
                return 0;
            }
            int cap = 99;
            if (auto m = regex_first_group(text, "至多叠加([0-9]+)层")) {
                cap = std::stoi(*m);
            }
            return std::min(cap, total_sell) * static_cast<int>(bonds.size());
        }
        int per = 0;
        if (auto m = regex_first_group(text, "层数\\+([0-9]+)")) {
            per = std::stoi(*m);
        }
        if (per == 0) {
            return 0;
        }
        if (text.find("层数最多") != std::string::npos || text.find("核心盟约") != std::string::npos) {
            return stackable_active.empty() ? 0 : per;
        }
        return per * static_cast<int>(bonds.size());
    };

    // ---- `Mt`：同名干员只让左上角第一名计叠层 ----
    std::unordered_map<std::string, int> first_by_name;
    {
        std::vector<int> sorted = placed_idx;
        std::sort(sorted.begin(), sorted.end(), [&](int a, int b) {
            if (all_units[a].r != all_units[b].r) {
                return all_units[a].r < all_units[b].r;
            }
            return all_units[a].c < all_units[b].c;
        });
        for (int uid : sorted) {
            const auto* op = d(uid);
            if (op == nullptr) {
                continue;
            }
            if (first_by_name.count(op->name) == 0) {
                first_by_name[op->name] = uid;
            }
        }
    }

    // ---- 特质标签叠层（`Nt` L6198-6280）----
    for (int uid : placed_idx) {
        const auto* op = d(uid);
        if (op == nullptr) {
            continue;
        }
        const auto& u = all_units[uid];
        const double oe = has_focus_bond(*op) ? 2.0 : 1.0;
        auto fit = first_by_name.find(op->name);
        if (fit == first_by_name.end() || fit->second != uid) {
            continue; // 同名只左上角第一名计
        }

        // `<战斗中>...每击倒一名敌人...层数+`
        for (const auto& tr : op->traits) {
            const std::string t = strip_ws(tr.text);
            const bool in_range_tag = std::find(tr.tags.begin(), tr.tags.end(), "inRange") != tr.tags.end();
            if (in_range_tag || t.find("<战斗中>") == std::string::npos) {
                continue;
            }
            static const std::regex kKnock(R"(每击倒一名敌人[^<]*层数\+)");
            if (!std::regex_search(t, kKnock)) {
                continue;
            }
            double v = 0.0;
            int engage = 0;
            range_cover(uid, v, engage);
            stacking += (10.0 * v + 30.0 * engage) * oe;
        }

        for (const auto& tr : op->traits) {
            const int cap = trait_cap(tr);
            const bool per_battle = trait_per_battle(tr);
            for (const auto& tag : tr.tags) {
                if (tag == "front1" || tag == "back1") {
                    const int k = (tag == "front1") ? 1 : -1;
                    int dr = 0;
                    int dc = 0;
                    if (u.facing == "RIGHT") {
                        dr = 0;
                        dc = 1;
                    }
                    else if (u.facing == "LEFT") {
                        dr = 0;
                        dc = -1;
                    }
                    else if (u.facing == "UP") {
                        dr = -1;
                        dc = 0;
                    }
                    else {
                        dr = 1;
                        dc = 0;
                    }
                    const int nr = u.r + dr * k;
                    const int nc = u.c + dc * k;
                    int nb = -1;
                    for (int other : placed_idx) {
                        if (all_units[other].r == nr && all_units[other].c == nc) {
                            nb = other;
                            break;
                        }
                    }
                    if (nb < 0) {
                        continue;
                    }
                    const auto* nop = d(nb);
                    if (nop == nullptr) {
                        continue;
                    }
                    if (tr.text.find("特质与其相同") != std::string::npos) {
                        double ct = 0.0;
                        for (const auto& ntr : nop->traits) {
                            ct += rest_layers(ntr.text);
                        }
                        if (ct > 0.0) {
                            stacking += ct * kFn * oe;
                        }
                    }
                    else {
                        std::vector<std::string> bonds;
                        const bool self_and = tr.text.find("自身及") != std::string::npos || tr.text.find("自身以及") != std::string::npos;
                        for (const auto& b : op->bond_ids) {
                            if (self_and) {
                                bonds.push_back(b);
                            }
                        }
                        for (const auto& b : nop->bond_ids) {
                            bonds.push_back(b);
                        }
                        std::set<std::string> uniq;
                        for (const auto& b : bonds) {
                            if (stackable_active.count(b) > 0) {
                                uniq.insert(b);
                            }
                        }
                        if (!uniq.empty()) {
                            int layers = 0;
                            if (tr.text.find("售出价格") != std::string::npos) {
                                layers = 2 * static_cast<int>(sell_value(*nop));
                            }
                            else if (auto m = regex_first_group(tr.text, "层数\\+([0-9]+)")) {
                                layers = std::stoi(*m);
                            }
                            else {
                                layers = 6;
                            }
                            bool any_focus = false;
                            for (const auto& b : uniq) {
                                if (focus_ids.count(b) > 0) {
                                    any_focus = true;
                                    break;
                                }
                            }
                            stacking += layers * kFn * static_cast<double>(uniq.size()) * (any_focus ? 2.0 : 1.0);
                        }
                    }
                }
                else if (tag == "sameRow") {
                    int cnt = 0;
                    for (int other : placed_idx) {
                        if (other != uid && all_units[other].r == u.r) {
                            ++cnt;
                        }
                    }
                    stacking += 12.0 * static_cast<double>(cnt) * oe;
                }
                else if (tag == "around8") {
                    const std::string t = strip_ws(tr.text);
                    if (t.find("周围八格每有一名敌人") == std::string::npos) {
                        continue;
                    }
                    double gsum = 0.0;
                    int fc = 0;
                    for (int dr = -1; dr <= 1; ++dr) {
                        for (int dc = -1; dc <= 1; ++dc) {
                            if (dr == 0 && dc == 0) {
                                continue;
                            }
                            const int rr = u.r + dr;
                            const int cc = u.c + dc;
                            if (rr < 0 || cc < 0 || rr >= R || cc >= Cn) {
                                continue;
                            }
                            if (traffic.ground[rr][cc] > 0.0) {
                                const char ch = rows[rr][cc];
                                if (ch != 'S' && ch != 'E') {
                                    gsum += traffic.ground[rr][cc];
                                }
                            }
                            if (first_contact.count(std::to_string(rr) + "," + std::to_string(cc)) > 0) {
                                ++fc;
                            }
                        }
                    }
                    stacking += (12.0 * gsum + 40.0 * static_cast<double>(fc)) * oe;
                }
                else if (tag == "onSkill"
                    && std::find(tr.tags.begin(), tr.tags.end(), "front1") == tr.tags.end()) {
                    double v = 0.0;
                    int engage = 0;
                    range_cover(uid, v, engage);
                    stacking += (6.0 * v + 20.0 * engage) * oe;
                }
                else if (tag == "around4" || tag == "around12") {
                    const int rad = (tag == "around4") ? 1 : 2;
                    int cnt = 0;
                    for (int other : placed_idx) {
                        if (other == uid) {
                            continue;
                        }
                        const int dist = std::max(
                            std::abs(all_units[other].r - u.r),
                            std::abs(all_units[other].c - u.c));
                        if (dist <= rad) {
                            ++cnt;
                        }
                    }
                    stacking += 12.0 * static_cast<double>(cnt) * oe;
                }
                else if (tag == "inRange") {
                    double v = 0.0;
                    int engage = 0;
                    range_cover(uid, v, engage);
                    stacking += (14.0 * v + 40.0 * engage) * oe;
                }
                else if (tag == "onDeploy" && per_battle && cap >= 4) {
                    const double cell = block_value(u.r, u.c, uid);
                    const double base = (cell > 0.0 ? cell : 0.0) * 0.3;
                    const double g = traffic.ground_progress[u.r][u.c];
                    stacking += 60.0 * base * (1.4 - g) * oe;
                }
            }
        }
    }

    // ---- 休息期叠层（`lr`）—— service.pretty.mjs L6288-6300 ----
    {
        std::set<std::string> lr_done;
        std::vector<int> order = placed_idx;
        for (size_t k = 0; k < all_units.size(); ++k) {
            if (std::find(placed_idx.begin(), placed_idx.end(), static_cast<int>(k)) == placed_idx.end()) {
                order.push_back(static_cast<int>(k));
            }
        }
        // JS：`Number(Xt.has(oe.uid)) - Number(Xt.has(G.uid))` —— 未上场的排在前面
        std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
            const bool pa = std::find(placed_idx.begin(), placed_idx.end(), a) != placed_idx.end();
            const bool pb = std::find(placed_idx.begin(), placed_idx.end(), b) != placed_idx.end();
            return static_cast<int>(pa) < static_cast<int>(pb);
        });
        for (int uid : order) {
            const auto* op = d(uid);
            if (op == nullptr) {
                continue;
            }
            const bool on_board = std::find(placed_idx.begin(), placed_idx.end(), uid) != placed_idx.end();
            bool any_focus_bond = false;
            for (const auto& b : op->bond_ids) {
                if (focus_ids.count(b) > 0) {
                    any_focus_bond = true;
                    break;
                }
            }
            const double mul = any_focus_bond ? 2.0 : 1.0;
            // `!(on_board && Mt.get(name)?.uid !== uid) && !lr.has(name)`
            if (on_board) {
                auto it = first_by_name.find(op->name);
                if (it == first_by_name.end() || it->second != uid) {
                    continue;
                }
            }
            if (lr_done.count(op->name) > 0) {
                continue;
            }
            for (const auto& tr : op->traits) {
                const std::string& text = tr.text;
                if (text.find("休整期结束时") == std::string::npos) {
                    continue;
                }
                if (!on_board && text.find("整备区也生效") == std::string::npos) {
                    continue;
                }
                if (text.find("身前一格") != std::string::npos || text.find("身后一格") != std::string::npos
                    || text.find("同一行") != std::string::npos || text.find("本回合每") != std::string::npos
                    || text.find("场上每") != std::string::npos || text.find("整备区每") != std::string::npos) {
                    continue;
                }
                int layer = 0;
                if (text.find("售出价格") != std::string::npos && text.find("场上所有干员") != std::string::npos) {
                    int cap = 99;
                    if (auto m = regex_first_group(text, "至多叠加([0-9]+)层")) {
                        cap = std::stoi(*m);
                    }
                    int bond_cnt = 0;
                    {
                        size_t pos = 0;
                        while (true) {
                            const size_t a = text.find("【", pos);
                            if (a == std::string::npos) {
                                break;
                            }
                            const size_t b = text.find("】", a + 3);
                            if (b == std::string::npos) {
                                break;
                            }
                            const std::string id = bond_id_of(text.substr(a + 3, b - (a + 3)));
                            if (!id.empty() && stackable_active.count(id) > 0) {
                                ++bond_cnt;
                            }
                            pos = b + 3;
                        }
                    }
                    if (bond_cnt > 0) {
                        layer = std::min(cap, total_sell) * bond_cnt;
                    }
                }
                else {
                    int per = 0;
                    if (auto m = regex_first_group(text, "层数\\+([0-9]+)")) {
                        per = std::stoi(*m);
                    }
                    if (per == 0) {
                        continue;
                    }
                    if (text.find("层数最多") != std::string::npos || text.find("核心盟约") != std::string::npos) {
                        layer = stackable_active.empty() ? 0 : per;
                    }
                    else {
                        int bond_cnt = 0;
                        size_t pos = 0;
                        while (true) {
                            const size_t a = text.find("【", pos);
                            if (a == std::string::npos) {
                                break;
                            }
                            const size_t b = text.find("】", a + 3);
                            if (b == std::string::npos) {
                                break;
                            }
                            const std::string id = bond_id_of(text.substr(a + 3, b - (a + 3)));
                            if (!id.empty() && stackable_active.count(id) > 0) {
                                ++bond_cnt;
                            }
                            pos = b + 3;
                        }
                        layer = per * bond_cnt;
                    }
                }
                if (layer > 0) {
                    lr_done.insert(op->name);
                    stacking += layer * kFn * mul;
                }
            }
        }
    }

    // `bossSide`：把推进分替换成“整体靠左/靠右”
    if (!opts.boss_side.empty()) {
        double col_sum = 0.0;
        int right_facing = 0;
        for (int uid : placed_idx) {
            col_sum += all_units[uid].c;
            if (all_units[uid].facing == "RIGHT") {
                ++right_facing;
            }
        }
        T = 1000.0 * (opts.boss_side == "left" ? -1.0 : 1.0) * col_sum + right_facing * 20.0;
    }

    out.defense = std::round(M);
    out.stacking = std::round(stacking);
    out.exposed = C;
    out.advance_score = T;
    out.gate = me;
    out.unguarded_routes = static_cast<int>(unguarded.size());
    out.score = out.defense;
    return out;
}



double SPAFarmTaskPlugin::evaluate_placement(
    const std::vector<PlacementAssign>& placement,
    const std::vector<UnitSlot>& bench_units,
    const StageTrafficMap& traffic,
    bool advance_mode) const
{
    PlacementEvalOptions opts;
    for (const auto& cb : m_active_core_bonds) {
        opts.focus_bonds.insert(cb);
    }
    if (m_goal != "medal") {
        for (const auto& gt : kGoalTasks) {
            if (gt.id == m_goal) {
                opts.focus_bonds.insert(gt.bonds.begin(), gt.bonds.end());
                break;
            }
        }
    }
    opts.focus_bond_mul = (m_goal == "medal") ? 2.0 : 6.0;
    opts.core_bond_mul = 1.5;
    opts.support_defense_weight = 1.0;
    opts.support_heal_weight = 1.0;
    opts.advance = advance_mode;
    opts.round = m_current_round;
    opts.avoid_right_col = (m_current_round >= 9 && m_current_boss.find("左") == std::string::npos);
    return evaluate_placement_nt(placement, bench_units, traffic, opts).score;
}

SPAFarmTaskPlugin::PlacementEvalOptions SPAFarmTaskPlugin::default_eval_options(bool advance_mode) const
{
    PlacementEvalOptions o;
    o.focus_bonds = std::unordered_set<std::string>(m_active_core_bonds.begin(), m_active_core_bonds.end());
    // 注意：JS 的 focusBonds 是 **bondId**；这里沿用名字，由 evaluate_placement_nt 内部按 bond_id 比对，
    // 因此调用前会用 m_bonds 把名字翻译成 id。
    o.focus_bond_mul = 2.0;
    o.core_bond_mul = 1.5;
    o.support_defense_weight = 1.0;
    o.support_heal_weight = 1.0;
    o.advance = advance_mode;
    o.avoid_right_col = false;
    o.round = m_current_round;
    return o;
}

std::pair<std::vector<SPAFarmTaskPlugin::PlacementAssign>, std::vector<SPAFarmTaskPlugin::PlacementMove>>
SPAFarmTaskPlugin::solve_placement(
    const StageTrafficMap& traffic,
    int max_deploy,
    bool advance_mode,
    const PlacementEvalOptions* extra) const
{
    // `ss(a)` L6409-6478 + `Zr(a, s, e, t)` L6312-6388
    std::vector<PlacementAssign> no_assign;
    std::vector<PlacementMove> no_moves;

    // ---- 候选单位（上场 + 备战）----
    struct SU
    {
        std::string name;
        const OperatorInfo* op = nullptr;
        int r = -1;
        int c = -1;
        std::string facing = "RIGHT";
        bool elite = false;
    };
    std::vector<SU> units;
    for (const auto& u : m_units) {
        if (u.name.empty() || find_operator(u.name) == nullptr) {
            continue;
        }
        SU su;
        su.name = u.name;
        su.op = find_operator(u.name);
        su.r = u.r;
        su.c = u.c;
        su.facing = u.facing.empty() ? "RIGHT" : u.facing;
        su.elite = u.elite;
        units.push_back(su);
    }
    if (units.empty()) {
        return { no_assign, no_moves };
    }

    const auto& rows = traffic.grid;
    const int R = traffic.rows;
    const int Cn = traffic.cols;

    // 可部署格（排除 H 所在行）
    std::vector<std::pair<int, int>> cells;
    for (const auto& p : deploy_cells(rows)) {
        if (rows[p.first][p.second] != 'H') {
            cells.emplace_back(p);
        }
    }
    if (extra != nullptr && !cells.empty()) {
        const int cols = traffic.cols;
        if (extra->exclude_left_cols > 0 || extra->left_half_only) {
            int min_col_all = std::numeric_limits<int>::max();
            for (int r = 0; r < traffic.rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    const char ch = traffic.grid[r][c];
                    if (ch != '#' && ch != 'H' && ch != 'Q') {
                        min_col_all = std::min(min_col_all, c);
                    }
                }
            }
            const int cut = extra->exclude_left_cols;
            std::vector<std::pair<int, int>> kept;
            for (const auto& p : cells) {
                if (cut > 0 && p.second < min_col_all + cut) {
                    continue;
                }
                if (extra->left_half_only && p.second >= cols / 2 + cols % 2) {
                    continue;
                }
                kept.push_back(p);
            }
            cells.swap(kept);
        }
    }
    if (cells.empty()) {
        return { no_assign, no_moves };
    }
    int min_col_cells = std::numeric_limits<int>::max();
    for (const auto& p : cells) {
        min_col_cells = std::min(min_col_cells, p.second);
    }
    int all_min_col = std::numeric_limits<int>::max();
    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < Cn; ++c) {
            const char ch = rows[r][c];
            if (ch == '#' || ch == 'H' || ch == 'Q') {
                continue;
            }
            all_min_col = std::min(all_min_col, c);
        }
    }
    (void)min_col_cells;

    std::vector<std::pair<int, int>> usable;
    for (const auto& p : cells) {
        if (p.second >= all_min_col) {
            usable.emplace_back(p);
        }
    }
    if (usable.empty()) {
        usable = cells;
    }

    // 每行最右列（`l` / `E`）
    std::unordered_map<int, int> rightmost;
    for (const auto& p : usable) {
        auto it = rightmost.find(p.first);
        if (it == rightmost.end() || it->second < p.second) {
            rightmost[p.first] = p.second;
        }
    }
    auto is_rightmost = [&](const std::pair<int, int>& p) {
        auto it = rightmost.find(p.first);
        return it != rightmost.end() && it->second == p.second;
    };

    const int deploy_limit = std::min(max_deploy > 0 ? max_deploy : 8, static_cast<int>(usable.size()));
    std::vector<SU> chosen(units.begin(), units.begin() + std::min<int>(deploy_limit, static_cast<int>(units.size())));

    auto valid_cell = [&](const SU& u, const std::pair<int, int>& p) {
        if (u.op == nullptr) {
            return false;
        }
        if (rows[p.first][p.second] == 'h' && u.op->position != "RANGED") {
            return false;
        }
        return true;
    };

    // `Zr(a, s, e, t)`
    auto zr = [&](uint32_t seed,
                  const std::function<double(const PlacementEvalResult&)>& weight,
                  const std::vector<PlacementAssign>* init,
                  int iterations_override = 0,
                  bool no_new_units = false) -> std::vector<PlacementAssign> {
        uint32_t st = seed != 0 ? seed : 1;
        auto rnd = [&]() {
            st ^= st << 13;
            st ^= st >> 17;
            st ^= st << 5;
            return static_cast<double>(st) / 4294967296.0;
        };

        std::vector<PlacementAssign> cur;
        if (init != nullptr) {
            cur = *init;
        }
        else {
            std::set<std::pair<int, int>> used;
            for (const auto& u : chosen) {
                std::vector<std::pair<int, int>> pool;
                for (const auto& p : usable) {
                    if (used.count(p) > 0 || !valid_cell(u, p)) {
                        continue;
                    }
                    pool.push_back(p);
                }
                if (pool.empty()) {
                    continue;
                }
                std::stable_sort(pool.begin(), pool.end(), [&](const auto& a, const auto& b) {
                    const bool ga = valid_cell(u, a), gb = valid_cell(u, b);
                    if (ga != gb) {
                        return gb;
                    }
                    const bool sa = traffic.ground[a.first][a.second] > 0.0;
                    const bool sb = traffic.ground[b.first][b.second] > 0.0;
                    if (u.op != nullptr && u.op->position == "MELEE") {
                        return sb && !sa;
                    }
                    return sa && !sb;
                });
                used.insert(pool.front());
                PlacementAssign pa;
                pa.name = u.name;
                pa.r = pool.front().first;
                pa.c = pool.front().second;
                pa.facing = "RIGHT";
                pa.elite = u.elite;
                cur.push_back(pa);
            }
        }
        if (cur.empty()) {
            return cur;
        }

        auto evaluate = [&](const std::vector<PlacementAssign>& pl) {
            auto opts = default_eval_options(advance_mode);
            if (extra != nullptr) {
                opts.avoid_right_col = extra->avoid_right_col;
                // `boss_side` 不在这里生效：JS 的 `ws`/`is` 是用加权函数重排，
                // 不是改 `Nt` 本身的推进分。见下面的 `is` 阶段。
                opts.focus_bond_mul = extra->focus_bond_mul;
                opts.core_bond_mul = extra->core_bond_mul;
                opts.support_defense_weight = extra->support_defense_weight;
                opts.support_heal_weight = extra->support_heal_weight;
            }
            // focus_bonds 用 bondId
            std::unordered_set<std::string> ids;
            for (const auto& name : m_active_core_bonds) {
                for (const auto& b : m_bonds) {
                    if (b.name == name) {
                        ids.insert(b.bond_id);
                    }
                }
            }
            if (!ids.empty()) {
                opts.focus_bonds = ids;
            }
            std::vector<UnitSlot> bench;
            for (const auto& u : units) {
                bool placed = std::any_of(pl.begin(), pl.end(), [&](const PlacementAssign& a) {
                    return a.name == u.name;
                });
                if (!placed) {
                    UnitSlot s;
                    s.name = u.name;
                    bench.push_back(s);
                }
            }
            return evaluate_placement_nt(pl, bench, traffic, opts);
        };

        double best_val = weight(evaluate(cur));
        std::vector<PlacementAssign> best = cur;
        double cur_val = best_val;

        const int n = static_cast<int>(cur.size());
        const int total_units = static_cast<int>(units.size());
        const int iterations = (iterations_override > 0)
            ? iterations_override
            : static_cast<int>(
                std::lround((2500.0 + 1100.0 * n + 400.0 * std::max(0, total_units - n))));
        static const std::array<std::string, 4> kDirs = { "RIGHT", "UP", "LEFT", "DOWN" };

        for (int it = 0; it < iterations && !need_exit(); ++it) {
            const double temp = 50.0 * (1.0 - static_cast<double>(it) / iterations) + 0.5;
            auto cand = cur;
            const int qi = std::min<int>(n - 1, static_cast<int>(rnd() * n));
            const double p = rnd();

            // 未上场单位
            std::vector<int> idle;
            for (int i = 0; i < total_units; ++i) {
                const bool used = std::any_of(cand.begin(), cand.end(), [&](const PlacementAssign& a) {
                    return a.name == units[i].name;
                });
                if (!used) {
                    idle.push_back(i);
                }
            }

            if (!no_new_units && !idle.empty() && p < 0.12) {
                const int pick = idle[std::min<int>(static_cast<int>(idle.size()) - 1, static_cast<int>(rnd() * idle.size()))];
                if (!valid_cell(units[pick], { cand[qi].r, cand[qi].c })) {
                    continue;
                }
                cand[qi].name = units[pick].name;
                cand[qi].elite = units[pick].elite;
            }
            else if (p < 0.4) {
                cand[qi].facing = kDirs[std::min<int>(3, static_cast<int>(rnd() * 4))];
            }
            else if (p < 0.72) {
                std::set<std::pair<int, int>> used_cells;
                for (const auto& a : cand) {
                    used_cells.insert({ a.r, a.c });
                }
                std::vector<std::pair<int, int>> free_cells;
                const auto* op = find_operator(cand[qi].name);
                for (const auto& cell : usable) {
                    if (used_cells.count(cell) > 0) {
                        continue;
                    }
                    if (op != nullptr && rows[cell.first][cell.second] == 'h' && op->position != "RANGED") {
                        continue;
                    }
                    free_cells.push_back(cell);
                }
                if (free_cells.empty()) {
                    continue;
                }
                const auto& cell = free_cells[std::min<int>(static_cast<int>(free_cells.size()) - 1, static_cast<int>(rnd() * free_cells.size()))];
                cand[qi].r = cell.first;
                cand[qi].c = cell.second;
            }
            else {
                const int ei = std::min<int>(n - 1, static_cast<int>(rnd() * n));
                if (ei == qi) {
                    continue;
                }
                std::swap(cand[qi].r, cand[ei].r);
                std::swap(cand[qi].c, cand[ei].c);
                std::swap(cand[qi].facing, cand[ei].facing);
            }

            const double val = weight(evaluate(cand));
            if (val > cur_val || rnd() < std::exp((val - cur_val) / temp)) {
                cur = cand;
                cur_val = val;
                if (val > best_val) {
                    best_val = val;
                    best = cand;
                }
            }
        }
        return best;
    };

    // ---- 第一阶段：防御（3 个种子）----
    auto w1 = [&](const PlacementEvalResult& r) {
        return r.defense + 0.05 * r.stacking
            - (advance_mode ? 1e5 * r.exposed : 0.0) - 3e3 * r.gate;
    };
    std::vector<PlacementAssign> best1;
    double best1_val = -1e18;
    for (int k = 0; k < 3; ++k) {
        auto pl = zr(static_cast<uint32_t>(42 + 101 * k), w1, nullptr);
        if (pl.empty()) {
            continue;
        }
        const double v = w1(evaluate_placement_nt(
            pl,
            {},
            traffic,
            default_eval_options(advance_mode)));
        if (v > best1_val) {
            best1_val = v;
            best1 = pl;
        }
    }
    if (best1.empty()) {
        return { no_assign, no_moves };
    }

    // ---- 第二阶段：叠层（`ss` L6438-6441；`strategy === "defense"` 时整段跳过）----
    const std::string strategy = (extra != nullptr) ? extra->strategy : "balance";
    double defense_max = evaluate_placement_nt(best1, {}, traffic, default_eval_options(advance_mode)).defense;
    auto best2 = best1;
    if (strategy != "defense") {
        // `as = { defense:1, balance:.85, stack:.6 }`
        const double as_factor = (strategy == "stack") ? 0.6 : 0.85;
        const double def_coef = (strategy == "balance") ? 0.6 : 0.3;
        auto w2 = [&](const PlacementEvalResult& r) {
            const double target = as_factor * defense_max;
            return r.stacking * 2.0 + r.defense * def_coef
                - 4.0 * std::max(0.0, target - r.defense)
                - 300.0 * r.unguarded_routes
                - (advance_mode ? 1e5 * r.exposed : 0.0) - 3e3 * r.gate;
        };
        auto pl = zr(43, w2, &best1);
        if (!pl.empty()) {
            best2 = pl;
        }
    }

    // ---- 第三阶段：advance 约束优先 ----
    if (advance_mode) {
        const auto base = evaluate_placement_nt(best2, {}, traffic, default_eval_options(true));
        const double def_floor = base.defense - std::abs(base.defense) * 0.05;
        const double stack_floor = base.stacking - std::abs(base.stacking) * 0.05;
        auto w3 = [&](const PlacementEvalResult& r) {
            if (r.gate > base.gate || r.exposed > base.exposed + 0.001
                || r.defense < def_floor || r.stacking < stack_floor
                || r.unguarded_routes > base.unguarded_routes) {
                return -1e6 - 1e5 * r.exposed - 1e3 * std::max(0.0, def_floor - r.defense)
                    - 1e3 * std::max(0.0, stack_floor - r.stacking);
            }
            return r.defense + r.stacking * 2.0 + r.advance_score * 0.05;
        };
        auto pl = zr(59, w3, &best2);
        if (!pl.empty() && w3(evaluate_placement_nt(pl, {}, traffic, default_eval_options(true))) >= w3(base)) {
            best2 = pl;
        }
    }

    // ---- `ws` / `is`：BOSS 回合以现有布阵为起点重排（L6390-6408 / L8267-8290）----
    if (extra != nullptr && !extra->boss_side.empty()) {
        // `is(a, s, e)` 的 `t`：units 只保留已上场的、advance=true、maxDeploy=s.length、
        // bossSide=e、iterations=8000
        PlacementEvalOptions is_opts = default_eval_options(true);
        is_opts.avoid_right_col = extra->avoid_right_col;
        is_opts.exclude_left_cols = extra->exclude_left_cols;
        is_opts.left_half_only = extra->left_half_only;
        is_opts.strategy = "balance";
        is_opts.boss_side = extra->boss_side;

        const bool left_side = (extra->boss_side == "left");
        auto is_weight = [left_side](const PlacementEvalResult& r) {
            if (left_side) {
                return r.exposed > 0.0 ? -1e9 - r.exposed * 1e6
                                       : r.advance_score + r.defense * 0.03 + r.stacking * 0.08;
            }
            return r.defense + r.stacking * 0.5 + r.advance_score * 0.05 - 150.0 * r.exposed;
        };

        std::vector<PlacementAssign> best_is = best2;
        double best_is_val = is_weight(evaluate_placement_nt(best2, {}, traffic, is_opts));
        for (int k = 0; k < 2; ++k) {
            auto pl = zr(static_cast<uint32_t>(331 + 101 * k), is_weight, &best2, 8000, true);
            if (pl.empty()) {
                continue;
            }
            const double v = is_weight(evaluate_placement_nt(pl, {}, traffic, is_opts));
            if (v > best_is_val) {
                best_is_val = v;
                best_is = pl;
            }
        }
        best2 = best_is;
    }

    // ---- 生成 moves ----
    std::vector<PlacementMove> moves;
    for (const auto& a : best2) {
        PlacementMove mv;
        mv.unit = a.name;
        mv.to_area = "board";
        mv.to_r = a.r;
        mv.to_c = a.c;
        mv.facing = a.facing;
        const UnitSlot* cur = nullptr;
        for (const auto& u : m_units) {
            if (u.name == a.name) {
                cur = &u;
                break;
            }
        }
        if (cur == nullptr) {
            mv.type = "deploy";
            mv.from_area = "bench";
        }
        else if (cur->area == "board" && cur->r == a.r && cur->c == a.c) {
            mv.type = (cur->facing == a.facing) ? "none" : "orient";
            mv.from_area = "board";
            mv.from_r = cur->r;
            mv.from_c = cur->c;
        }
        else {
            mv.type = "reposition";
            mv.from_area = cur->area;
            mv.from_r = cur->r;
            mv.from_c = cur->c;
        }
        if (mv.type != "none") {
            moves.push_back(mv);
        }
    }
    // 场上但不在新方案里的 → 撤回备战席
    for (const auto& u : m_units) {
        if (u.area != "board" || u.name.empty()) {
            continue;
        }
        const bool kept = std::any_of(best2.begin(), best2.end(), [&](const PlacementAssign& a) {
            return a.name == u.name;
        });
        if (!kept) {
            PlacementMove mv;
            mv.type = "bench";
            mv.unit = u.name;
            mv.from_area = "board";
            mv.from_r = u.r;
            mv.from_c = u.c;
            mv.to_area = "bench";
            moves.push_back(mv);
        }
    }

    return { best2, moves };
}

std::optional<std::pair<double, double>> SPAFarmTaskPlugin::detect_facing_white_box(const cv::Mat& img)
{
    // 1:1 port of `Ei(a)` in service.pretty.mjs L4907-4944:
    //   resize 960x540 → scan x 180..950, y 40..500 for bright neutral pixels
    //   (min > 200 && max - min < 40) → 8-connected components
    //   → bbox width in (220, 470), height in (160, 400) → largest by pixel count
    //   → return (x0 + x1, y0 + y1) which is the centre doubled into 1920x1080 space.
    if (img.empty()) {
        return std::nullopt;
    }
    cv::Mat small_img;
    cv::resize(img, small_img, cv::Size(960, 540), 0.0, 0.0, cv::INTER_AREA);
    constexpr int W = 960;
    constexpr int H = 540;
    std::vector<uint8_t> mask(static_cast<size_t>(W) * H, 0);
    for (int y = 40; y < 500; ++y) {
        const auto* row = small_img.ptr<cv::Vec3b>(y);
        for (int x = 180; x < 950; ++x) {
            const int B = row[x][0], G = row[x][1], R = row[x][2];
            const int mx = std::max({ R, G, B });
            const int mn = std::min({ R, G, B });
            if (mn > 200 && (mx - mn) < 40) {
                mask[static_cast<size_t>(y) * W + x] = 1;
            }
        }
    }

    struct FacingBox
    {
        int n = 0;
        int x0 = W;
        int x1 = 0;
        int y0 = H;
        int y1 = 0;
    };

    std::vector<uint8_t> vis(static_cast<size_t>(W) * H, 0);
    std::vector<FacingBox> boxes;
    std::vector<int> stack;
    for (int y = 40; y < 500; ++y) {
        for (int x = 180; x < 950; ++x) {
            const int idx = y * W + x;
            if (!mask[idx] || vis[idx]) {
                continue;
            }
            FacingBox c;
            vis[idx] = 1;
            stack.clear();
            stack.push_back(idx);
            while (!stack.empty()) {
                const int cur = stack.back();
                stack.pop_back();
                const int cx = cur % W;
                const int cy = cur / W;
                ++c.n;
                c.x0 = std::min(c.x0, cx);
                c.x1 = std::max(c.x1, cx);
                c.y0 = std::min(c.y0, cy);
                c.y1 = std::max(c.y1, cy);
                // `Ei` 用 8 邻域：[-t-1, -t, -t+1, -1, 1, t-1, t, t+1]
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) {
                            continue;
                        }
                        const int nx = cx + dx;
                        const int ny = cy + dy;
                        if (nx < 0 || ny < 0 || nx >= W || ny >= H) {
                            continue;
                        }
                        const int nidx = ny * W + nx;
                        if (mask[nidx] && !vis[nidx]) {
                            vis[nidx] = 1;
                            stack.push_back(nidx);
                        }
                    }
                }
            }
            boxes.push_back(c);
        }
    }

    const FacingBox* best = nullptr;
    for (const auto& c : boxes) {
        const int bw = c.x1 - c.x0;
        const int bh = c.y1 - c.y0;
        // `Ei` 的唯一判据：宽 220..470、高 160..400
        if (!(bw > 220 && bw < 470 && bh > 160 && bh < 400)) {
            continue;
        }
        if (best == nullptr || c.n > best->n) {
            best = &c;
        }
    }
    if (best == nullptr) {
        return std::nullopt;
    }
    // 960x540 下的中心 (x0+x1)/2 映射回 1920x1080 正好是 (x0+x1)
    return std::make_pair(
        static_cast<double>(best->x0 + best->x1),
        static_cast<double>(best->y0 + best->y1));
}

bool SPAFarmTaskPlugin::execute_placement_move(const PlacementMove& mv, const CameraDetectResult& cam)
{
    // Exact 1:1 port of `At(I)` in service.pretty.mjs lines 9724-9983
    const auto H = active_board_homography(cam);
    auto [fx, fy] = project_point(H, mv.from_c + 0.5, mv.from_r + 0.5);
    auto [tx, ty] = project_point(H, mv.to_c + 0.5, mv.to_r + 0.5);

    if (mv.type == "orient") {
        tap(tx, ty, 200);
    }
    else {
        drag_board_unit(fx, fy, tx, ty, false);
        sleep_ms(static_cast<int>(std::lround(150.0 * m_step_scale)));
    }

    if (mv.to_area != "board") {
        close_detail_panel();
        return true;
    }

    // `At` L9093-9096：每 60ms 截一帧找朝向控件，最多 1500ms。
    // 旧实现只轮询 5×120ms=600ms，慢一帧就误判「未出现朝向控件」。
    std::optional<std::pair<double, double>> box_center;
    {
        const auto t0 = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - t0).count() < 1500
               && !need_exit()) {
            cv::Mat cur = shot();
            box_center = detect_facing_white_box(cur);
            if (box_center.has_value()) {
                break;
            }
            sleep_ms(60);
        }
    }

    double cx = box_center ? box_center->first : tx;
    double cy = box_center ? box_center->second : ty;

    auto dir_delta = [](std::string_view dir) -> std::pair<double, double> {
        if (dir == "RIGHT") {
            return { 210.0, 0.0 };
        }
        if (dir == "LEFT") {
            return { -210.0, 0.0 };
        }
        if (dir == "UP") {
            return { 0.0, -180.0 };
        }
        if (dir == "DOWN") {
            return { 0.0, 180.0 };
        }
        return { -210.0, 0.0 };
    };

    auto [dx, dy] = dir_delta(mv.facing);
    for (int attempt = 0; attempt < 3 && !need_exit(); ++attempt) {
        drag_segments(cx, cy, cx + dx, cy + dy, 4, 12, 40); // `At` L9113-9115: steps 4, hold 40
        sleep_ms(220);
        cv::Mat after = shot();
        auto still_box = detect_facing_white_box(after);
        if (!still_box.has_value()) {
            // Update local unit state
            std::string old_slot = (mv.from_area == "bench")
                ? ("bench:" + std::to_string(mv.from_c + 1))
                : ("board:" + std::to_string(mv.from_r) + "," + std::to_string(mv.from_c));
            std::string new_slot = "board:" + std::to_string(mv.to_r) + "," + std::to_string(mv.to_c);
            for (auto& u : m_units) {
                if (u.slot == old_slot) {
                    u.slot = new_slot;
                    u.area = "board";
                    u.r = mv.to_r;
                    u.c = mv.to_c;
                    u.facing = mv.facing;
                    m_remembered_units[new_slot] = u;
                    break;
                }
            }
            if (old_slot != new_slot) {
                m_remembered_units.erase(old_slot);
            }
            close_detail_panel();
            return true;
        }
        cx = still_box->first;
        cy = still_box->second;
    }

    close_detail_panel();
    return false;
}

int SPAFarmTaskPlugin::run_auto_place(const CameraDetectResult& cam, bool advance_mode, bool retry)
{
    // Exact 1:1 port of `we(I = {})` in service.pretty.mjs lines 9724-9983:
    // ① If `收起` is visible on screen, tap `收起` (300ms) first so the shop panel does not occlude the board.
    // ② Call `wait_stable_board(8000)`.
    // ③ If unstable, call `close_detail_panel()` + `tap(1150, 950, 800)` + `wait_stable_board(6000)`.
    // ④ If still unstable, log `"部署前棋盘定位未稳定"` and abort deployment cleanly.
    // ⑤ Before executing moves, call `wait_stable_board(5000)` (fallback `close_detail_panel()` + `wait_stable_board(5000)`),
    //   resolve swap cycles via a temporary empty bench/board slot (`Re`/`xe`), and after moves call `sleep_ms(300)` + `wait_stable_board(4000)`.
    (void)cam;
    cv::Mat init_img = shot();
    if (!init_img.empty()) {
        auto init_ocr = ocr(init_img);
        if (auto collapse_btn = find_ocr(init_ocr, { "收起" })) {
            tap(collapse_btn->cx, collapse_btn->cy, static_cast<int>(std::lround(300.0 * m_step_scale)));
        }
    }

    auto st = wait_stable_board(8000);
    if (!st.has_value()) {
        close_detail_panel();
        tap(1150, 950, static_cast<int>(std::lround(800.0 * m_step_scale)));
        st = wait_stable_board(6000);
    }
    if (!st.has_value()) {
        log_info("部署前棋盘定位未稳定");
        return 0;
    }

    CameraDetectResult cur_cam { st->camera, st->raw_H, st->fit };

    // ---- `we` 的三合一阶段（L9729-9736） ----
    if (!m_pending_merge.empty()) {
        wait_stable_board(3000);
        cv::Mat merge_img = shot();
        if (!merge_img.empty()) {
            refresh_board_snapshot(merge_img, cur_cam, true); // `e.snapshot({force:!0})`
        }
    }
    bool golden_summon = false; // `B`
    for (const auto& nm : m_pending_merge) {
        m_elite_units.insert(nm);
        // `<获得时>...(随机获得|获得N名)...干员` 的金干员合成后会改变整备区构成，需要全量重读
        const auto* mop = find_operator(nm);
        if (mop != nullptr && mop->golden) {
            for (const auto& tr : mop->traits) {
                const std::string t = strip_ws(tr.text);
                if (t.find("<获得时>") == std::string::npos || t.find("干员") == std::string::npos) {
                    continue;
                }
                static const std::regex kGrant(R"(随机获得|获得[0-9一二三]?名)");
                if (std::regex_search(t, kGrant)) {
                    golden_summon = true;
                    break;
                }
            }
        }
        check_triple_merge(nm, true);
    }
    const bool merged_any = !m_pending_merge.empty();
    if (merged_any) {
        std::string names;
        for (const auto& nm : m_pending_merge) {
            names += (names.empty() ? "" : "、") + nm;
        }
        log_info("第 " + std::to_string(m_current_round) + " 回合 · " + names + " 精锐合成，重新识别"
            + (golden_summon ? "全部干员" : "相关格子"));
        m_pending_merge.clear();
    }

    refresh_board_snapshot(st->img, cur_cam, false);

    // `ee = z === 0 && bench数 < Wt(state) - 1`
    int bench_cnt = 0;
    for (const auto& u : m_units) {
        if (u.area == "bench") {
            ++bench_cnt;
        }
    }
    int bench_cap = 10;
    for (const auto& stg : m_stages) {
        if (stg.stage_id != m_stage_id) {
            continue;
        }
        for (const auto& row : stg.rows) {
            if (row.find('H') == std::string::npos) {
                continue;
            }
            const int cnt = static_cast<int>(std::count(row.begin(), row.end(), 'H'));
            bench_cap = (cnt > 0) ? cnt : 10;
            break;
        }
        break;
    }
    const bool skip_bench_unknown = (bench_cnt < bench_cap - 1);
    // `onlyUnknown: !identifyUnknown && !B`，生产 `identifyUnknown=false` → `!B`
    inspect_unknown_units(cur_cam, skip_bench_unknown, golden_summon);

    // `N(round) && !state.info?.boss` → 点 (510,50) 打开本局信息页读 BOSS，再关掉
    {
        const bool boss_round_pre = (m_current_round == m_last_round);
        if (boss_round_pre && m_current_boss.empty()) {
            tap(510, 50, 500);
            ScreenInfo si = classify_screen(shot());
            for (int k = 0; si.screen != ScreenState::Info && k < 5; ++k) {
                sleep_ms(500);
                si = classify_screen(shot());
            }
            if (si.screen == ScreenState::Info) {
                tap(1805, 65, 500);
                wait_for([](const ScreenInfo& s) { return s.screen == ScreenState::Prep; }, 5000);
            }
            if (m_current_boss.empty()) {
                log_info("BOSS 未识别，按普通 BOSS（靠右）处理");
            }
        }
    }

    StageTrafficMap traffic = build_stage_traffic(m_stage_id, m_current_round <= 3);
    // `we` 的 `e.recommend({...})` 参数
    const bool boss_round = (m_current_round == m_last_round);
    const bool mob_boss = boss_round && m_current_boss.find("矩兽") != std::string::npos;
    PlacementEvalOptions extra = default_eval_options(true);
    extra.focus_bond_mul = (m_goal != "medal") ? 6.0 : 2.0; // `be()` 为真时 6
    extra.exclude_left_cols = mob_boss ? 0 : 3;
    extra.left_half_only = mob_boss;
    extra.avoid_right_col = !boss_round;
    extra.boss_side = boss_round ? "right" : "";
    extra.strategy = boss_round ? "defense" : "balance";
    (void)advance_mode;
    // `we` 里 `maxDeploy: 8` 是硬编码的，不走 `m_deploy_cap`
    auto [assigns, moves] = solve_placement(traffic, 8, true, &extra);
    if (moves.empty()) {
        // JS `we` 里 `Pe.length === 0` 只让移动循环空转，后面的 `Dt`(召唤物) 与 `on`(装备) 仍继续执行；
        // 这里不能静默 return，否则整备阶段的装备分发会被一并跳过。
        log_info("第 " + std::to_string(m_current_round) + " 回合 · 布阵 0 名角色，仍继续召唤物与装备");
    }

    auto pre_mv_st = wait_stable_board(5000);
    if (!pre_mv_st.has_value()) {
        close_detail_panel();
        pre_mv_st = wait_stable_board(5000);
    }
    if (!pre_mv_st.has_value()) {
        log_info("移动前棋盘未稳定");
        return 0;
    }
    cur_cam = CameraDetectResult { pre_mv_st->camera, pre_mv_st->raw_H, pre_mv_st->fit };

    // `we` L9798-9900：先跑 `tt`（把方案外的场上单位撤到整备区空格），
    // 再跑 `pt`（把方案内单位移到目标格）。`Re()`=H 行空格，`xe()`=棋盘临时空格，
    // `Je`=放弃部署的单位，`$t`=每个单位最多中转 2 次，总循环上限 40。
    std::set<std::pair<int, int>> occupied_cells;
    for (const auto& u : m_units) {
        if ((u.area == "board" && !u.ghost) || u.area == "bench") {
            occupied_cells.emplace(u.r, u.c);
        }
    }

    // `Ze = Ae.rows.findIndex(re => re.includes("H"))`
    int bench_row = -1;
    for (int r = 0; r < traffic.rows; ++r) {
        if (traffic.grid[r].find('H') != std::string::npos) {
            bench_row = r;
            break;
        }
    }

    // `Re()`：整备区 H 行里第一个未被占用的 H 格
    auto find_bench_cell = [&]() -> std::optional<std::pair<int, int>> {
        if (bench_row < 0) {
            return std::nullopt;
        }
        for (int c = 0; c < traffic.cols; ++c) {
            if (traffic.grid[bench_row][c] == 'H' && occupied_cells.count({ bench_row, c }) == 0) {
                return std::make_pair(bench_row, c);
            }
        }
        return std::nullopt;
    };

    // `xe(re)`：棋盘上 'a'/'b'/'h'、非 H 行、未被占用、且不在目标方案里的临时格
    auto find_board_cell = [&]() -> std::optional<std::pair<int, int>> {
        std::set<std::pair<int, int>> targets;
        for (const auto& p : moves) {
            if (p.to_area == "board") {
                targets.emplace(p.to_r, p.to_c);
            }
        }
        for (int r = 0; r < traffic.rows; ++r) {
            if (r == bench_row) {
                continue;
            }
            for (int c = 0; c < traffic.cols; ++c) {
                const char ch = traffic.grid[r][c];
                if ((ch == 'a' || ch == 'b' || ch == 'h') && occupied_cells.count({ r, c }) == 0
                    && targets.count({ r, c }) == 0) {
                    return std::make_pair(r, c);
                }
            }
        }
        return std::nullopt;
    };

    // `tt` = 方案外的场上单位 → bench；`pt` = 方案内的目标站位
    std::vector<PlacementMove> bench_moves;
    std::vector<PlacementMove> pending;
    for (const auto& mv : moves) {
        if (mv.type == "bench") {
            bench_moves.push_back(mv);
        }
        else {
            pending.push_back(mv);
        }
    }

    // `we` 的失败路径（L9810-9827）：
    //   非 retry → 把该格的单位名清空并抛 `wa`，外层 `we(!0)` 重跑一次
    //   retry    → 记日志、跳过该单位，不再重试
    bool need_restart = false;
    auto on_move_fail = [&](const PlacementMove& mv) {
        if (retry) {
            log_info((mv.unit.empty() ? std::string("对象") : mv.unit) + " 仍无法部署，本回合跳过");
            return;
        }
        log_info((mv.unit.empty() ? std::string("对象") : mv.unit) + " 无法部署，重新识别该格后重排");
        for (auto& u : m_units) {
            if (u.r == mv.to_r && u.c == mv.to_c) {
                u.name.clear();
                u.source.clear();
                u.verified = false;
                break;
            }
        }
        need_restart = true;
    };

    int executed = 0;
    auto do_move = [&](const PlacementMove& mv) -> bool {
        log_info(
            "布阵动作: " + mv.type + " " + mv.unit + " -> ("
            + std::to_string(mv.to_r) + "," + std::to_string(mv.to_c) + ") 朝向:" + mv.facing);
        if (execute_placement_move(mv, cur_cam)) {
            occupied_cells.erase({ mv.from_r, mv.from_c });
            occupied_cells.emplace(mv.to_r, mv.to_c);
            ++executed;
            return true;
        }
        on_move_fail(mv);
        return false;
    };

    std::set<std::string> skipped; // `Je`
    // ---- `tt` (L9822-9838)：先撤下多余单位，腾出棋盘格 ----
    for (size_t bi = 0; bi < bench_moves.size() && !need_exit() && !need_restart; ++bi) {
        PlacementMove mv = bench_moves[bi];
        auto cell = find_bench_cell();
        if (!cell) {
            // `Oe && ze.filter(He => He.area === "board" && !He.auxiliary).length < 8 && await nt(Oe.unit, Oe)`
            // 场上不足 8 人时，先把一个方案内的 bench 单位上场，腾出整备区格
            int board_cnt = 0;
            for (const auto& u : m_units) {
                if (u.area == "board" && !u.auxiliary) {
                    ++board_cnt;
                }
            }
            if (board_cnt < 8) {
                for (size_t j = 0; j < pending.size(); ++j) {
                    const auto& up = pending[j];
                    if (up.from_area != "bench" || skipped.count(up.unit) > 0
                        || occupied_cells.count({ up.to_r, up.to_c }) > 0) {
                        continue;
                    }
                    PlacementMove up_mv = up;
                    pending.erase(pending.begin() + static_cast<long>(j));
                    if (do_move(up_mv)) {
                        cell = find_bench_cell();
                    }
                    break;
                }
            }
        }
        if (!cell) {
            // `整备区已满，X 留在场上不换下`；并把最后一个 bench 目标动作加入 `Je`
            log_info("整备区已满，" + (mv.unit.empty() ? std::string("对象") : mv.unit) + " 留在场上不换下");
            for (auto it = pending.rbegin(); it != pending.rend(); ++it) {
                if (it->from_area == "bench" && skipped.count(it->unit) == 0) {
                    skipped.insert(it->unit);
                    break;
                }
            }
            continue;
        }
        // `At(Ve, re, _e, _e.r === Ze)`：目标必须是 H 行的具体空格，不能是棋盘 (0,0)
        mv.to_r = cell->first;
        mv.to_c = cell->second;
        mv.facing = "RIGHT";
        do_move(mv);
    }
    pending.erase(
        std::remove_if(pending.begin(), pending.end(), [&](const PlacementMove& m) {
            return skipped.count(m.unit) > 0;
        }),
        pending.end());

    // ---- `pt` (L9839-9900)：把方案内单位移到目标格 ----
    std::unordered_map<std::string, int> park_count; // `$t`
    for (int loop = 0; loop < 40 && !pending.empty() && !need_exit() && !need_restart; ++loop) {
        // 1. `pt.findIndex(He => !ze.some(lt => lt !== He.unit && lt.r === He.r && lt.c === He.c))`
        int ready_idx = -1;
        for (size_t i = 0; i < pending.size(); ++i) {
            const auto& mv = pending[i];
            if (mv.type == "orient"
                || (mv.from_r == mv.to_r && mv.from_c == mv.to_c)
                || occupied_cells.count({ mv.to_r, mv.to_c }) == 0) {
                ready_idx = static_cast<int>(i);
                break;
            }
        }
        if (ready_idx >= 0) {
            PlacementMove mv = pending[ready_idx];
            pending.erase(pending.begin() + ready_idx);
            do_move(mv);
            continue;
        }

        // 2. `Re() ?? xe(...)`
        auto bench_cell = find_bench_cell();
        const bool to_bench = bench_cell.has_value();
        std::pair<int, int> cell = to_bench ? *bench_cell : std::make_pair(-1, -1);
        if (!to_bench) {
            if (auto board_cell = find_board_cell()) {
                cell = *board_cell;
            }
            else {
                // `整备区已满，保留当前站位`
                log_info("整备区已满，保留当前站位");
                break;
            }
        }

        // `$t.get(He) < 2` / `站位调整陷入循环，保留剩余单位原位`
        const std::string& cyc_unit = pending.front().unit;
        if (++park_count[cyc_unit] > 2) {
            log_info("站位调整陷入循环，保留剩余单位原位");
            break;
        }

        auto [tr, tc] = cell;
        PlacementMove& cyc_mv = pending.front();
        PlacementMove park_mv {
            to_bench ? "bench" : "reposition",
            cyc_mv.unit,
            cyc_mv.from_area,
            cyc_mv.from_r,
            cyc_mv.from_c,
            to_bench ? "bench" : "board",
            tr,
            tc,
            cyc_mv.facing.empty() ? "LEFT" : cyc_mv.facing
        };
        if (execute_placement_move(park_mv, cur_cam)) {
            occupied_cells.erase({ cyc_mv.from_r, cyc_mv.from_c });
            occupied_cells.emplace(tr, tc);
            cyc_mv.from_area = to_bench ? "bench" : "board";
            cyc_mv.from_r = tr;
            cyc_mv.from_c = tc;
            cyc_mv.type = to_bench ? "deploy" : "reposition";
            ++executed;
        }
        else {
            log_info("布阵动作: " + park_mv.type + " " + park_mv.unit + " -> ("
                     + std::to_string(tr) + "," + std::to_string(tc) + ") 朝向:" + park_mv.facing);
            on_move_fail(cyc_mv);
            pending.erase(pending.begin());
        }
    }
    if (!pending.empty()) {
        log_info("站位调整未全部完成（剩 " + std::to_string(pending.size()) + " 项），按当前站位开战");
    }

    if (need_restart) {
        close_detail_panel();
        return -1; // 外层按 `wa` 处理：`we(!0)` 重跑一次
    }

    sleep_ms(static_cast<int>(std::lround(300.0 * m_step_scale)));
    auto post_st = wait_stable_board(4000);
    if (!post_st.has_value()) {
        log_info("部署后棋盘未稳定");
        return executed;
    }
    cur_cam = CameraDetectResult { post_st->camera, post_st->raw_H, post_st->fit };
    refresh_board_snapshot(post_st->img, cur_cam, false);

    // `!H && (N(round) || benchCount >= Wt(state) - 2) && await o("deploy.summons", Dt(state, H))`
    {
        int summon_bench_cnt = 0;
        for (const auto& u : m_units) {
            if (u.area == "bench") {
                ++summon_bench_cnt;
            }
        }
        int summon_bench_cap = 10;
        for (const auto& stg : m_stages) {
            if (stg.stage_id != m_stage_id) {
                continue;
            }
            for (const auto& row : stg.rows) {
                if (row.find('H') == std::string::npos) {
                    continue;
                }
                const int cnt = static_cast<int>(std::count(row.begin(), row.end(), 'H'));
                summon_bench_cap = (cnt > 0) ? cnt : 10;
                break;
            }
            break;
        }
        // `!H && (N(round) || benchCount >= Wt(state) - 2)` → `Dt(state, H)`
        if (!mob_boss && (boss_round || summon_bench_cnt >= summon_bench_cap - 2)) {
            deploy_summons(traffic);
        }
    }

    // `X.useEquipment` → `await o("deploy.equip", on(state, H))`
    scan_and_distribute_equipment(post_st->img, cur_cam);
    return executed;
}

// ============================================================================
// Shop Multi-Step Action Planner & Execution (`_r`, `gs`, `ps`, `Lr`, `ha`, `ur`, `ct`, `oe`)
// ============================================================================

SPAFarmTaskPlugin::RoundPacing SPAFarmTaskPlugin::get_round_pacing(int round) const
{
    // Exact 1:1 port of `hs(a)` in service.pretty.mjs lines 7972-7982
    int r = std::max(1, round);
    if (r == 1) {
        return { 1, 3, 2, false };
    }
    if (r == 2) {
        return { 2, 4, 3, false };
    }
    if (r == 3) {
        return { 3, 5, 4, false };
    }
    // `hs` L7971：`boss: a === _t.lastRound`（quick=9 / standard=14）
    return { r, 6, 5, r == m_last_round };
}



// ============================================================================
// Shop Planning —— 严格对齐 service.pretty.mjs
//
// 调用链：`je`（准备阶段）→ `ha`（模拟执行 12 步）→ `ps`（单步决策）→ `Lr`（卡牌评分）
//
// 关键事实（决定了这里为什么只买干员）：
//   L10137  `f = e === "medal" || s.mode === "quick" ? "quick" : "standard"`
//           → goal 为 medal 时模式被强制 quick，`$e()` 恒 false，`ha` 收到的 `std` 为 undefined
//   L8303   `const E = m && Lt && _t.mode === "standard" ? gs(...) : ps(...)`
//           → 只有标准模式才走 `gs`；`gs` 的 L8044 才是唯一的装备购买入口
//   L8250   `Lr` 只对 `kind === "operator"` 的卡评分
//   → 因而 `ps` 路径（刷刻章）**从不买装备**。装备在刷章模式下只来自三选一奖励，
//     随后由 `on`（scan_and_distribute_equipment）分配给场上干员。
// ============================================================================

int SPAFarmTaskPlugin::bond_need(const std::string& bond) const
{
    // `xn.get(bond) ?? 3`：`en()` 用 bonds.json 的 `thresholds[0]` 填充
    if (const auto* b = find_bond(bond)) {
        if (!b->thresholds.empty()) {
            return b->thresholds.front();
        }
    }
    return 3;
}

std::optional<std::string> SPAFarmTaskPlugin::active_focus_bond(
    const ShopSimState& sim,
    bool include_bench) const
{
    // `fa(a, s, e = false)`  L7975-7980
    // 在聚焦盟约里找第一个「场上（或全部）持有该盟约的不同干员数 >= 阈值」的盟约
    std::unordered_set<std::string> names;
    for (const auto& u : sim.units) {
        if (u.name.empty()) {
            continue;
        }
        if (!include_bench && u.area != "board") {
            continue;
        }
        names.insert(u.name);
    }
    for (const auto& focus : m_active_core_bonds) {
        int cnt = 0;
        for (const auto& n : names) {
            if (const auto* op = find_operator(n)) {
                if (std::find(op->bond_names.begin(), op->bond_names.end(), focus) != op->bond_names.end()) {
                    ++cnt;
                }
            }
        }
        if (cnt >= bond_need(focus)) {
            return focus;
        }
    }
    return std::nullopt;
}

SPAFarmTaskPlugin::ShopSimState SPAFarmTaskPlugin::make_shop_sim(const ShopHeaderState& header) const
{
    ShopSimState sim;
    for (const auto& u : m_units) {
        if (u.name.empty()) {
            continue; // JS `t = s.units.map(c => c.name)` 允许 null，但后续都按有名字处理
        }
        sim.units.push_back(ShopSimUnit { u.name, u.area, u.elite, u.auxiliary, u.item });
    }
    sim.shop = m_shop;
    sim.level = header.level.value_or(m_current_shop_level);
    sim.gold = header.gold.value_or(m_current_gold);
    if (header.upgrade_cost.has_value()) {
        sim.upgrade_cost = header.upgrade_cost;
    }
    else if (header.upgrade_btn.has_value()) {
        sim.upgrade_cost = std::nullopt; // 按钮在但读不到价 → 视为未知
    }
    sim.refreshes = m_shop_refreshes;
    return sim;
}

std::vector<SPAFarmTaskPlugin::ScoredShopCard> SPAFarmTaskPlugin::score_shop_cards(
    const ShopSimState& sim) const
{
    // 1:1 port of `Lr(a, s, e)` —— service.pretty.mjs L8243-8286
    const int round = std::max(1, m_current_round);
    const bool medal_mode = (m_goal == "medal"); // JS `Lt` 的反面

    std::unordered_map<std::string, int> owned;   // t: 名字出现次数
    std::unordered_set<std::string> elite_names;  // `g` 用
    int melee_owned = 0;                          // `u`
    bool has_healer = false;
    for (const auto& u : sim.units) {
        if (u.name.empty()) {
            continue;
        }
        owned[u.name]++;
        if (u.elite) {
            elite_names.insert(u.name);
        }
        if (const auto* op = find_operator(u.name)) {
            if (op->position == "MELEE") {
                ++melee_owned;
            }
            if (op->is_healer) {
                has_healer = true;
            }
        }
    }

    // `m = /矩兽/.test(boss) && !ownedHealer`
    const bool boss_is_ju = m_current_boss.find("矩兽") != std::string::npos;
    const bool need_healer = boss_is_ju && !has_healer;

    // `w`：盟约 → 持有该盟约的干员数
    std::unordered_map<std::string, int> bond_owned;
    for (const auto& u : sim.units) {
        if (u.name.empty()) {
            continue;
        }
        if (const auto* op = find_operator(u.name)) {
            for (const auto& b : op->bond_names) {
                bond_owned[b]++;
            }
        }
    }

    const std::vector<std::string>& active = m_active_bond_names;

    auto in_focus = [&](const std::string& bond) {
        if (std::find(m_active_core_bonds.begin(), m_active_core_bonds.end(), bond) == m_active_core_bonds.end()) {
            return false;
        }
        // `(!f.length || f.includes(y))`
        if (active.empty()) {
            return true;
        }
        return std::find(active.begin(), active.end(), bond) != active.end();
    };

    std::vector<ScoredShopCard> out;
    for (const auto& card : sim.shop) {
        if (card.kind != "operator" || card.source != "ocr" || card.name.empty()) {
            continue;
        }
        const auto* op = find_operator(card.name);
        if (op == nullptr) {
            out.push_back(ScoredShopCard { card, -1.0, 0 });
            continue;
        }
        // `if (!l || Et.has(l.name)) return {score:-1}`
        if (elite_names.count(op->name) > 0) {
            out.push_back(ScoredShopCard { card, -1.0, op->purchase_price });
            continue;
        }

        std::vector<std::string> hit_focus; // `E`
        for (const auto& b : op->bond_names) {
            if (in_focus(b)) {
                hit_focus.push_back(b);
            }
        }
        const int copies = owned.count(op->name) ? owned[op->name] : 0; // `d`
        const bool has_elite_copy = elite_names.count(op->name) > 0;    // `g`

        double n = static_cast<double>(hit_focus.size()) * 80.0 + static_cast<double>(op->tier) * 2.0;

        // + 迅捷（`or`）命中数 * 40；medal 目标下 `or` 就是 {"迅捷"}
        int swift = 0;
        for (const auto& b : op->bond_names) {
            if (b == "迅捷" && (active.empty() || std::find(active.begin(), active.end(), b) != active.end())) {
                ++swift;
            }
        }
        n += static_cast<double>(swift) * 40.0;

        // + (ht.includes(name) && (E.length || Lt)) ? 90 : 0
        const bool is_stacker =
            in_stacker_list(op->name);
        if (is_stacker && (!hit_focus.empty() || !medal_mode)) {
            n += 90.0;
        }
        // + l.bondNames.some(y => f.includes(y)) ? 20 : 0
        if (!active.empty()) {
            bool any_active = false;
            for (const auto& b : op->bond_names) {
                if (std::find(active.begin(), active.end(), b) != active.end()) {
                    any_active = true;
                    break;
                }
            }
            if (any_active) {
                n += 20.0;
            }
        }
        // + er.includes(name) ? (round >= 6 ? 110 : 15) : 0
        if (std::find(kQuickFinishers.begin(), kQuickFinishers.end(), op->name) != kQuickFinishers.end()) {
            n += (round >= 6) ? 110.0 : 15.0;
        }
        // + round === 1 && ["灰喉","卡达"].includes(name) ? 180 : 0
        if (round == 1 && (op->name == "灰喉" || op->name == "卡达")) {
            n += 180.0;
        }
        // + round <= 2 && position === "RANGED" ? 30 : 0
        if (round <= 2 && op->position == "RANGED") {
            n += 30.0;
        }
        // + E.some(b => w.get(b) === 2) && !d ? 150 : 0
        if (copies == 0) {
            bool two_bond = false;
            for (const auto& b : hit_focus) {
                auto it = bond_owned.find(b);
                if (it != bond_owned.end() && it->second == 2) {
                    two_bond = true;
                    break;
                }
            }
            if (two_bond) {
                n += 150.0;
            }
        }
        // + healsAllies ? (m ? 240 : round >= 6 && st.includes("聚合") ? 45 : 0) : 0
        if (op->is_healer) {
            if (need_healer) {
                n += 240.0;
            }
            else if (round >= 6
                     && std::find(m_active_core_bonds.begin(), m_active_core_bonds.end(), "聚合") != m_active_core_bonds.end()) {
                n += 45.0;
            }
        }
        // d === 2 ? +100 : (d ? (shop 同名 >= 2 ? 0 : -120) : 0)
        if (copies == 2) {
            n += 100.0;
        }
        else if (copies > 0) {
            int same_in_shop = 0;
            for (const auto& c2 : sim.shop) {
                if (c2.name == op->name) {
                    ++same_in_shop;
                }
            }
            if (same_in_shop < 2) {
                n -= 120.0;
            }
        }
        // - (g ? 220 : 0)
        if (has_elite_copy) {
            n -= 220.0;
        }
        // - (round >= 6 && !E.length && !迅捷 && !er && !healsAllies && !(Lt && ht)) ? 120 : 0
        if (round >= 6 && hit_focus.empty() && swift == 0
            && std::find(kQuickFinishers.begin(), kQuickFinishers.end(), op->name) == kQuickFinishers.end()
            && !op->is_healer && !(!medal_mode && is_stacker)) {
            n -= 120.0;
        }
        // + round >= 4 && melee_owned < 2 && position === "MELEE" ? 220 : 0
        if (round >= 4 && melee_owned < 2 && op->position == "MELEE") {
            n += 220.0;
        }

        out.push_back(ScoredShopCard { card, n, op->purchase_price });
    }

    // `.sort((c, l) => l.score - c.score || c.card.x - l.card.x)`
    std::sort(out.begin(), out.end(), [](const ScoredShopCard& a, const ScoredShopCard& b) {
        if (a.score != b.score) {
            return a.score > b.score;
        }
        return a.card.x < b.card.x;
    });
    return out;
}

// `tr(a)` —— service.pretty.mjs L7924-7935
// 取所有特质里「层数+N」的最大值；`至多叠加M层` 覆盖 N；`每有/每名/...` 记 8；
// `自身及(身前|身后)一格` 且无上限时 ×2。
int SPAFarmTaskPlugin::stack_potential(const OperatorInfo& op)
{
    static const std::regex kLayer(R"(层数\+([0-9]+))", std::regex::ECMAScript);
    static const std::regex kCap(R"(至多叠加([0-9]+)层)", std::regex::ECMAScript);
    static const std::regex kPer(R"(每有|每名|每获得|每进行|每消耗|每击倒|存活的我方)", std::regex::ECMAScript);
    static const std::regex kSelfAdj(R"(自身及(身前|身后)一格)", std::regex::ECMAScript);

    int best = 0;
    for (const auto& tr : op.traits) {
        std::string t = tr.text;
        std::string clean;
        clean.reserve(t.size());
        for (size_t k = 0; k < t.size();) {
            const unsigned char ch = static_cast<unsigned char>(t[k]);
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v') {
                ++k;
                continue;
            }
            if (t.compare(k, 3, "　") == 0) {
                k += 3;
                continue;
            }
            clean.push_back(t[k]);
            ++k;
        }
        if (clean.rfind("<获得时>", 0) == 0) {
            continue;
        }
        std::smatch m;
        if (!std::regex_search(clean, m, kLayer)) {
            continue;
        }
        int i = 0;
        std::smatch cap;
        const bool has_cap = std::regex_search(clean, cap, kCap);
        if (has_cap) {
            i = std::stoi(cap[1].str());
        }
        else if (std::regex_search(clean, m, kPer)) {
            i = 8;
        }
        else {
            i = std::stoi(m[1].str());
        }
        if (!has_cap && std::regex_search(clean, kSelfAdj)) {
            i *= 2;
        }
        best = std::max(best, i);
    }
    return best;
}

// `Bn(a)` —— L7979：叠层名单里的干员，且叠层潜力 > 3
bool SPAFarmTaskPlugin::is_stacker(const OperatorInfo& op) const
{
    // `Bn = a => !!a && ht.includes(a.name) && tr(a) > 3` —— L7978
    if (!in_stacker_list(op.name)) {
        return false;
    }
    return stack_potential(op) > 3;
}

// `_r(a, s)` —— service.pretty.mjs L7984-8015
std::vector<SPAFarmTaskPlugin::ShopCandidate> SPAFarmTaskPlugin::rank_shop_candidates(
    const ShopSimState& sim) const
{
    const auto& focus = m_active_core_bonds;
    const auto& active = m_active_bond_names;

    // `t(c)`：卡牌盟约里既属于聚焦盟约、又（若已读到激活列表）已激活的那些
    auto focus_active_bonds = [&](const std::vector<std::string>& names) {
        std::vector<std::string> r;
        for (const auto& n : names) {
            if (std::find(focus.begin(), focus.end(), n) == focus.end()) {
                continue;
            }
            if (!active.empty() && std::find(active.begin(), active.end(), n) == active.end()) {
                continue;
            }
            r.push_back(n);
        }
        return r;
    };

    std::unordered_set<std::string> owned; // `f`
    for (const auto& u : sim.units) {
        if (!u.name.empty()) {
            owned.insert(u.name);
        }
    }
    for (const auto& n : m_bought_operators) {
        owned.insert(n);
    }

    std::vector<const OperatorInfo*> stackers; // `w`
    for (const auto& n : owned) {
        if (const auto* op = find_operator(n); op != nullptr && is_stacker(*op)) {
            stackers.push_back(op);
        }
    }
    const OperatorInfo* weakest = nullptr; // `i`
    for (const auto* op : stackers) {
        if (weakest == nullptr || stack_potential(*op) < stack_potential(*weakest)) {
            weakest = op;
        }
    }

    auto owned_count = [&](const std::string& name) {
        int c = 0;
        for (const auto& u : sim.units) {
            if (u.name == name) {
                ++c;
            }
        }
        return c;
    };

    std::vector<ShopCandidate> out;
    for (const auto& card : sim.shop) {
        if (card.kind != "operator" || card.source != "ocr" || card.name.empty()) {
            continue;
        }
        if (m_elite_units.count(card.name) > 0) {
            continue;
        }
        const auto* op = find_operator(card.name);
        if (op == nullptr) {
            continue;
        }
        ShopCandidate cand;
        cand.card = card;
        cand.unit_name = op->name;
        cand.unit_tier = op->tier;
        cand.unit_price = op->purchase_price;

        const auto tl = focus_active_bonds(op->bond_names);
        const bool have = owned.count(op->name) > 0;

        if (!tl.empty() && !have) {
            cand.tier = 1;
            out.push_back(cand);
        }
        else if (is_stacker(*op) && !have) {
            if (static_cast<int>(stackers.size()) < 2) {
                cand.tier = 2;
                out.push_back(cand);
            }
            else if (weakest != nullptr && stack_potential(*weakest) < stack_potential(*op)) {
                cand.tier = 2;
                cand.sell = weakest->name;
                cand.has_sell = true;
                out.push_back(cand);
            }
        }
        else if ((!tl.empty() || is_stacker(*op)) && have && owned_count(op->name) < 3) {
            cand.tier = 3;
            out.push_back(cand);
        }
    }

    // JS：`c.tier - l.tier || tr(l.unit) - tr(c.unit) || l.unit.tier - c.unit.tier`
    std::stable_sort(out.begin(), out.end(), [&](const ShopCandidate& a, const ShopCandidate& b) {
        if (a.tier != b.tier) {
            return a.tier < b.tier;
        }
        const auto* oa = find_operator(a.unit_name);
        const auto* ob = find_operator(b.unit_name);
        const int ta = oa == nullptr ? 0 : stack_potential(*oa);
        const int tb = ob == nullptr ? 0 : stack_potential(*ob);
        if (ta != tb) {
            return tb < ta;
        }
        return b.unit_tier < a.unit_tier;
    });
    return out;
}

// `gs(a, s, e, t)` —— service.pretty.mjs L8017-8140（标准模式决策）
SPAFarmTaskPlugin::ShopPlanStep SPAFarmTaskPlugin::gs_step(ShopSimState& sim, PrepTrackState& prep) const
{
    const int round = m_current_round;
    const auto cands = rank_shop_candidates(sim);

    std::unordered_set<std::string> owned_now; // `i`
    for (const auto& u : sim.units) {
        if (!u.name.empty()) {
            owned_now.insert(u.name);
        }
    }
    for (const auto& n : m_bought_operators) {
        owned_now.insert(n);
    }

    int named_count = 0;
    for (const auto& u : sim.units) {
        if (!u.name.empty() && !u.auxiliary && !u.item) {
            ++named_count;
        }
    }
    int bench_count = 0;
    for (const auto& u : sim.units) {
        if (u.area == "bench") {
            ++bench_count;
        }
    }

    std::string filler_present; // `E`
    for (const auto& n : m_filler_units) {
        for (const auto& u : sim.units) {
            if (u.name == n) {
                filler_present = n;
                break;
            }
        }
        if (!filler_present.empty()) {
            break;
        }
    }

    const bool bonus_gold = (named_count >= 8 && !filler_present.empty());

    std::vector<int> m_idx; // 买得起的候选下标
    for (int k = 0; k < static_cast<int>(cands.size()); ++k) {
        const int extra = (cands[k].has_sell || bonus_gold) ? 1 : 0;
        if (cands[k].unit_price <= sim.gold + extra) {
            m_idx.push_back(k);
        }
    }
    const bool bench_full = bench_count >= 9;

    std::optional<int> first_pick;
    if (!bench_full && !m_idx.empty()) {
        first_pick = m_idx.front();
    }

    auto make_step = [&](int idx) -> ShopPlanStep {
        const auto& c = cands[idx];
        ShopPlanStep st;
        st.target = c.card.name;
        st.kind = c.card.kind;
        st.x = c.card.x;
        st.cost = c.unit_price;
        if (c.has_sell) {
            st.action = "swap";
            st.sell_name = c.sell;
            for (const auto& un : m_units) {
                if (un.name == c.sell) {
                    st.sell_slot = un.slot;
                    break;
                }
            }
            st.reason = "换更强叠层手 " + c.card.name;
        }
        else if (named_count >= 8 && !filler_present.empty()) {
            st.action = "swap";
            st.sell_name = filler_present;
            for (const auto& un : m_units) {
                if (un.name == filler_present) {
                    st.sell_slot = un.slot;
                    break;
                }
            }
            st.reason = "整备区已满，卖补位 " + filler_present;
        }
        else {
            st.action = "buy";
            st.reason = "标准模式";
        }
        return st;
    };

    const bool can_upgrade =
        sim.upgrade_cost.has_value() && sim.level < 6 && sim.gold >= *sim.upgrade_cost;
    const bool can_refresh = sim.gold >= 1 && sim.refreshes < 30;

    std::optional<int> overflow; // `y`：买不起的第一张（留着冻结）
    for (int k = 0; k < static_cast<int>(cands.size()); ++k) {
        if (std::find(m_idx.begin(), m_idx.end(), k) == m_idx.end()) {
            overflow = k;
            break;
        }
    }

    // 物品卡（`b` / `C()`）
    std::optional<int> item_idx;
    if (prep.use_items && !prep.item_bought) {
        for (int k = 0; k < static_cast<int>(sim.shop.size()); ++k) {
            const auto& c = sim.shop[k];
            if (c.kind == "item" && !c.name.empty() && c.name.find("寻呼") == std::string::npos) {
                item_idx = k;
                break;
            }
        }
    }
    int item_price = 99;
    if (item_idx.has_value()) {
        const auto* it = find_item(sim.shop[*item_idx].name);
        item_price = it == nullptr ? 99 : it->price;
    }
    auto try_item = [&]() -> std::optional<ShopPlanStep> {
        if (!item_idx.has_value() || sim.gold < item_price) {
            return std::nullopt;
        }
        const auto& c = sim.shop[*item_idx];
        ShopPlanStep st;
        st.action = "buy";
        st.kind = c.kind;
        st.target = c.name;
        st.x = c.x;
        st.cost = item_price;
        st.reason = "装备";
        return st;
    };
    auto finish = [&]() -> ShopPlanStep {
        if (auto st = try_item(); st.has_value()) {
            return *st;
        }
        if (overflow.has_value()) {
            ShopPlanStep st;
            st.action = "freeze";
            st.target = cands[*overflow].unit_name;
            st.reason = "冻结保留";
            return st;
        }
        ShopPlanStep st;
        st.action = "done";
        return st;
    };

    // `me()`：能买到的干员卡，按 tier 降序
    struct ShopOp
    {
        int card_index = 0;
        const OperatorInfo* op = nullptr;
    };
    std::vector<ShopOp> op_cards;
    for (int k = 0; k < static_cast<int>(sim.shop.size()); ++k) {
        const auto& c = sim.shop[k];
        if (c.kind != "operator" || c.source != "ocr" || c.name.empty()) {
            continue;
        }
        if (m_elite_units.count(c.name) > 0) {
            continue;
        }
        const auto* op = find_operator(c.name);
        if (op == nullptr) {
            continue;
        }
        op_cards.push_back(ShopOp { k, op });
    }
    std::stable_sort(op_cards.begin(), op_cards.end(), [](const ShopOp& a, const ShopOp& b) {
        return b.op->tier < a.op->tier;
    });

    // `Be()`：2 块钱时买一名补位
    auto try_filler = [&]() -> std::optional<ShopPlanStep> {
        if (bench_full || sim.gold != 2 || prep.filler_bought) {
            return std::nullopt;
        }
        for (const auto& so : op_cards) {
            if (so.op->purchase_price > sim.gold || owned_now.count(so.op->name) > 0) {
                continue;
            }
            const auto& c = sim.shop[so.card_index];
            ShopPlanStep st;
            st.action = "buy";
            st.kind = c.kind;
            st.target = c.name;
            st.x = c.x;
            st.cost = so.op->purchase_price;
            st.reason = "前期补位";
            st.filler = true;
            return st;
        }
        return std::nullopt;
    };

    auto upgrade_step = [&]() {
        ShopPlanStep st;
        st.action = "upgrade";
        st.cost = *sim.upgrade_cost;
        st.reason = "升商店";
        return st;
    };
    auto refresh_step = [&]() {
        ShopPlanStep st;
        st.action = "refresh";
        st.reason = "刷新";
        return st;
    };

    if (round == 1) {
        if (prep.bought >= 2) {
            return finish();
        }
        if (first_pick.has_value()) {
            return make_step(*first_pick);
        }
        if (!bench_full) {
            const auto scored = score_shop_cards(sim);
            for (const auto& sc : scored) {
                if (sc.score <= 0) {
                    continue;
                }
                const auto* op = find_operator(sc.card.name);
                if (op == nullptr || op->purchase_price > sim.gold) {
                    continue;
                }
                ShopPlanStep st;
                st.action = "buy";
                st.kind = sc.card.kind;
                st.target = sc.card.name;
                st.x = sc.card.x;
                st.cost = op->purchase_price;
                st.reason = "评分最高";
                return st;
            }
        }
        return finish();
    }

    if (round == 2) {
        static const std::vector<std::string> kFront = { "TANK", "PIONEER", "WARRIOR" };
        std::vector<ShopOp> fronts;
        for (const auto& so : op_cards) {
            if (std::find(kFront.begin(), kFront.end(), so.op->profession) != kFront.end()) {
                fronts.push_back(so);
            }
        }
        bool have_front = false;
        for (const auto& n : owned_now) {
            const auto* op = find_operator(n);
            if (op != nullptr && std::find(kFront.begin(), kFront.end(), op->profession) != kFront.end()) {
                have_front = true;
                break;
            }
        }
        if (prep.front_plan || (!have_front && !fronts.empty() && prep.upgrades == 0)) {
            prep.front_plan = true;
            if (prep.bought >= 2) {
                return finish();
            }
            if (first_pick.has_value()) {
                return make_step(*first_pick);
            }
            for (const auto& so : fronts) {
                if (so.op->purchase_price > sim.gold) {
                    continue;
                }
                const auto& c = sim.shop[so.card_index];
                ShopPlanStep st;
                st.action = "buy";
                st.kind = c.kind;
                st.target = c.name;
                st.x = c.x;
                st.cost = so.op->purchase_price;
                st.reason = "补前排";
                return st;
            }
            for (const auto& so : op_cards) {
                if (so.op->purchase_price > sim.gold) {
                    continue;
                }
                const auto& c = sim.shop[so.card_index];
                ShopPlanStep st;
                st.action = "buy";
                st.kind = c.kind;
                st.target = c.name;
                st.x = c.x;
                st.cost = so.op->purchase_price;
                st.reason = "补人";
                return st;
            }
            return finish();
        }
        if (first_pick.has_value()) {
            return make_step(*first_pick);
        }
        if (prep.bought == 0 && prep.upgrades == 0 && can_upgrade) {
            return upgrade_step();
        }
        if (auto st = try_filler(); st.has_value()) {
            return *st;
        }
        return finish();
    }

    if (round == 3) {
        if (first_pick.has_value()) {
            return make_step(*first_pick);
        }
        if (overflow.has_value()) {
            ShopPlanStep st;
            st.action = "freeze";
            st.target = cands[*overflow].unit_name;
            st.reason = "冻结保留";
            return st;
        }
        if (auto st = try_filler(); st.has_value()) {
            return *st;
        }
        return can_refresh ? refresh_step() : finish();
    }

    if (first_pick.has_value()) {
        return make_step(*first_pick);
    }
    if (overflow.has_value()) {
        return finish();
    }
    if (prep.upgrades == 0 && sim.upgrade_cost.has_value() && sim.level < 6) {
        if (prep.bought == 0 && *sim.upgrade_cost + 2 >= prep.start_gold) {
            const int reserve = prep.start_gold - *sim.upgrade_cost;
            if (sim.refreshes < reserve && sim.gold - 1 >= *sim.upgrade_cost) {
                return refresh_step();
            }
            if (can_upgrade) {
                return upgrade_step();
            }
        }
        else if (can_upgrade) {
            return upgrade_step();
        }
    }
    if (auto st = try_item(); st.has_value()) {
        return *st;
    }
    return can_refresh ? refresh_step() : finish();
}

// `bs(a, s)` —— service.pretty.mjs L8387-8390：挑一张最值得冻结的卡
std::optional<std::string> SPAFarmTaskPlugin::pick_freeze_target(const ShopSimState& sim) const
{
    const auto& focus = m_active_core_bonds;
    const auto& active = m_active_bond_names;
    for (const auto& sc : score_shop_cards(sim)) {
        if (sc.score <= 0) {
            continue;
        }
        const auto* op = find_operator(sc.card.name);
        if (op == nullptr) {
            continue;
        }
        if (is_stacker(*op)) {
            return op->name;
        }
        for (const auto& b : op->bond_names) {
            if (std::find(focus.begin(), focus.end(), b) == focus.end()) {
                continue;
            }
            if (!active.empty() && std::find(active.begin(), active.end(), b) == active.end()) {
                continue;
            }
            return op->name;
        }
    }
    return std::nullopt;
}

SPAFarmTaskPlugin::ShopPlanStep SPAFarmTaskPlugin::ps_step(ShopSimState& sim) const
{
    // `ps(a, s, e)` —— service.pretty.mjs L8142-8241，逐分支对齐
    const int round = std::max(1, m_current_round);
    const RoundPacing pacing = get_round_pacing(round);
    const bool not_medal = (m_goal != "medal"); // `Lt`
    const int level = sim.level;
    const int gold = sim.gold;

    // `w`
    const bool want_upgrade = level < (not_medal ? std::min(pacing.target_shop_level, 5) : pacing.target_shop_level);

    int combat_units = 0;
    int bench_units = 0;
    std::vector<const ShopSimUnit*> real_units; // !auxiliary && !item
    std::unordered_set<std::string> owned;     // `c`
    std::unordered_set<std::string> elite_names; // `Et`
    for (const auto& u : sim.units) {
        if (!u.auxiliary) {
            ++combat_units;
        }
        if (u.area == "bench") {
            ++bench_units;
        }
        if (!u.name.empty()) {
            owned.insert(u.name);
            if (u.elite) {
                elite_names.insert(u.name);
            }
            if (!u.auxiliary && !u.item) {
                real_units.push_back(&u);
            }
        }
    }
    const bool need_units = combat_units < pacing.min_deploy; // `i`

    // `m` / `u`
    std::vector<ScoredShopCard> scored = score_shop_cards(sim);
    std::vector<ScoredShopCard> affordable;
    for (const auto& sc : scored) {
        if (sc.score <= 0.0)
            continue;
        if (sc.price <= 0 || sc.price > gold)
            continue;
        if (bench_units >= 9)
            continue;
        affordable.push_back(sc);
    }

    const std::vector<std::string>& active = m_active_bond_names;
    auto in_focus = [&](const std::string& bond) {
        if (std::find(m_active_core_bonds.begin(), m_active_core_bonds.end(), bond) == m_active_core_bonds.end())
            return false;
        if (active.empty())
            return true;
        return std::find(active.begin(), active.end(), bond) != active.end();
    };
    // `E(unit)`
    auto unit_focus_bonds = [&](const OperatorInfo& op) {
        std::vector<std::string> out;
        for (const auto& b : op.bond_names) {
            if (in_focus(b))
                out.push_back(b);
        }
        return out;
    };
    auto bond_need_count = [&](const std::string& bond) { return bond_need(bond); };
    // `b(bond)`：还差几个
    auto shortage = [&](const std::string& bond) {
        std::set<std::string> names;
        for (const auto& u : sim.units) {
            if (u.name.empty() || owned.count(u.name) == 0)
                continue;
            const auto* op = find_operator(u.name);
            if (op == nullptr)
                continue;
            if (std::find(op->bond_names.begin(), op->bond_names.end(), bond) != op->bond_names.end())
                names.insert(op->name);
        }
        return bond_need_count(bond) - static_cast<int>(names.size());
    };
    std::vector<std::string> focus_list; // `M`
    for (const auto& b : m_active_core_bonds) {
        if (active.empty() || std::find(active.begin(), active.end(), b) != active.end())
            focus_list.push_back(b);
    }
    bool any_focus_done = false; // `C`
    for (const auto& b : focus_list) {
        if (shortage(b) <= 0) {
            any_focus_done = true;
            break;
        }
    }

    auto buy = [](const ScoredShopCard& sc) {
        ShopPlanStep st;
        st.action = "buy";
        st.target = sc.card.name;
        st.kind = sc.card.kind;
        st.x = sc.card.x;
        st.cost = sc.price;
        return st;
    };
    auto refresh = [](const std::string& reason) {
        ShopPlanStep st;
        st.action = "refresh";
        st.kind = "refresh";
        st.cost = 1;
        st.reason = reason;
        return st;
    };

    // 1) `g`：某聚焦盟约已凑到 2 名，再买一名即可激活
    for (const auto& sc : affordable) {
        const auto* op = find_operator(sc.card.name);
        if (op == nullptr || owned.count(op->name) > 0)
            continue;
        for (const auto& b : unit_focus_bonds(*op)) {
            int cnt = 0;
            for (const auto& n : owned) {
                const auto* o = find_operator(n);
                if (o != nullptr && std::find(o->bond_names.begin(), o->bond_names.end(), b) != o->bond_names.end())
                    ++cnt;
            }
            if (cnt == 2)
                return buy(sc);
        }
    }

    // 2) `Lt && cs && level>=3 && M.length && !C`
    if (not_medal && level >= 3 && !focus_list.empty() && !any_focus_done) {
        auto min_shortage = [&](const OperatorInfo& op) {
            int best = std::numeric_limits<int>::max();
            for (const auto& b : unit_focus_bonds(op))
                best = std::min(best, shortage(b));
            return best;
        };
        const ScoredShopCard* pick = nullptr;
        int pick_s = std::numeric_limits<int>::max();
        for (const auto& sc : affordable) {
            const auto* op = find_operator(sc.card.name);
            if (op == nullptr)
                continue;
            if (unit_focus_bonds(*op).empty() || owned.count(op->name) > 0)
                continue;
            const int s = min_shortage(*op);
            if (s < pick_s) {
                pick_s = s;
                pick = &sc;
            }
        }
        if (pick != nullptr)
            return buy(*pick);
        for (const auto& sc : affordable) {
            if (in_stacker_list(sc.card.name)
                && owned.count(sc.card.name) == 0)
                return buy(sc);
        }
        if (gold >= 3 && sim.refreshes < 20)
            return refresh("补盟约刷新");
    }

    // 3) `t <= 2 && i && u.length`
    if (round <= 2 && need_units && !affordable.empty())
        return buy(affordable.front());

    // 4) `Be`：三合一
    for (const auto& sc : affordable) {
        const auto* op = find_operator(sc.card.name);
        if (op == nullptr || elite_names.count(op->name) > 0)
            continue;
        int own = 0;
        for (const auto& u : sim.units) {
            if (u.name == op->name && !u.elite)
                ++own;
        }
        if (own >= 3)
            continue;
        int in_shop = 0;
        for (const auto& c2 : sim.shop) {
            if (c2.name == op->name)
                ++in_shop;
        }
        if (own + in_shop < 3)
            continue;
        if ((3 - own) * sc.price > gold)
            continue;
        return buy(sc);
    }

    // 5) `Lt`：先补盟约，再补叠层
    if (not_medal) {
        for (const auto& sc : affordable) {
            const auto* op = find_operator(sc.card.name);
            if (op != nullptr && !unit_focus_bonds(*op).empty() && owned.count(op->name) == 0)
                return buy(sc);
        }
        for (const auto& sc : affordable) {
            if (in_stacker_list(sc.card.name)
                && owned.count(sc.card.name) == 0)
                return buy(sc);
        }
    }

    // 6) `X`：极速 4~6 回合补人
    const bool rush = (m_mode == "quick" && round >= 4 && round <= 6 && combat_units < 5);
    if (rush && !affordable.empty())
        return buy(affordable.front());

    // 7) 升商店
    if (!rush && want_upgrade && sim.upgrade_cost.has_value() && gold >= *sim.upgrade_cost) {
        ShopPlanStep st;
        st.action = "upgrade";
        st.kind = "upgrade";
        st.cost = *sim.upgrade_cost;
        st.target = "商店Lv" + std::to_string(level + 1);
        return st;
    }

    // 8) `Lt && (!C || !w)` 刷新
    auto is_useful = [&](const ScoredShopCard& sc) {
        const auto* op = find_operator(sc.card.name);
        if (op == nullptr)
            return false;
        return in_stacker_list(op->name)
            || !unit_focus_bonds(*op).empty();
    };
    if (not_medal && (!any_focus_done || !want_upgrade) && round >= 2 && sim.refreshes < 10 && gold >= 3
        && !std::any_of(affordable.begin(), affordable.end(), is_useful))
        return refresh("补盟约刷新");

    // 9) `N`：叠层干员（medal 需命中聚焦盟约）
    for (const auto& sc : affordable) {
        const auto* op = find_operator(sc.card.name);
        if (op == nullptr)
            continue;
        const bool stacker = in_stacker_list(op->name);
        if (stacker && (not_medal || !unit_focus_bonds(*op).empty()))
            return buy(sc);
    }

    // 10) `U` / `q`：矩兽 BOSS 补治疗
    const bool boss_is_ju = m_current_boss.find("矩兽") != std::string::npos;
    const ScoredShopCard* healer = nullptr;
    for (const auto& sc : affordable) {
        const auto* op = find_operator(sc.card.name);
        if (op != nullptr && op->is_healer && owned.count(op->name) == 0) {
            healer = &sc;
            break;
        }
    }
    bool has_healer = false;
    for (const auto& u : sim.units) {
        if (u.name.empty())
            continue;
        const auto* op = find_operator(u.name);
        if (op != nullptr && op->is_healer && owned.count(op->name) > 0)
            has_healer = true;
    }
    if (healer != nullptr && !has_healer && boss_is_ju)
        return buy(*healer);

    // 11) `p`：第 6 回合后补终结者
    if (round >= 6) {
        for (const auto& sc : affordable) {
            const auto* op = find_operator(sc.card.name);
            if (op == nullptr)
                continue;
            if (std::find(kQuickFinishers.begin(), kQuickFinishers.end(), op->name) != kQuickFinishers.end()
                && !unit_focus_bonds(*op).empty() && owned.count(op->name) == 0)
                return buy(sc);
        }
        if ((healer != nullptr) && !has_healer
            && std::find(m_active_core_bonds.begin(), m_active_core_bonds.end(), "聚合") != m_active_core_bonds.end())
            return buy(*healer);
    }

    // 12) 近战不足 2
    int melee_owned = 0;
    for (const auto& u : real_units) {
        const auto* op = find_operator(u->name);
        if (op != nullptr && op->position == "MELEE")
            ++melee_owned;
    }
    if (round >= 4 && melee_owned < 2) {
        for (const auto& sc : affordable) {
            const auto* op = find_operator(sc.card.name);
            if (op != nullptr && op->position == "MELEE")
                return buy(sc);
        }
    }

    // 13) `!Lt && quick && d && !w`
    const bool bond_done = active_focus_bond(sim, true).has_value(); // `d`
    if (!not_medal && m_mode == "quick" && bond_done && !want_upgrade) {
        for (const auto& sc : affordable) {
            const auto* op = find_operator(sc.card.name);
            if (op == nullptr || elite_names.count(op->name) > 0 || owned.count(op->name) > 0)
                continue;
            bool ok = !unit_focus_bonds(*op).empty();
            if (!ok) {
                for (const auto& b : op->bond_names) {
                    if (b == "迅捷") {
                        ok = true;
                        break;
                    }
                }
            }
            if (!ok && in_stacker_list(op->name))
                ok = true;
            if (ok)
                return buy(sc);
        }
        // 买同名凑三连
        struct DupPick
        {
            const ShopCard* card = nullptr;
            int price = 0;
            int own = 0;
            int tier = 0;
        };
        std::optional<DupPick> best_dup;
        const bool bench_room = bench_units < 9;
        for (const auto& card : sim.shop) {
            if (card.kind != "operator" || card.source != "ocr" || card.name.empty())
                continue;
            if (owned.count(card.name) == 0)
                continue;
            if (elite_names.count(card.name) > 0)
                continue;
            int own = 0;
            for (const auto& u : sim.units) {
                if (u.name == card.name && !u.auxiliary)
                    ++own;
            }
            if (own >= 3)
                continue;
            if (!(bench_room || own == 2))
                continue;
            const auto* op = find_operator(card.name);
            if (op == nullptr)
                continue;
            const int price = op->purchase_price;
            if (price > gold)
                continue;
            bool useful = !unit_focus_bonds(*op).empty();
            if (!useful) {
                for (const auto& b : op->bond_names) {
                    if (b == "迅捷") {
                        useful = true;
                        break;
                    }
                }
            }
            if (!useful && in_stacker_list(op->name))
                useful = true;
            if (!useful)
                continue;
            if (!best_dup.has_value() || own < best_dup->own
                || (own == best_dup->own && op->tier > best_dup->tier)) {
                best_dup = DupPick { &card, price, own, op->tier };
            }
        }
        if (best_dup.has_value()) {
            ShopPlanStep st;
            st.action = "buy";
            st.target = best_dup->card->name;
            st.kind = best_dup->card->kind;
            st.x = best_dup->card->x;
            st.cost = best_dup->price;
            return st;
        }
        if (gold >= 3 && sim.refreshes < 20)
            return refresh("凑同名刷新");
    }

    // 14) `Ee` / `ds` 刷新
    auto is_valuable = [&](const ScoredShopCard& sc) {
        const auto* op = find_operator(sc.card.name);
        if (op == nullptr)
            return false;
        if (!unit_focus_bonds(*op).empty())
            return true;
        if (in_stacker_list(op->name))
            return true;
        if (std::find(kQuickFinishers.begin(), kQuickFinishers.end(), op->name) != kQuickFinishers.end())
            return true;
        return op->is_healer && !has_healer && boss_is_ju;
    };
    if (gold >= 3 && sim.refreshes < 10 && round >= 3 && !affordable.empty()
        && !is_valuable(affordable.front()) && gold - 1 >= affordable.front().price
        && (need_units || (!want_upgrade && round >= 6 && !bond_done)))
        return refresh("低质商店刷新");

    // 15) 兜底
    if (!affordable.empty()) {
        const bool cond_a = need_units || (!want_upgrade && round >= 6 && !bond_done);
        const bool can_upgrade = want_upgrade && sim.upgrade_cost.has_value() && gold >= *sim.upgrade_cost;
        if (cond_a || rush || !can_upgrade)
            return buy(affordable.front());
    }

    // 16) 冻结
    for (const auto& sc : scored) {
        const auto* op = find_operator(sc.card.name);
        if (op == nullptr)
            continue;
        if (in_stacker_list(op->name)
            && !unit_focus_bonds(*op).empty()) {
            ShopPlanStep st;
            st.action = "freeze";
            st.target = op->name;
            st.kind = "operator";
            return st;
        }
    }
    if (!want_upgrade && round >= 6 && !bond_done && gold >= 4 && sim.refreshes < 8)
        return refresh("末期搜卡");

    ShopPlanStep done;
    done.action = "done";
    return done;
}

std::vector<SPAFarmTaskPlugin::ShopPlanStep> SPAFarmTaskPlugin::plan_shop_Steps(
    const ShopHeaderState& header,
    const std::vector<ShopCard>& shop)
{
    // `ha(a, s, e)` —— service.pretty.mjs L8292-8382
    // `je` L10040：`Je = { ...Ve, shop: Ve.shop.filter(De => !Xe.has(Math.round(De.x / 100))) }`
    // 规划用的卡面是「剔除买失败槽位」之后的副本，不是完整 `m_shop`。
    // 在状态副本上模拟最多 12 步，避免「执行一步再重读」带来的状态漂移。
    ShopSimState sim = make_shop_sim(header);
    sim.shop = shop;

    // `$e()`：只有 task 目标 + standard 模式才走 `gs`；medal 恒走 `ps`（`Lt` 为假）
    const bool standard = (m_mode == "standard" && m_goal != "medal");
    if (m_prep_track_round != m_current_round) {
        m_prep_track_round = m_current_round;
        m_prep_track = PrepTrackState {};
        m_prep_track.start_gold = -1;
    }
    PrepTrackState prep = m_prep_track;
    if (prep.start_gold < 0) {
        prep.start_gold = sim.gold; // `$e() && Ce.startGold < 0 && (Ce.startGold = xe.gold)`
    }
    prep.use_items = true; // JS 默认 `X.useEquipment = true`

    std::vector<ShopPlanStep> steps;
    for (int l = 0; l < 12; ++l) {
        ShopPlanStep E = standard ? gs_step(sim, prep) : ps_step(sim);

        auto drop_card = [&](const ShopCard& card) {
            for (auto it = sim.shop.begin(); it != sim.shop.end(); ++it) {
                if (it->x == card.x && it->name == card.name) {
                    sim.shop.erase(it);
                    return;
                }
            }
        };

        if (E.action == "swap") {
            steps.push_back(E);
            sim.gold += 1 - E.cost; // 卖价恒为 1（`soldPrice` 最低 1；JS `w += 1 - E.price`）
            for (auto it = sim.units.begin(); it != sim.units.end(); ++it) {
                if (it->name == E.sell_name) {
                    sim.units.erase(it);
                    break;
                }
            }
            ShopCard sold_card;
            sold_card.x = E.x;
            sold_card.name = E.target;
            drop_card(sold_card);
            ShopSimUnit nu;
            nu.name = E.target;
            nu.area = "bench";
            sim.units.push_back(nu);
            if (standard) {
                ++prep.bought;
            }
            continue;
        }
        if (E.action == "done") {
            break;
        }
        if (E.action == "freeze" || E.action == "refresh") {
            steps.push_back(E);
            break;
        }
        if (E.action == "upgrade") {
            steps.push_back(E);
            sim.gold -= E.cost;
            ++sim.level;
            if (standard) {
                ++prep.upgrades;
            }
            // `i = e.knownUpgradeCosts?.[f] ?? null`：只有 `!verifyUpgrade && level===1 && round===1`
            // 时 JS 才把价格表传进来，其余情况一律视为未知并停止本批
            const bool has_known_costs = !m_verify_upgrade && m_current_shop_level == 1 && m_current_round == 1;
            if (has_known_costs) {
                if (auto it = m_shop_upgrade_costs.find(sim.level); it != m_shop_upgrade_costs.end()) {
                    sim.upgrade_cost = it->second;
                    continue;
                }
            }
            sim.upgrade_cost.reset();
            break;
        }

        // buy
        steps.push_back(E);
        sim.gold -= E.cost;
        if (standard) {
            if (E.kind == "item") {
                prep.item_bought = true;
            }
            else {
                ++prep.bought;
            }
            if (E.filler) {
                prep.filler_bought = true;
            }
        }
        ShopCard bought_card;
        bought_card.x = E.x;
        bought_card.name = E.target;
        drop_card(bought_card);
        if (E.kind != "item") {
            ShopSimUnit nu;
            nu.name = E.target;
            nu.area = "bench";
            sim.units.push_back(nu);
        }
    }
    m_prep_track = prep;
    return steps;
}

bool SPAFarmTaskPlugin::buy_shop_card(int card_x, const std::string& expected_name, int upgrade_x_hint)
{
    // Exact 1:1 port of `Tt(I, k)` (`ur`) in service.pretty.mjs lines 8857-8907:
    // Snaps x to 243px grid from upgrade_x_hint if available, taps (B + 65, 900),
    // polls up to 1500ms checking [B - 80, 985, 320, 80] for `/确认购买/` and [20, 265, 560, 70]
    // for the detail panel name (`ot`). If name mismatches expected_name, closes detail and aborts!
    // Otherwise taps (B + 65, 1025) to confirm purchase.
    int B = card_x;
    if (upgrade_x_hint > 0) {
        int slot_k = std::max(1, static_cast<int>(std::lround(static_cast<double>(card_x + 65 - upgrade_x_hint) / 243.0)));
        B = upgrade_x_hint + 243 * slot_k - 65;
    }

    tap(B + 65, 900, 180);

    bool confirmed_ui = false;
    const auto t0 = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count() < 1500
           && !need_exit()) {
        cv::Mat cur = shot();
        auto btn_ocr = ocr(cur, std::array<int, 4> { std::max(0, B - 80), 985, 320, 80 });
        auto title_ocr = ocr(cur, std::array<int, 4> { 20, 265, 560, 70 });

        std::string panel_title;
        for (const auto& it : title_ocr) {
            panel_title += it.text;
        }
        if (!expected_name.empty() && !panel_title.empty()) {
            auto cm = match_dict(panel_title, m_chess_dict);
            auto im = match_dict(panel_title, m_item_dict);
            std::string det_name = !cm.empty() ? cm.front().name : (!im.empty() ? im.front().name : "");
            if (!det_name.empty() && norm_text(det_name) != norm_text(expected_name)) {
                log_info("购买核名不符: 期望 " + expected_name + " 实测 " + det_name + "，取消购买");
                close_detail_panel();
                return false;
            }
        }
        if (has_ocr(btn_ocr, { "确认购买" }) || !panel_title.empty()) {
            confirmed_ui = true;
            break;
        }
        sleep_ms(120);
    }

    if (!confirmed_ui) {
        close_detail_panel();
        return false;
    }

    tap(B + 65, 1025, 380);
    if (!expected_name.empty()) {
        // `ke.add(k), Gt = k, rr = !0` —— `ke` 即 Nr，`Gt` 记录最近一次买到的干员
        m_last_bought_name = expected_name;
        if (find_operator(expected_name)) {
            // `if (state.units.filter(x => x.name === k && !x.elite).length >= 2) p.add(k)`
            int same_cnt = 0;
            for (const auto& u : m_units) {
                if (u.name == expected_name && !u.elite) {
                    ++same_cnt;
                }
            }
            if (same_cnt >= 2) {
                m_pending_merge.insert(expected_name);
            }
            m_recent_bought_names.push_back(expected_name);
            if (m_recent_bought_names.size() > 10) {
                m_recent_bought_names.erase(m_recent_bought_names.begin());
            }
        }
        else if (find_item(expected_name)) {
            m_bought_items.insert(expected_name);
        }
    }
    return true;
}

bool SPAFarmTaskPlugin::upgrade_shop(const ShopHeaderState& header)
{
    // 1:1 port of `ct(I)` in service.pretty.mjs L9055-9088（非 pc 设备路径）。
    //
    // `ct` 点升级 → 轮询「确认升级」弹窗 → 点了之后调 `_()`；
    // 没出现弹窗时按「等级 +1 / 金币扣减」二选一，都不满足才 `return !1`。
    // `_()` 是 pc 专属的 3 秒等级复核，非 pc 首行直接 `return !0`，
    // 所以 MAA 上升级成功与否只由「确认升级弹窗 / 等级 / 金币」判定。
    if (!header.upgrade_btn.has_value()) {
        return false;
    }
    const int k = header.upgrade_btn->first;
    const int D = header.upgrade_btn->second;
    const std::optional<int> level_before = header.level;
    const std::optional<int> gold_before = header.gold;
    const int up_cost = header.upgrade_cost.value_or(-1);

    // `ct(I)` 里的 `_()`（service.pretty.mjs L9057-9063）首行是
    //     `if (e.getState().device?.kind !== "pc") return !0;`
    // MAA 走 Win32Controller，设备恒为非 pc（实测 pc 分支不可达，按用户要求不移植），
    // 因此 `_()` 恒返回 true：不做 3 秒等级轮询、不把 `i.level` 置 null、
    // 也不抛「PC 升级结果未确认」。旧实现漏掉这个守卫，把 pc 专属逻辑无条件执行，
    // 升级后只要 OCR 一帧没读到新等级就置 level=0 并抛错，导致整回合商店批处理被 break。
    auto verify_level_up = []() -> bool {
        return true;
    };

    tap(k, D, 200);

    const auto t0 = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - t0).count() < 1500
           && !need_exit()) {
        sleep_ms(80);
        const int rx = std::max(0, static_cast<int>(std::lround(static_cast<double>(k) - 130.0)));
        const int ry = std::max(0, static_cast<int>(std::lround(static_cast<double>(D) - 120.0)));
        cv::Mat cur = shot();
        if (cur.empty()) {
            continue;
        }
        auto items = ocr(
            cur,
            std::array<int, 4> { rx,
                                 ry,
                                 260,
                                 std::min(std::max(0, 1080 - ry), 200) });
        if (auto hit = find_ocr(items, { "确认升级" })) {
            tap(hit->cx, hit->cy, 0);
            sleep_ms(250);
            return verify_level_up();
        }
    }

    cv::Mat cur = shot();
    ShopHeaderState now_hdr;
    if (!cur.empty()) {
        now_hdr = read_shop_header(cur, ocr(cur));
    }
    if (level_before.has_value() && now_hdr.level.has_value()
        && *now_hdr.level == *level_before + 1) {
        return true;
    }
    if (gold_before.has_value() && up_cost > 0 && now_hdr.gold.has_value()
        && *now_hdr.gold == *gold_before - up_cost) {
        return verify_level_up();
    }
    log_info("点击升级后未出现确认升级，跳过本次升级");
    tap(1450, 300, 300);
    return false;
}

// `lr(a)` —— service.pretty.mjs L8939-8942：过渡期干员（不追盟约、不叠层、不是核心）
bool SPAFarmTaskPlugin::is_transition_unit(std::string_view name) const
{
    const auto* op = find_operator(name);
    if (op == nullptr || op->golden) {
        return false;
    }
    static const std::vector<std::string> kStackers = {
        "伺夜", "空弦", "黑键", "雪猎", "旅骨", "霍尔海雅",
        "迷迭香", "鸿雪", "Miss.Christine", "溯光星源", "逻各斯", "夕"
    };
    static const std::vector<std::string> kFinishers = { "蕾缪安", "塔露拉", "艾雅法拉", "维伊" };

    for (const auto& b : op->bond_names) {
        if (std::find(m_active_core_bonds.begin(), m_active_core_bonds.end(), b) != m_active_core_bonds.end()) {
            return false;
        }
        if (m_goal == "medal" && b == "迅捷") {
            return false; // `or`：medal 目标是 ["迅捷"]
        }
    }
    if (std::find(kStackers.begin(), kStackers.end(), op->name) != kStackers.end()) {
        return false;
    }
    if (std::find(kFinishers.begin(), kFinishers.end(), op->name) != kFinishers.end()) {
        return false;
    }
    return true;
}

// `F(I, k)` —— service.pretty.mjs L8944-8948：挑一名可卖的过渡干员（精英/高费靠后）
const SPAFarmTaskPlugin::UnitSlot* SPAFarmTaskPlugin::pick_sell_bench_unit(std::string_view except_name) const
{
    auto rank = [&](const UnitSlot& u) {
        const int elite = (u.elite || m_elite_units.count(u.name) > 0) ? 1 : 0;
        const auto* op = find_operator(u.name);
        return std::pair<int, int> { elite, op == nullptr ? 9 : op->tier };
    };
    const UnitSlot* best = nullptr;
    for (const auto& u : m_units) {
        if (u.area != "bench" || u.name.empty() || u.auxiliary || u.item) {
            continue;
        }
        if (u.name == except_name || !is_transition_unit(u.name)) {
            continue;
        }
        if (best == nullptr || rank(u) < rank(*best)) {
            best = &u;
        }
    }
    return best;
}

bool SPAFarmTaskPlugin::sell_unit_slot(const std::string& slot, int gold_baseline)
{
    // `oe(I, k)` —— service.pretty.mjs L8969-8989
    //   拖到 (1780,930) → 700ms → 收起详情 → 重读商店头 → **金币必须比基准大** 才算卖掉
    auto it = std::find_if(m_units.begin(), m_units.end(), [&](const UnitSlot& u) {
        return u.slot == slot;
    });
    if (it == m_units.end()) {
        return false;
    }
    auto st = wait_stable_board(4000);
    if (!st.has_value()) {
        return false;
    }
    auto [fx, fy] = project_point(st->H, it->c + 0.5, it->r + 0.5);
    drag_segments(fx, fy, 1780, 930, 16, 25, 400);
    sleep_ms(static_cast<int>(std::lround(700.0 * m_step_scale)));
    close_detail_panel();

    // `Rt(await Ke(await h()))`：收起面板后读商店头
    {
        cv::Mat cur = shot();
        auto items = ocr(cur);
        if (auto hit = find_text_regex(items, "^.?展开$")) {
            tap(hit->cx, hit->cy, 250);
        }
        cv::Mat hdr_img = shot();
        auto hdr = read_shop_header(hdr_img, ocr(hdr_img));
        const bool sold = hdr.gold.has_value() && *hdr.gold > gold_baseline;
        if (!sold) {
            return false;
        }
        if (hdr.gold.has_value()) {
            m_current_gold = *hdr.gold;
        }
    }

    // `removeUnit(slot)`
    {
        const std::string sold_name = it->name;
        const bool ghost = it->ghost;
        m_remembered_units.erase(slot);
        if (ghost) {
            m_units.erase(it);
        }
        else {
            it->name.clear();
            it->source.clear();
            it->verified = false;
        }
        // `ke.delete(name)`：场上没有同名（除自己）时，从「已买过」里移除
        if (!sold_name.empty()) {
            bool other_same = false;
            for (const auto& u : m_units) {
                if (u.slot != slot && u.name == sold_name) {
                    other_same = true;
                    break;
                }
            }
            if (!other_same) {
                m_bought_operators.erase(sold_name);
            }
        }
    }
    return true;
}

// ============================================================================
// Equipment Classification (`Es`/`Fs`/`ma`/`vs`/`An`/`ga`), Row `Q`, Jessica Pager, Mob Elite & Summons
// ============================================================================

std::string SPAFarmTaskPlugin::classify_item_category(std::string_view name, std::string_view effect)
{
    // Exact 1:1 port of `Es`, `Fs`, `ma`, `vs`, and `An(a)` in service.pretty.mjs lines 8391-8399:
    // - Es = /流失|无法被治疗|生命上限-|阻挡数-1|攻击速度-|更容易受到攻击(?!.*生命上限\+)/
    // - Fs = /资金|刷新|促销|最大可部署人数|指令|晋升|调度中心/
    // - ma = /层数|盟约|身份牌|推荐信|初始干员/
    // - vs = /生命上限\+|防御力\+|阻挡数\+|复活|护盾|免疫|致命伤害|隐匿/
    if (name == "信标") {
        return "debuff";
    }
    if (name == "博士投影") {
        return "output";
    }

    auto has_any = [](std::string_view text, std::initializer_list<std::string_view> kws) {
        for (std::string_view kw : kws) {
            if (!kw.empty() && text.find(kw) != std::string_view::npos) {
                return true;
            }
        }
        return false;
    };

    const bool match_ma = has_any(effect, { "层数", "盟约", "身份牌", "推荐信", "初始干员" });
    const bool match_Fs = has_any(effect, { "资金", "刷新", "促销", "最大可部署人数", "指令", "晋升", "调度中心" });
    const bool match_Es = has_any(effect, { "流失", "无法被治疗", "生命上限-", "阻挡数-1", "攻击速度-" })
        || (effect.find("更容易受到攻击") != std::string_view::npos && effect.find("生命上限+") == std::string_view::npos);
    const bool match_vs = has_any(effect, { "生命上限+", "防御力+", "阻挡数+", "复活", "护盾", "免疫", "致命伤害", "隐匿" });
    const bool match_atk_dmg = has_any(effect, { "攻击力+", "伤害" });

    if (effect.find("装备时销毁") != std::string_view::npos) {
        return match_ma ? "stack" : "utility";
    }
    // 原文顺序：`Fs`（utility）在 `Es`（debuff）之前
    if (match_Fs) {
        return "utility";
    }
    if (match_Es) {
        return "debuff";
    }
    if (match_ma) {
        return "stack";
    }
    if (match_vs && !match_atk_dmg) {
        return "defense";
    }
    return "output";
}

int SPAFarmTaskPlugin::equip_cap(const std::string& name) const
{
    // `Ct(I)` —— service.pretty.mjs L9289-9291
    //   !$e()（goal=="medal"）|| strategy!=="杰西卡" || round>13 → 2
    //   否则「是寻呼目标」→ 1，其余 2
    if (m_goal == "medal" || m_current_strategy.find("杰西卡") == std::string::npos || m_current_round > 13) {
        return 2;
    }
    auto tgt = pick_pager_target();
    return (tgt.has_value() && tgt->name == name) ? 1 : 2;
}

std::vector<SPAFarmTaskPlugin::EquipTarget> SPAFarmTaskPlugin::build_equip_targets() const
{
    // `on` 的 `B` —— L9434-9440
    std::vector<EquipTarget> out;
    for (const auto& u : m_units) {
        if (u.area != "board" || u.name.empty() || u.auxiliary) {
            continue;
        }
        int eq = 0;
        if (auto it = m_unit_equipped_count.find(u.name); it != m_unit_equipped_count.end()) {
            eq = it->second;
        }
        if (eq >= equip_cap(u.name)) {
            continue;
        }
        out.push_back(EquipTarget { u.slot, u.name, u.elite, eq, u.r, u.c });
    }
    return out;
}

const SPAFarmTaskPlugin::EquipTarget* SPAFarmTaskPlugin::pick_equipment_target(
    std::string_view category,
    const std::vector<EquipTarget>& targets,
    const EquipTarget* exclude)
{
    // Exact 1:1 port of `ga(a, s, e, t, f)` in service.pretty.mjs lines 8401-8414
    if (category == "debuff" || targets.empty()) {
        return nullptr;
    }

    auto score = [&](const EquipTarget& c) -> double {
        const auto* op = find_operator(c.name);
        if (op == nullptr) {
            return -1e9; // JS: `l ? ... : -1e9`
        }
        const double val_i = 200.0 * (1.0 + 0.25 * (op->tier - 1)) * (c.elite ? 1.4 : 1.0);
        const double val_m = -static_cast<double>(c.equipped) * 150.0;
        if (category == "stack") {
            const double keep = in_stacker_list(op->name) ? 1000.0 : 0.0;
            bool has_focus = false;
            for (const auto& b : op->bond_names) {
                if (std::find(m_active_core_bonds.begin(), m_active_core_bonds.end(), b) != m_active_core_bonds.end()) {
                    has_focus = true;
                    break;
                }
            }
            return keep + (has_focus ? 500.0 : 0.0) + val_i / 10.0 + val_m;
        }
        if (category == "defense") {
            return (op->position == "MELEE" ? 1000.0 : 0.0) + op->block_cnt * 100.0 + op->max_hp / 100.0 + val_m;
        }
        if (category == "utility") {
            return val_m;
        }
        return val_i + (op->position == "RANGED" ? 50.0 : 0.0) + val_m;
    };

    const EquipTarget* best = nullptr;
    double best_score = 0.0;
    for (const auto& c : targets) {
        if (exclude != nullptr && &c == exclude) {
            continue;
        }
        const double sc = score(c);
        if (best == nullptr || sc > best_score) { // JS `sort` 稳定，取首个最大值
            best = &c;
            best_score = sc;
        }
    }
    return best;
}

// `Pt(I, k, D, B)` —— service.pretty.mjs L9197-9206：以格心为中心取 80×60 灰度 patch
std::vector<uint8_t> SPAFarmTaskPlugin::capture_item_patch(
    const cv::Mat& img,
    const std::array<double, 9>& H,
    int r,
    int c)
{
    std::vector<uint8_t> out(80 * 60, 0);
    if (img.empty()) {
        return out;
    }
    auto [px, py] = project_point(H, c + 0.5, r + 0.5);
    const int left = std::max(0, static_cast<int>(std::lround(px - 40)));
    const int top = std::max(0, static_cast<int>(std::lround(py - 45)));
    const int w = std::min(80, img.cols - left);
    const int h = std::min(60, img.rows - top);
    if (w <= 0 || h <= 0) {
        return out;
    }
    cv::Mat gray;
    if (img.channels() == 3) {
        cv::cvtColor(img(cv::Rect(left, top, w, h)), gray, cv::COLOR_BGR2GRAY);
    }
    else {
        gray = img(cv::Rect(left, top, w, h)).clone();
    }
    for (int y = 0; y < h; ++y) {
        std::memcpy(out.data() + static_cast<size_t>(y) * 80, gray.ptr<uint8_t>(y), static_cast<size_t>(w));
    }
    return out;
}

// `zr(I, k)` —— service.pretty.mjs L9207-9209：逐像素平均绝对差
double SPAFarmTaskPlugin::patch_diff(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b)
{
    if (a.empty() || a.size() != b.size()) {
        return 1e9;
    }
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        sum += std::abs(static_cast<int>(a[i]) - static_cast<int>(b[i]));
    }
    return sum / static_cast<double>(a.size());
}

// `Vt(I, k, D)` —— service.pretty.mjs L9248-9283
//   扫棋盘里所有 `Q` 格：patch 未变（<18）跳过；未缓存时只有第 1 回合或 force 才继续；
//   名字靠 `cr` 缓存（patch 差 <10）或 `inspectCell` 现读；物品进返回值，干员进 `wr`。
std::vector<SPAFarmTaskPlugin::EquipCell> SPAFarmTaskPlugin::detect_item_slots(
    const std::vector<std::string>& rows,
    const std::array<double, 9>& H,
    bool force)
{
    std::vector<EquipCell> items;
    std::vector<std::pair<int, int>> cells;
    for (int r = 0; r < static_cast<int>(rows.size()); ++r) {
        for (int c = 0; c < static_cast<int>(rows[r].size()); ++c) {
            if (rows[r][c] == 'Q') {
                cells.emplace_back(r, c);
            }
        }
    }
    cv::Mat cap = shot();
    const int round = m_current_round > 0 ? m_current_round : 9; // `(i.round ?? 9)`

    for (const auto& [r, c] : cells) {
        if (need_exit()) {
            break;
        }
        const std::string key = std::to_string(r) + "," + std::to_string(c);
        const auto patch = capture_item_patch(cap, H, r, c);

        auto pc = m_equip_patch_cache.find(key);
        if (pc != m_equip_patch_cache.end()) {
            if (patch_diff(patch, pc->second) < 18.0) {
                continue;
            }
        }
        else {
            if (round <= 1) {
                m_equip_patch_cache[key] = patch;
                continue;
            }
            if (!force) {
                continue;
            }
        }

        std::string name;
        auto nc = m_equip_name_cache.find(key);
        if (nc != m_equip_name_cache.end() && patch_diff(patch, nc->second.patch) < 10.0) {
            name = nc->second.name;
        }
        else {
            auto got = inspect_cell_name(H, r, c);
            if (got.has_value()) {
                name = *got;
            }
        }
        if (!name.empty()) {
            m_equip_name_cache[key] = EquipNameCache { patch, name };
        }
        if (!name.empty() && find_item(name) != nullptr) {
            items.push_back(EquipCell { r, c, name });
        }
        else if (!name.empty() && find_operator(name) != nullptr) {
            m_overflow_equip_units.push_back(EquipCell { r, c, name }); // `wr.push`
        }
    }
    return items;
}

// `sn(I, k)` —— service.pretty.mjs L9300-9351：两张全屏差分找「拖拽预览框」
//   960×540 上按 4×4 块（亮度差 >30 的像素 ≥60%）打点 → 连通域 → 尺寸筛选 → 取最大
std::optional<std::array<double, 4>> SPAFarmTaskPlugin::detect_drop_popup(const cv::Mat& before, const cv::Mat& after)
{
    if (before.empty() || after.empty()) {
        return std::nullopt;
    }
    cv::Mat a, b;
    cv::resize(before, a, cv::Size(960, 540), 0.0, 0.0, cv::INTER_AREA);
    cv::resize(after, b, cv::Size(960, 540), 0.0, 0.0, cv::INTER_AREA);
    cv::Mat ga, gb;
    cv::cvtColor(a, ga, cv::COLOR_BGR2GRAY);
    cv::cvtColor(b, gb, cv::COLOR_BGR2GRAY);

    constexpr int kGw = 240;
    constexpr int kGh = 135;
    std::vector<uint8_t> mask(static_cast<size_t>(kGw) * kGh, 0);
    for (int gy = 0; gy < kGh; ++gy) {
        const int y0 = gy * 4;
        for (int gx = 0; gx < kGw; ++gx) {
            const int x0 = gx * 4;
            if (x0 < 360 || y0 < 50 || y0 > 460 || (x0 > 800 && y0 > 330)) {
                continue;
            }
            int cnt = 0;
            for (int dy = 0; dy < 4 && cnt < 10; ++dy) {
                const uint8_t* pa = ga.ptr<uint8_t>(y0 + dy) + x0;
                const uint8_t* pb = gb.ptr<uint8_t>(y0 + dy) + x0;
                for (int dx = 0; dx < 4; ++dx) {
                    if (static_cast<int>(pa[dx]) - static_cast<int>(pb[dx]) > 30) {
                        ++cnt;
                    }
                }
            }
            if (cnt >= 16 * 0.6) {
                mask[static_cast<size_t>(gy) * kGw + gx] = 1;
            }
        }
    }

    std::vector<uint8_t> seen(mask.size(), 0);
    struct Box
    {
        int x0 = 1 << 30;
        int y0 = 1 << 30;
        int x1 = -1;
        int y1 = -1;
        int n = 0;
    };
    std::optional<Box> best;
    std::vector<int> stack;
    for (int idx = 0; idx < static_cast<int>(mask.size()); ++idx) {
        if (mask[static_cast<size_t>(idx)] == 0 || seen[static_cast<size_t>(idx)] != 0) {
            continue;
        }
        stack.clear();
        stack.push_back(idx);
        seen[static_cast<size_t>(idx)] = 1;
        Box box;
        while (!stack.empty()) {
            const int cur = stack.back();
            stack.pop_back();
            const int cx = cur % kGw;
            const int cy = cur / kGw;
            ++box.n;
            box.x0 = std::min(box.x0, cx);
            box.x1 = std::max(box.x1, cx);
            box.y0 = std::min(box.y0, cy);
            box.y1 = std::max(box.y1, cy);
            const std::array<std::pair<int, int>, 4> nb = {
                std::pair<int, int> { cx + 1, cy },
                std::pair<int, int> { cx - 1, cy },
                std::pair<int, int> { cx, cy + 1 },
                std::pair<int, int> { cx, cy - 1 }
            };
            for (const auto& [nx, ny] : nb) {
                if (nx < 0 || ny < 0 || nx >= kGw || ny >= kGh) {
                    continue;
                }
                const size_t ni = static_cast<size_t>(ny) * kGw + nx;
                if (mask[ni] != 0 && seen[ni] == 0) {
                    seen[ni] = 1;
                    stack.push_back(static_cast<int>(ni));
                }
            }
        }
        const int wpx = (box.x1 - box.x0 + 1) * 8;
        const int hpx = (box.y1 - box.y0 + 1) * 8;
        if (wpx >= 120 && wpx <= 520 && hpx >= 40 && hpx <= 220 && (!best.has_value() || box.n > best->n)) {
            best = box;
        }
    }
    if (!best.has_value()) {
        return std::nullopt;
    }
    const int wpx = (best->x1 - best->x0 + 1) * 8;
    const int hpx = (best->y1 - best->y0 + 1) * 8;
    if (wpx < 120 || wpx > 520 || hpx < 40 || hpx > 220) {
        return std::nullopt;
    }
    return std::array<double, 4> {
        (best->x0 + best->x1 + 1) / 2.0 * 8.0,
        (best->y0 + best->y1 + 1) / 2.0 * 8.0,
        static_cast<double>(wpx),
        static_cast<double>(hpx)
    };
}

// `ot(j, J, K, ce, {steps, stepMs, hold, press, beforeUp})` —— service.pretty.mjs L7319-7345
void SPAFarmTaskPlugin::drag_with_hook(
    double x1,
    double y1,
    double x2,
    double y2,
    int steps,
    int step_ms,
    int hold_ms,
    int press_ms,
    const std::function<std::optional<DragHookResult>(const std::pair<double, double>&)>& before_up)
{
    steps = std::max(1, steps);
    touch_event(static_cast<int>(InputEvent::Type::TOUCH_DOWN), x1, y1);
    sleep_ms(static_cast<int>(std::lround(press_ms * m_step_scale)));
    for (int i = 1; i <= steps; ++i) {
        const double r = static_cast<double>(i) / static_cast<double>(steps);
        touch_event(
            static_cast<int>(InputEvent::Type::TOUCH_MOVE),
            x1 + (x2 - x1) * r,
            y1 + (y2 - y1) * r);
        sleep_ms(static_cast<int>(std::lround(step_ms * m_step_scale)));
    }
    sleep_ms(static_cast<int>(std::lround(hold_ms * m_step_scale)));

    std::pair<double, double> pos = { x2, y2 };
    for (int b = 0; before_up && b < 8; ++b) {
        auto ret = before_up(pos);
        if (!ret.has_value()) {
            break;
        }
        const auto to = ret->to;
        for (int i = 1; i <= 8; ++i) {
            const double r = static_cast<double>(i) / 8.0;
            touch_event(
                static_cast<int>(InputEvent::Type::TOUCH_MOVE),
                pos.first + (to.first - pos.first) * r,
                pos.second + (to.second - pos.second) * r);
            sleep_ms(static_cast<int>(std::lround(step_ms * m_step_scale)));
        }
        pos = to;
        sleep_ms(static_cast<int>(std::lround(hold_ms * m_step_scale)));
        if (ret->final) {
            break;
        }
    }
    touch_event(static_cast<int>(InputEvent::Type::TOUCH_UP), pos.first, pos.second);
}

// `Cr(I, k, D, B, _)` —— service.pretty.mjs L9352-9408
//   拖拽全程核对：先核「拿起的是不是这件」，再核「预览框在不在目标头顶」；
//   偏了就按 delta 修正，连续失败换 4 个候选落点，最后放回原格（`Ar` 置位供 miss-rescan）
bool SPAFarmTaskPlugin::equip_drag_verified(
    const std::string& tag,
    const std::array<double, 9>& H,
    const EquipCell& src,
    const EquipTarget& tgt,
    const std::string& expected_name)
{
    constexpr double kPopupOffset = 145.0; // `qr`
    auto [sx, sy] = project_point(H, src.c + 0.5, src.r + 0.5);
    auto [tx, ty] = project_point(H, tgt.c + 0.5, tgt.r + 0.5);
    const std::array<double, 2> expect = { tx, ty - kPopupOffset };
    std::array<std::array<double, 2>, 4> fallback;
    const std::array<double, 4> offsets = { 0.85, 0.65, 1.05, 0.4 };
    for (size_t i = 0; i < offsets.size(); ++i) {
        auto [px, py] = project_point(H, tgt.c + 0.5, tgt.r + offsets[i]);
        fallback[i] = { px, py };
    }
    auto offscreen = [](const std::array<double, 2>& p) -> bool {
        return (p[0] > 1630 && p[1] > 760) || p[1] > 1040 || p[1] < 120 || p[0] < 20 || p[0] > 1900;
    };

    cv::Mat base = shot(); // `Q = await e.capture()`
    int tries = 0;         // `H`
    bool ok = false;       // `ve`
    bool name_checked = false; // `Pe`（`_` 恒非空，所以首轮必查）

    m_equip_drag_missed = false; // `Ar = !1`

    auto hook = [&](const std::pair<double, double>& finger) -> std::optional<DragHookResult> {
        cv::Mat cur = shot();
        if (!name_checked) {
            auto ocr_items = ocr(cur, std::array<int, 4> { 20, 270, 400, 60 });
            std::string grabbed;
            for (const auto& it : ocr_items) {
                grabbed += strip_ws(it.text);
            }
            const std::string head = expected_name.substr(0, std::min<size_t>(3, expected_name.size()));
            if (grabbed.find(head) == std::string::npos) {
                m_equip_drag_missed = true;
                log_info("[drop] " + tag + " 拿起的是「" + grabbed + "」不是 " + expected_name + "，放回原处");
                return DragHookResult { { sx, sy }, true };
            }
            name_checked = true;
        }
        auto popup = detect_drop_popup(base, cur);
        const bool at_target = popup.has_value() && std::abs((*popup)[0] - expect[0]) < 60
            && std::abs((*popup)[1] - expect[1]) < 55;
        std::string popup_desc = popup.has_value()
            ? (std::to_string(static_cast<int>(std::lround((*popup)[0]))) + ","
               + std::to_string(static_cast<int>(std::lround((*popup)[1]))) + " "
               + std::to_string(static_cast<int>(std::lround((*popup)[2]))) + "x"
               + std::to_string(static_cast<int>(std::lround((*popup)[3]))))
            : "-";
        log_info(
            "[drop] " + tag + " t" + std::to_string(tries) + " finger("
            + std::to_string(static_cast<int>(std::lround(finger.first))) + ","
            + std::to_string(static_cast<int>(std::lround(finger.second))) + ") popup(" + popup_desc + ") expect("
            + std::to_string(static_cast<int>(std::lround(expect[0]))) + ","
            + std::to_string(static_cast<int>(std::lround(expect[1]))) + ") " + (at_target ? "OK" : "miss"));
        if (at_target) {
            ok = true;
            return std::nullopt;
        }
        ++tries;
        std::optional<std::array<double, 2>> next;
        if (tries < static_cast<int>(fallback.size())) {
            if (popup.has_value() && tries == 1) {
                next = std::array<double, 2> {
                    finger.first + (expect[0] - (*popup)[0]),
                    finger.second + (expect[1] - (*popup)[1])
                };
            }
            else {
                next = fallback[tries];
            }
        }
        if (next.has_value() && offscreen(*next)) {
            if (tries < static_cast<int>(fallback.size())) {
                next = fallback[tries];
            }
            else {
                next.reset();
            }
        }
        if (next.has_value() && !offscreen(*next)) {
            return DragHookResult { { (*next)[0], (*next)[1] }, false };
        }
        return DragHookResult { { sx, sy }, true };
    };

    drag_with_hook(sx, sy, fallback[0][0], fallback[0][1], 16, 25, 400, 300, hook);
    if (!ok) {
        log_info(
            "拖放核对：预览框不在 " + std::to_string(tgt.r) + "," + std::to_string(tgt.c) + " 头顶，已放回原格（不装给别人）");
    }
    return ok;
}

// `Dt(I, k)` —— service.pretty.mjs L9124-9201：把整备区的召唤物拖上场
//   `D` = 整备区里 auxiliary、非装备、有名字、且不在 chess.json 里的单位
//   落点：地面路线上的空 `b` 格（排除已占格 + `front1`/`back1` 干员的前/后一格），按 groundProgress 升序
void SPAFarmTaskPlugin::deploy_summons(const StageTrafficMap& traffic)
{
    std::vector<UnitSlot> summons;
    for (const auto& u : m_units) {
        if (u.area != "bench" || !u.auxiliary || u.item || u.name.empty()) {
            continue;
        }
        if (find_operator(u.name) != nullptr) {
            continue; // `!e.chess.some(v => v.name === x.name)`
        }
        summons.push_back(u);
    }
    if (summons.empty()) {
        return;
    }

    std::set<std::string> occupied;
    for (const auto& u : m_units) {
        if (u.area == "board") {
            occupied.insert(std::to_string(u.r) + "," + std::to_string(u.c));
        }
    }
    for (int uid = 0; uid < static_cast<int>(m_units.size()); ++uid) {
        (void)uid;
    }
    // `front1` / `back1` 干员的前后一格也视为占用
    {
        auto delta_of = [](const std::string& facing) -> std::pair<int, int> {
            if (facing == "LEFT") {
                return { 0, -1 };
            }
            if (facing == "UP") {
                return { -1, 0 };
            }
            if (facing == "DOWN") {
                return { 1, 0 };
            }
            return { 0, 1 };
        };
        for (const auto& u : m_units) {
            if (u.area != "board" || u.name.empty() || u.auxiliary) {
                continue;
            }
            const auto* op = find_operator(u.name);
            if (op == nullptr || op->golden) {
                continue;
            }
            const auto [dr, dc] = delta_of(u.facing.empty() ? "RIGHT" : u.facing);
            for (const auto& tr : op->traits) {
                const bool front = std::find(tr.tags.begin(), tr.tags.end(), "front1") != tr.tags.end();
                const bool back = std::find(tr.tags.begin(), tr.tags.end(), "back1") != tr.tags.end();
                if (front) {
                    occupied.insert(std::to_string(u.r + dr) + "," + std::to_string(u.c + dc));
                }
                if (back) {
                    occupied.insert(std::to_string(u.r - dr) + "," + std::to_string(u.c - dc));
                }
            }
        }
    }

    struct Cell
    {
        int r = 0;
        int c = 0;
        double prog = 0.0;
    };
    std::vector<Cell> cells;
    for (int r = 0; r < traffic.rows && r < static_cast<int>(traffic.grid.size()); ++r) {
        const std::string& line = traffic.grid[r];
        for (int c = 0; c < static_cast<int>(line.size()) && c < traffic.cols; ++c) {
            if (line[c] != 'b') {
                continue;
            }
            if (traffic.ground[r][c] <= 0.0) {
                continue;
            }
            if (occupied.count(std::to_string(r) + "," + std::to_string(c)) > 0) {
                continue;
            }
            cells.push_back(Cell { r, c, traffic.ground_progress[r][c] });
        }
    }
    std::sort(cells.begin(), cells.end(), [](const Cell& a, const Cell& b) {
        return a.prog < b.prog;
    });

    auto st = wait_stable_board(4000);
    if (!st.has_value()) {
        return;
    }

    std::vector<std::string> deployed;
    size_t next_cell = 0;
    for (const auto& s : summons) {
        if (need_exit() || next_cell >= cells.size()) {
            break;
        }
        const Cell target = cells[next_cell++];

        // 朝向：落点四邻中地面流量最小的那个方向（`Dt` 里的 Q 计算）
        std::string facing = "RIGHT";
        {
            static const std::pair<const char*, std::pair<int, int>> kDirs[] = {
                { "RIGHT", { 0, 1 } }, { "LEFT", { 0, -1 } }, { "UP", { -1, 0 } }, { "DOWN", { 1, 0 } }
            };
            double best = std::numeric_limits<double>::max();
            for (const auto& [name, off] : kDirs) {
                const int rr = target.r + off.first;
                const int cc = target.c + off.second;
                if (rr < 0 || cc < 0 || rr >= traffic.rows || cc >= traffic.cols) {
                    continue;
                }
                if (traffic.ground[rr][cc] <= 0.0) {
                    continue;
                }
                const double prog = traffic.ground_progress[rr][cc];
                if (prog < best) {
                    best = prog;
                    facing = name;
                }
            }
        }

        auto [sx, sy] = project_point(st->H, s.c + 0.5, 6.5);
        auto [tx, ty] = project_point(st->H, target.c + 0.5, target.r + 0.5);
        drag_board_unit(sx, sy, tx, ty, false);
        sleep_ms(250);

        // 摆正朝向（同 `we` 的 orient）
        {
            auto after = wait_stable_board(3000);
            if (after.has_value()) {
                auto [cx, cy] = project_point(after->H, target.c + 0.5, target.r + 0.5);
                double dx = 210.0;
                double dy = 0.0;
                if (facing == "LEFT") {
                    dx = -210.0;
                }
                else if (facing == "UP") {
                    dx = 0.0;
                    dy = -180.0;
                }
                else if (facing == "DOWN") {
                    dx = 0.0;
                    dy = 180.0;
                }
                drag_segments(cx, cy, cx + dx, cy + dy, 4, 12, 40);
                sleep_ms(200);
                close_detail_panel();
            }
        }

        // 状态更新：整备区该格变为场上
        for (auto& u : m_units) {
            if (u.slot == s.slot) {
                const std::string old_slot = u.slot;
                u.slot = "board:" + std::to_string(target.r) + "," + std::to_string(target.c);
                u.area = "board";
                u.r = target.r;
                u.c = target.c;
                u.facing = facing;
                m_remembered_units[u.slot] = u;
                m_remembered_units.erase(old_slot);
                break;
            }
        }
        deployed.push_back(s.name + "→" + std::to_string(target.r) + "," + std::to_string(target.c));
    }
    if (!deployed.empty()) {
        std::string joined;
        for (const auto& d : deployed) {
            joined += (joined.empty() ? "" : "、") + d;
        }
        log_info("召唤物上场：" + joined);
    }
}

void SPAFarmTaskPlugin::handle_overflow_units(const CameraDetectResult& cam)
{
    // Exact 1:1 port of `nn(I)` in service.pretty.mjs lines 9212-9245
    //   `Vt(I, k.H, !0)` 扫临时格（装备行为干员的格子）→ 卖一个整备区腾位 → 把该干员拖进整备区
    auto st = wait_stable_board(4000);
    if (!st.has_value()) {
        return;
    }
    const StageInfo* stage = find_stage(m_stage_id);
    static const std::vector<std::string> kNoRows;
    const std::vector<std::string>& rows = (stage != nullptr) ? stage->rows : kNoRows;

    m_overflow_equip_units.clear(); // `wr = []`
    detect_item_slots(rows, st->H, true);

    // `_`：名字在 `chess.json` 里，且（首个盟约属于目标盟约 或 名字在叠层手名单里）
    auto is_keeper = [&](const std::string& name) -> bool {
        const auto* op = find_operator(name);
        if (op == nullptr) {
            return false;
        }
        for (const auto& b : op->bond_names) {
            if (std::find(m_active_core_bonds.begin(), m_active_core_bonds.end(), b) != m_active_core_bonds.end()) {
                return true;
            }
        }
        return in_stacker_list(name);
    };
    auto tier_of = [&](const std::string& name) -> int {
        const auto* op = find_operator(name);
        return op != nullptr ? op->tier : 9;
    };
    // `Rt(await Ke(await h()))`：展开商店面板后读头部金币
    auto read_gold = [&]() -> int {
        cv::Mat cur = shot();
        auto items = ocr(cur);
        if (auto hit = find_text_regex(items, "^.?展开$")) {
            tap(hit->cx, hit->cy, 250);
            cur = shot();
        }
        auto hdr = read_shop_header(cur, ocr(cur));
        return hdr.gold.value_or(0);
    };

    for (const auto& cell : m_overflow_equip_units) {
        if (need_exit()) {
            return;
        }
        // `ee`：整备区里可卖的（不是盟约/叠层主力，或是临时补位），按 tier 升序取第一个
        const UnitSlot* sell = nullptr;
        for (const auto& u : m_units) {
            if (u.area != "bench" || u.name.empty() || u.item) {
                continue;
            }
            if (is_keeper(u.name) && m_filler_units.count(u.name) == 0) {
                continue;
            }
            if (sell == nullptr || tier_of(u.name) < tier_of(sell->name)) {
                sell = &u;
            }
        }
        if (sell == nullptr) {
            log_info(cell.name + " 在临时格，整备区没有可卖的人，回合结束会被清空");
            continue;
        }
        const std::string sell_slot = sell->slot;
        const std::string sell_name = sell->name;
        const int sell_r = sell->r;
        const int sell_c = sell->c;

        const int gold = read_gold();
        if (!sell_unit_slot(sell_slot, gold)) {
            log_info("卖 " + sell_name + " 腾位失败，" + cell.name + " 留在临时格");
            continue;
        }
        auto st2 = wait_stable_board(4000);
        if (!st2.has_value()) {
            break;
        }
        auto [sx, sy] = project_point(st2->H, cell.c + 0.5, cell.r + 0.5);
        auto [tx, ty] = project_point(st2->H, sell_c + 0.5, sell_r + 0.5);
        drag_with_hook(sx, sy, tx, ty, 12, 20, 200, 150, nullptr);
        sleep_ms(600);
        m_filler_units.erase(sell_name); // `It.delete(ee.name)`
        log_info("整备区已满：卖掉 " + sell_name + "，把临时格里的 " + cell.name + " 移进整备区");
    }
    if (!m_overflow_equip_units.empty()) {
        // `e.snapshot({ force: !0 })`：重新标定当前帧后再强制重读
        cv::Mat img = shot();
        auto fresh = detect_camera(img, goal_only_allowed());
        refresh_board_snapshot(img, fresh.camera.empty() ? cam : fresh, true);
    }
}

// `br(I)` —— service.pretty.mjs L9409-9418
//   「装备已满，请选择 / 销毁并替换」弹窗：点 (1846,284) 关闭（**不销毁**），并把该干员装备数记为 2
bool SPAFarmTaskPlugin::close_equip_full_dialog(const std::string& unit_name)
{
    for (int k = 0; k < 2; ++k) {
        cv::Mat cur = shot();
        auto items = ocr(cur);
        bool full = false;
        for (const auto& it : items) {
            if (it.score < 0.55) {
                continue;
            }
            if (it.text.find("装备已满") != std::string::npos || it.text.find("销毁并替换") != std::string::npos) {
                full = true;
                break;
            }
        }
        if (full) {
            tap(1846, 284, 800);
            m_unit_equipped_count[unit_name] = 2;
            log_info(unit_name + " 装备槽已满（关闭替换框，不销毁）");
            return true;
        }
        if (k == 0) {
            sleep_ms(500);
        }
    }
    return false;
}

void SPAFarmTaskPlugin::scan_and_distribute_equipment(const cv::Mat& img, const CameraDetectResult& cam)
{
    // Exact 1:1 port of `on(I, k)` in service.pretty.mjs lines 9419-9487
    (void)img;
    (void)cam;

    std::vector<EquipCell> items; // `D`
    for (const auto& u : m_units) {
        if (u.item && !u.name.empty()) {
            items.push_back(EquipCell { u.r, u.c, u.name });
        }
    }

    const StageInfo* stage = find_stage(m_stage_id);
    static const std::vector<std::string> kNoRows;
    const std::vector<std::string>& rows = (stage != nullptr) ? stage->rows : kNoRows;

    if (auto board3 = wait_stable_board(3000)) { // `if (k)`
        auto found = detect_item_slots(rows, board3->H, m_equip_patch_cache.empty());
        for (const auto& e : found) {
            if (e.name.find("寻呼") != std::string::npos) {
                continue;
            }
            items.push_back(e);
        }
    }
    if (items.empty()) {
        return;
    }

    auto targets = build_equip_targets(); // `B`
    auto board = wait_stable_board(4000); // `_`
    if (!board.has_value()) {
        log_info("装备：棋盘未稳定，本回合跳过");
        return;
    }

    std::vector<std::string> equipped; // `Y`
    for (const auto& it : items) {
        if (need_exit()) {
            return;
        }
        const auto* item_info = find_item(it.name);
        if (item_info == nullptr || it.name.find("寻呼") != std::string::npos) {
            continue;
        }
        const std::string cat = classify_item_category(item_info->name, item_info->effect); // `An(te)`

        if (cat == "debuff") {
            auto [sx, sy] = project_point(board->H, it.c + 0.5, it.r + 0.5);
            drag_with_hook(sx, sy, 1785, 900, 16, 25, 400, 300, nullptr);
            sleep_ms(700);
            log_info("装备 " + it.name + "（减益）不使用，已销毁");
            continue;
        }

        EquipCell src = it; // JS 直接改 `L.r / L.c`
        auto attempt = [&](const EquipTarget& t) -> std::string {
            auto st3 = wait_stable_board(3000); // `ve`
            const std::array<double, 9> H3 = st3.has_value() ? st3->H : board->H;
            const std::string tag = "equip-r" + std::to_string(m_current_round) + "-" + src.name + "-" + t.name;
            bool ok = equip_drag_verified(tag, H3, src, t, src.name);
            if (!ok && m_equip_drag_missed) { // `!Ae && Ar && k` 的 miss-rescan
                auto st2 = wait_stable_board(3000);
                const std::array<double, 9> H2 = st2.has_value() ? st2->H : H3;
                auto again = detect_item_slots(rows, H2, true);
                for (const auto& e : again) {
                    if (e.name != src.name) {
                        continue;
                    }
                    src.r = e.r;
                    src.c = e.c;
                    ok = equip_drag_verified(tag + "-rescan", H2, src, t, src.name);
                    break;
                }
            }
            if (!ok) {
                return "miss";
            }
            sleep_ms(700);
            if (close_equip_full_dialog(t.name)) { // `br`
                return "full";
            }
            return "ok";
        };

        const EquipTarget* tgt = pick_equipment_target(cat, targets, nullptr);
        if (tgt == nullptr) {
            log_info("装备 " + it.name + "（无合适对象）不使用");
            continue;
        }
        const size_t tgt_idx = static_cast<size_t>(tgt - targets.data());
        const std::string first_name = targets[tgt_idx].name;
        std::string used_name = first_name;
        const std::string r1 = attempt(targets[tgt_idx]);
        bool success = (r1 == "ok");
        if (r1 == "full") {
            targets.erase(targets.begin() + static_cast<long>(tgt_idx)); // `B.splice`
        }
        if (!success) {
            const EquipTarget* alt = (r1 == "full")
                ? pick_equipment_target(cat, targets, nullptr)
                : pick_equipment_target(cat, targets, &targets[tgt_idx]);
            if (alt != nullptr) {
                const size_t alt_idx = static_cast<size_t>(alt - targets.data());
                const std::string alt_name = targets[alt_idx].name;
                const std::string r2 = attempt(targets[alt_idx]);
                if (r2 == "ok") {
                    log_info("装备 " + it.name + " 装不上 " + first_name + "，改装给 " + alt_name);
                    success = true;
                    used_name = alt_name;
                }
                else if (r2 == "full") {
                    targets.erase(targets.begin() + static_cast<long>(alt_idx));
                }
            }
        }
        if (!success) {
            log_info("装备 " + it.name + " 拖给 " + first_name + " 未生效");
            continue;
        }
        ++m_unit_equipped_count[used_name]; // `v.equipped++` + `Se.set(v.name, ... + 1)`
        equipped.push_back(it.name + "→" + used_name);
    }

    if (equipped.empty()) {
        return;
    }
    std::string msg;
    for (size_t i = 0; i < equipped.size(); ++i) {
        if (i > 0) {
            msg += "、";
        }
        msg += equipped[i];
    }
    const int left = static_cast<int>(items.size()) - static_cast<int>(equipped.size());
    if (left > 0) {
        msg += "（" + std::to_string(left) + " 件未装，留在装备栏）";
    }
    log_info("装备：" + msg);
}

// `fr()` —— service.pretty.mjs L9490-9504
//   在「场上、非召唤物、装备数 < 2、首个盟约属于目标盟约」的干员里，
//   按 (盟约还差几人, 副盟约优先级, 场上优先) 排序取第一个。
std::optional<SPAFarmTaskPlugin::UnitSlot> SPAFarmTaskPlugin::pick_pager_target() const
{
    std::set<std::string> owned;
    for (const auto& u : m_units) {
        if (!u.name.empty()) {
            owned.insert(u.name);
        }
    }
    for (const auto& n : m_bought_operators) {
        owned.insert(n);
    }
    auto bond_gap = [&](const std::string& bond_name) {
        int th = 3;
        for (const auto& b : m_bonds) {
            if (b.name == bond_name && !b.thresholds.empty()) {
                th = b.thresholds.front();
                break;
            }
        }
        std::set<std::string> have;
        for (const auto& op : m_operators) {
            if (!owned.count(op.name) || op.golden) {
                continue;
            }
            if (std::find(op.bond_names.begin(), op.bond_names.end(), bond_name) != op.bond_names.end()) {
                have.insert(op.name);
            }
        }
        return th - static_cast<int>(have.size());
    };
    auto sub_rank = [](const OperatorInfo& op) {
        if (op.bond_names.size() == 1) {
            return -1;
        }
        static const std::vector<std::string> kPref = { "远见", "精准", "奥术" };
        int best = 9;
        for (size_t k = 1; k < op.bond_names.size(); ++k) {
            auto it = std::find(kPref.begin(), kPref.end(), op.bond_names[k]);
            best = std::min(best, it == kPref.end() ? 9 : static_cast<int>(it - kPref.begin()));
        }
        return best;
    };

    const UnitSlot* best = nullptr;
    int best_gap = 0;
    int best_sub = 0;
    for (const auto& u : m_units) {
        if (u.area != "board" || u.name.empty() || u.auxiliary || u.item) {
            continue;
        }
        auto eq = m_unit_equipped_count.find(u.name);
        if (eq != m_unit_equipped_count.end() && eq->second >= 2) {
            continue;
        }
        const auto* op = find_operator(u.name);
        if (op == nullptr || op->golden || op->bond_names.empty()) {
            continue;
        }
        if (std::find(m_active_core_bonds.begin(), m_active_core_bonds.end(), op->bond_names.front())
            == m_active_core_bonds.end()) {
            continue;
        }
        const int gap = std::max(0, bond_gap(op->bond_names.front()));
        const int sr = sub_rank(*op);
        if (best == nullptr || gap < best_gap || (gap == best_gap && sr < best_sub)) {
            best = &u;
            best_gap = gap;
            best_sub = sr;
        }
    }
    if (best == nullptr) {
        return std::nullopt;
    }
    return *best;
}

// `e.inspectCell(H, r, c)` = 引擎里的 `fr(j, J, K)`（L7741-7751）：
//   点格子 → `xt(screen==="detail", 1500)` 轮询 → 读面板名 → 关面板
std::optional<std::string> SPAFarmTaskPlugin::inspect_cell_name(const std::array<double, 9>& H, int r, int c)
{
    auto [px, py] = project_point(H, c + 0.5, r + 0.5);
    tap(px, py, 0);
    std::optional<cv::Mat> detail;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1500);
    while (std::chrono::steady_clock::now() < deadline) {
        if (need_exit()) {
            break;
        }
        cv::Mat img = shot();
        if (is_detail_panel_open(img)) {
            detail = img;
            break;
        }
        sleep_ms(100);
    }
    std::optional<std::string> out;
    if (detail.has_value()) {
        auto [nm, facing] = read_unit_detail_panel(*detail);
        (void)facing;
        if (!nm.empty()) {
            out = nm;
        }
    }
    close_detail_panel();
    return out;
}

// `Ur(I)` —— service.pretty.mjs L9501-9510：包一层「收起 → 正文 → 还原来收起」
bool SPAFarmTaskPlugin::try_use_jessica_pager(const CameraDetectResult& cam)
{
    bool collapse_open = has_text_regex(ocr(shot()), "^收起");
    bool ok = jessica_pager_body(cam);
    if (collapse_open) {
        auto items = ocr(shot());
        if (!has_text_regex(items, "^收起")) {
            if (auto hit = find_text_regex(items, "^收起")) {
                tap(hit->cx, hit->cy, 400);
            }
        }
    }
    return ok;
}

// `Dr(I)` —— service.pretty.mjs L9512-9624
bool SPAFarmTaskPlugin::jessica_pager_body(const CameraDetectResult& cam)
{
    if (m_goal == "medal") {
        return false; // `$e()` 为假
    }
    if (m_current_strategy.find("杰西卡") == std::string::npos) {
        return false;
    }
    static const int kRounds[] = { 4, 7, 10, 13 };
    for (int r : kRounds) {
        if (m_current_round == r && m_pager_round != m_current_round) {
            ++m_pager_uses;
            m_pager_round = m_current_round;
        }
    }
    if (m_pager_uses <= 0 && !m_pager_started && m_current_round >= 4) {
        m_pager_uses = 1;
    }
    m_pager_started = true;
    if (m_pager_uses <= 0) {
        return false;
    }

    auto target = pick_pager_target();
    if (!target.has_value()) {
        bool has_unknown = false;
        for (const auto& u : m_units) {
            if (u.name.empty()) {
                has_unknown = true;
                break;
            }
        }
        if (has_unknown) {
            auto reread_st = wait_stable_board(3000);
            if (reread_st.has_value()) {
                CameraDetectResult rc { reread_st->camera, reread_st->raw_H, reread_st->fit };
                refresh_board_snapshot(reread_st->img, rc, false);
                inspect_unknown_units(rc, false, false);
            }
            target = pick_pager_target();
        }
    }
    if (!target.has_value()) {
        log_info("寻呼模块 ×" + std::to_string(m_pager_uses) + " 保留：场上没有第一盟约是主盟约、且装备槽未满的干员");
        return false;
    }

    {
        auto items = ocr(shot());
        if (auto hit = find_text_regex(items, "^收起")) {
            tap(hit->cx, hit->cy, 400);
        }
    }
    auto st = wait_stable_board(4000);
    if (!st.has_value()) {
        log_info("寻呼模块：棋盘未稳定，本回合不使用");
        return false;
    }

    const StageInfo* stage = find_stage(m_stage_id);
    static const std::vector<std::string> kNoRows;
    const std::vector<std::string>& rows = (stage != nullptr) ? stage->rows : kNoRows;

    // `Vt(I, H, !0)` 后按 /寻呼/ 过滤
    auto pager_cells = [&](const std::array<double, 9>& H) {
        std::vector<EquipCell> out;
        for (const auto& e : detect_item_slots(rows, H, true)) {
            if (e.name.find("寻呼") != std::string::npos) {
                out.push_back(e);
            }
        }
        return out;
    };

    auto pagers = pager_cells(st->H); // `_`
    if (pagers.empty()) {
        log_info("寻呼模块：装备栏里没找到，跳过");
        m_pager_uses = 0;
        return false;
    }
    m_pager_uses = static_cast<int>(pagers.size());

    int retries = 0; // `Y`
    for (size_t pi = 0; pi < pagers.size(); ++pi) {
        const EquipCell pager = pagers[pi]; // `ee`
        auto tgt = pick_pager_target();     // `L`
        for (int ve = 0; tgt.has_value() && ve < 3; ++ve) {
            auto vst = wait_stable_board(4000);
            if (!vst.has_value()) {
                break;
            }
            auto actual = inspect_cell_name(vst->H, tgt->r, tgt->c);
            close_detail_panel();
            if (!actual.has_value() || *actual == tgt->name) {
                break;
            }
            log_info("寻呼目标核对：" + std::to_string(tgt->r) + "," + std::to_string(tgt->c) + " 实际是 " + *actual
                + "（不是 " + tgt->name + "），改正后重新挑选");
            for (auto& u : m_units) {
                if (u.slot == tgt->slot) {
                    u.name = *actual;
                    u.confirmed = true;
                    break;
                }
            }
            tgt = pick_pager_target();
        }
        if (!tgt.has_value()) {
            log_info("寻呼模块 ×" + std::to_string(m_pager_uses) + " 保留：主盟约干员的装备槽都满了");
            break;
        }
        if (!wait_stable_board(4000).has_value()) {
            break;
        }

        // `x`：拖之前装备栏里还剩几张寻呼
        auto st_before = wait_stable_board(3000);
        const int before_cnt = static_cast<int>(pager_cells(st_before.has_value() ? st_before->H : st->H).size());

        // `v()`：拖完后确认「寻呼被消耗」或「免费卡可选」
        auto still_pending = [&]() -> bool {
            {
                auto items0 = ocr(shot());
                if (auto hit = find_text_regex(items0, "^.?展开$")) {
                    tap(hit->cx, hit->cy, 250);
                }
            }
            sleep_ms(400);
            {
                cv::Mat snap_img = shot();
                auto fresh = detect_camera(snap_img, goal_only_allowed());
                refresh_board_snapshot(snap_img, fresh.camera.empty() ? cam : fresh, true);
            }
            {
                auto items0 = ocr(shot());
                if (auto hit = find_text_regex(items0, "^.?展开$")) {
                    tap(hit->cx, hit->cy, 250);
                }
            }
            cv::Mat snap = shot();
            if (!snap.empty()) {
                auto shop_cards = read_shop_cards(snap);
                int free_cards = 0;
                for (const auto& c : shop_cards) {
                    if (c.free_card) {
                        ++free_cards;
                    }
                }
                if (free_cards >= 2) {
                    return false;
                }
            }
            auto items1 = ocr(shot());
            if (auto hit = find_text_regex(items1, "^收起")) {
                tap(hit->cx, hit->cy, 400);
            }
            auto pb = wait_stable_board(3000);
            if (!pb.has_value()) {
                return true;
            }
            return static_cast<int>(pager_cells(pb->H).size()) >= before_cnt;
        };

        bool consumed = false;   // `ie`
        bool equip_full = false; // `Q`
        {
            auto items = ocr(shot());
            if (auto hit = find_text_regex(items, "^收起")) {
                tap(hit->cx, hit->cy, 400);
            }
            auto pe_st = wait_stable_board(4000);
            if (!pe_st.has_value()) {
                break;
            }
            const std::string tag = "pager-r" + std::to_string(m_current_round) + "-" + tgt->name;
            if (!equip_drag_verified(
                    tag,
                    pe_st->H,
                    pager,
                    EquipTarget { tgt->slot, tgt->name, tgt->elite, 0, tgt->r, tgt->c },
                    "寻呼模块")) {
                break;
            }
            sleep_ms(900);

            auto after_items = ocr(shot());
            if (has_text_regex(after_items, "装备已满，请选择|销毁并替换")) {
                // 从弹窗里读「实际落在谁身上」
                std::string landed;
                for (const auto& it : after_items) {
                    if (it.x >= 1230 && it.x <= 1700 && it.y >= 250 && it.y <= 310
                        && it.text.find("装备") == std::string::npos && it.text.find("销毁") == std::string::npos
                        && it.text.find("替换") == std::string::npos) {
                        landed = strip_ws(it.text);
                        break;
                    }
                }
                tap(1846, 284, 800);
                const bool known = !landed.empty() && find_operator(landed) != nullptr;
                const std::string who = known ? landed : tgt->name;
                if (who != tgt->name) {
                    log_info("寻呼模块拖向 " + tgt->name + "，实际落在 " + who + " 身上");
                }
                log_info("寻呼模块：" + who + " 装备槽已满，换人");
                m_unit_equipped_count[who] = 2;
                equip_full = true;
            }
            else if (!still_pending()) {
                consumed = true;
            }
        }

        if (equip_full && retries++ < 3) {
            pagers.push_back(pager); // `_.push(ee)`：放回队尾再试
            continue;
        }
        if (!consumed) {
            std::string first_bond;
            if (const auto* top = find_operator(tgt->name); top != nullptr && !top->bond_names.empty()) {
                first_bond = top->bond_names.front();
            }
            log_info("寻呼模块拖到 " + tgt->name + " 未生效（可能【" + (first_bond.empty() ? "?" : first_bond)
                + "】已没有可出的干员），保留");
            break;
        }
        m_pager_uses = std::max(0, m_pager_uses - 1);
        log_info("寻呼模块 → " + tgt->name);

        // 领免费干员：3 次尝试
        bool got = false;
        for (int ve = 0; ve < 3 && !got; ++ve) {
            auto items = ocr(shot());
            if (auto hit = find_text_regex(items, "^收起")) {
                tap(hit->cx, hit->cy, 400);
            }
            sleep_ms(500);
            cv::Mat snap = shot();
            auto cards = read_shop_cards(snap);
            if (cards.empty()) {
                sleep_ms(500);
                continue;
            }
            ShopSimState sim = make_shop_sim(read_shop_header(snap, ocr(snap)));
            sim.shop = cards;
            std::optional<ShopCard> pick;
            auto cands = rank_shop_candidates(sim);
            if (!cands.empty()) {
                pick = cands.front().card;
            }
            if (!pick.has_value()) {
                for (const auto& c : cards) {
                    if (c.kind == "operator" && m_elite_units.count(c.name) == 0) {
                        pick = c;
                        break;
                    }
                }
            }
            if (!pick.has_value()) {
                pick = cards.front();
            }
            log_info("寻呼模块：领取 " + pick->name);
            got = buy_shop_card(pick->x, pick->name, m_upgrade_x_Ref.value_or(0));
        }
        if (!got) {
            log_info("寻呼模块用后没领到免费干员（识别失败）");
        }
    }
    return true;
}

bool SPAFarmTaskPlugin::try_use_mob_skill(const CameraDetectResult& cam, int gold)
{
    // Exact 1:1 port of `ut` (Mob `[1845, 350]` drag to board unit to elite for 1 gold) in service.pretty.mjs lines 9016-9056 (with `waitStableBoard(4e3)` L9021)
    (void)cam;
    if (m_mob_used_this_round || gold < 1 || m_current_strategy.find("莫布") == std::string::npos) {
        return false;
    }
    const UnitSlot* best_target = nullptr;
    int best_tier = 0;
    for (const auto& u : m_units) {
        if (u.area == "board" && !u.elite && !u.auxiliary && !u.name.empty()) {
            if (const auto* op = find_operator(u.name)) {
                if (op->tier >= best_tier) {
                    best_tier = op->tier;
                    best_target = &u;
                }
            }
        }
    }
    if (!best_target) {
        return false;
    }
    auto st = wait_stable_board(4000);
    if (!st.has_value()) {
        return false;
    }
    auto [tx, ty] = project_point(st->H, best_target->c + 0.5, best_target->r + 0.5);
    log_info("使用大嘴莫布策略升精 -> " + best_target->name);
    drag_board_unit(1845, 350, tx, ty, false);
    sleep_ms(static_cast<int>(std::lround(700.0 * m_step_scale)));
    close_detail_panel();
    m_mob_used_this_round = true;
    return true;
}



void SPAFarmTaskPlugin::run_prep_phase(const cv::Mat& img, const CameraDetectResult& cam)
{
    // Exact 1:1 port of `je(I)` in service.pretty.mjs lines 9984-10251
    m_mob_used_this_round = false;

    // Ensure shop is expanded during shopping
    cv::Mat cur_img = img;
    CameraDetectResult cur_cam = cam;
    if (cur_cam.camera == "collapsed") {
        auto full_ocr = ocr(cur_img);
        if (auto exp_btn = find_ocr(full_ocr, { "展开" })) {
            tap(exp_btn->cx, exp_btn->cy, 450);
            cur_img = shot();
            cur_cam = detect_camera(cur_img, goal_only_allowed());
        }
    }

    refresh_board_snapshot(cur_img, cur_cam, true);

    // `je` 的整备批处理：最多 80 批，每批「读 → 买 → 部署」，状态连续两批不变就停
    bool bench_full_flag = false;   // `v`
    bool unknown_reread = false;    // `ze`
    bool free_cards_taken = false;  // `Ee`
    bool give_up_buys = false;      // `Y`
    bool summon_deployed_flag = false; // `Ie`
    int third_copy_tries = 0;       // `Pe`
    std::string last_state_key;
    int same_state = 0;
    // `je` L10040：`const Xe = new Set` —— 买/换失败的卡槽位，后续批次从规划卡面里剔除
    std::unordered_set<int> failed_card_buckets;

    for (int iter = 0; iter < 80 && !need_exit() && !give_up_buys; ++iter) {
        cur_img = shot();
        auto full_items = ocr(cur_img);
        ShopHeaderState hdr = read_shop_header(cur_img, full_items);
        // `je` L10034：`xe.level !== null && (i.level = xe.level)`
        // 某一批没读到等级只沿用上一批的值，绝不中断循环。
        // 「商店等级暂未识别」在 JS 里只出现在循环外的 `i.level === null` 分支
        // （该分支由 pc 升级失败置 null 触发，非 pc 恒不可达）。
        if (hdr.level.has_value()) {
            m_current_shop_level = *hdr.level;
        }
        // `je` L10033：`if (!xe || xe.gold === null) throw new et("商店资金识别失败")`
        // → catch 里 `ne(...；跳过本回合剩余商店操作，继续部署), break`
        if (!hdr.gold.has_value()) {
            log_info("商店资金识别失败；跳过本回合剩余商店操作，继续部署");
            break;
        }
        m_current_gold = *hdr.gold;
        if (hdr.upgrade_btn.has_value()) {
            m_upgrade_x_Ref = hdr.upgrade_btn->first;
        }

        const bool units_full_text = has_ocr(full_items, { "整备区已达上限", "干员已满" });
        if (units_full_text) {
            bench_full_flag = true;
        }

        m_shop = read_shop_cards(cur_img);

        // `je` L10039：`Je = { ...Ve, shop: Ve.shop.filter(De => !Xe.has(Math.round(De.x / 100))) }`
        // 规划与状态指纹都用过滤后的卡面；免费卡识别 `Mt(D, Ve.shop)` 仍用完整 `m_shop`。
        std::vector<ShopCard> planning_shop;
        planning_shop.reserve(m_shop.size());
        for (const auto& c : m_shop) {
            const int bucket = static_cast<int>(std::lround(c.x / 100.0));
            if (failed_card_buckets.count(bucket) == 0) {
                planning_shop.push_back(c);
            }
        }

        // 连续两批状态完全不变 → 说明点了也没生效，停止购买
        {
            std::string key = std::to_string(m_current_gold) + "|" + std::to_string(m_current_shop_level) + "|";
            for (const auto& c : planning_shop) {
                key += c.name;
                key += ",";
            }
            key += "|" + std::to_string(m_units.size());
            if (key == last_state_key) {
                ++same_state;
            }
            else {
                same_state = 0;
                last_state_key = key;
            }
            if (same_state >= 2) {
                log_info("商店连续两批操作后资金、等级与卡面均未变化，跳过剩余购买，继续部署");
                break;
            }
        }

        // `Mt`：精锐免费选择（>=2 张时必领一张）
        {
            auto marked = m_shop;
            detect_free_cards(cur_img, marked);
            std::vector<ShopCard> free_only;
            for (const auto& c : marked) {
                if (c.free_card) {
                    free_only.push_back(c);
                }
            }
            if (!free_cards_taken && static_cast<int>(free_only.size()) >= 2) {
                // `!(nt.every(同首盟约) && Kt === round) && Gt && !Et.has(Gt) && (Et.add(Gt), p.add(Gt), r())`
                {
                    bool all_same_first_bond = true;
                    std::string first_bond;
                    for (size_t k = 0; k < free_only.size(); ++k) {
                        const auto* cop = find_operator(free_only[k].name);
                        const std::string b = (cop != nullptr && !cop->bond_names.empty()) ? cop->bond_names.front() : std::string();
                        if (k == 0) {
                            first_bond = b;
                        }
                        else if (b != first_bond) {
                            all_same_first_bond = false;
                            break;
                        }
                    }
                    const bool same_round = (m_pager_round == m_current_round);
                    if (!(all_same_first_bond && same_round)
                        && !m_last_bought_name.empty()
                        && m_elite_units.count(m_last_bought_name) == 0) {
                        m_elite_units.insert(m_last_bought_name);
                        m_pending_merge.insert(m_last_bought_name);
                    }
                }
                ShopSimState sim_free = make_shop_sim(hdr);
                sim_free.shop = free_only;
                std::optional<ShopCard> pick;
                if (m_mode == "standard" && m_goal != "medal") {
                    auto cands = rank_shop_candidates(sim_free);
                    if (!cands.empty()) {
                        pick = cands.front().card;
                    }
                }
                if (!pick.has_value()) {
                    for (const auto& sc : score_shop_cards(sim_free)) {
                        if (sc.score > 0) {
                            pick = sc.card;
                            break;
                        }
                    }
                }
                if (!pick.has_value()) {
                    for (const auto& c : free_only) {
                        if (c.kind == "operator" && m_elite_units.count(c.name) == 0) {
                            pick = c;
                            break;
                        }
                    }
                }
                if (!pick.has_value()) {
                    for (const auto& c : free_only) {
                        if (m_elite_units.count(c.name) == 0) {
                            pick = c;
                            break;
                        }
                    }
                }
                if (!pick.has_value()) {
                    pick = free_only.front();
                }
                free_cards_taken = true;
                log_info("回合 " + std::to_string(m_current_round) + " · 精锐免费选择：领取 " + pick->name);
                if (buy_shop_card(pick->x, pick->name, m_upgrade_x_Ref.value_or(0))) {
                    m_bought_operators.insert(pick->name);
                    continue;
                }
            }
        }

        // 整备区满 / 干员已满：优先凑第三张，其次按 `Lr` 买一名有用的并卖过渡
        if (third_copy_tries < 4 && (bench_full_flag || units_full_text)) {
            std::optional<ShopCard> third;
            for (const auto& c : m_shop) {
                if (c.kind != "operator" || c.name.empty() || m_elite_units.count(c.name) > 0) {
                    continue;
                }
                if (is_transition_unit(c.name)) {
                    continue;
                }
                int cnt = 0;
                for (const auto& u : m_units) {
                    if (u.name == c.name && !u.elite && !u.auxiliary) {
                        ++cnt;
                    }
                }
                if (cnt != 2) {
                    continue;
                }
                const auto* op = find_operator(c.name);
                if (op == nullptr || op->purchase_price > m_current_gold) {
                    continue;
                }
                third = c;
                break;
            }

            auto is_useful_for_plan = [&](std::string_view n) {
                const auto* op = find_operator(n);
                if (op == nullptr) {
                    return false;
                }
                for (const auto& b : op->bond_names) {
                    if (std::find(m_active_core_bonds.begin(), m_active_core_bonds.end(), b) != m_active_core_bonds.end()) {
                        return true;
                    }
                    if (m_goal == "medal" && b == "迅捷") {
                        return true;
                    }
                }
                if (is_stacker(*op)) {
                    return true;
                }
                static const std::vector<std::string> kFinishers = { "蕾缪安", "塔露拉", "艾雅法拉", "维伊" };
                return std::find(kFinishers.begin(), kFinishers.end(), op->name) != kFinishers.end();
            };

            std::optional<ShopCard> alt;
            if (!third.has_value()) {
                if (m_mode == "standard" && m_goal != "medal") {
                    ShopSimState sim_now = make_shop_sim(hdr);
                    for (const auto& c : rank_shop_candidates(sim_now)) {
                        if (c.unit_price <= m_current_gold + 1) {
                            alt = c.card;
                            break;
                        }
                    }
                }
                else {
                    ShopSimState sim_now = make_shop_sim(hdr);
                    for (const auto& sc : score_shop_cards(sim_now)) {
                        if (sc.score <= 0 || !is_useful_for_plan(sc.card.name)) {
                            continue;
                        }
                        const auto* op = find_operator(sc.card.name);
                        if (op == nullptr || op->purchase_price > m_current_gold + 1) {
                            continue;
                        }
                        bool owned = false;
                        for (const auto& u : m_units) {
                            if (u.name == sc.card.name) {
                                owned = true;
                                break;
                            }
                        }
                        if (owned) {
                            continue;
                        }
                        alt = sc.card;
                        break;
                    }
                }
            }

            const std::optional<ShopCard> target = third.has_value() ? third : alt;
            if (target.has_value()) {
                bool unknown_bench = false;
                for (const auto& u : m_units) {
                    if (u.area == "bench" && (u.name.empty() || !u.confirmed)) {
                        unknown_bench = true;
                        break;
                    }
                }
                if (!unknown_reread && unknown_bench) {
                    unknown_reread = true;
                    // `Te(Qe,/^收起/) && W(Qe,/^收起/,400)`
                    if (auto col = find_text_regex(ocr(shot()), "^收起")) {
                        tap(col->cx, col->cy, 400);
                    }
                    auto reread_img = shot();
                    refresh_board_snapshot(reread_img, cur_cam, false);
                    inspect_unknown_units(cur_cam, true);
                    // `await Ke(await h())`：展开后等「升级 / 已满级」出现
                    if (auto exp = find_text_regex(ocr(shot()), "^.?展开$")) {
                        tap(exp->cx, exp->cy, 250);
                        wait_for(
                            [](const ScreenInfo& s) {
                                return s.screen == ScreenState::Prep
                                    && (has_ocr_exact(s.texts, { "升级" }) || has_ocr_exact(s.texts, { "已满级" }));
                            },
                            4000);
                    }
                    continue;
                }
                // `Ie`：整备区里有还没上场的召唤物 → 先 `Dt` 把它们放上去
                {
                    bool has_summon = false;
                    for (const auto& u : m_units) {
                        if (u.area == "bench" && u.auxiliary && !u.item && !u.name.empty()
                            && find_operator(u.name) == nullptr) {
                            has_summon = true;
                            break;
                        }
                    }
                    if (target.has_value() && has_summon && !summon_deployed_flag) {
                        summon_deployed_flag = true;
                        // `Te(Bt,/^收起/) && W(Bt,/^收起/,400)` → `Dt` → `snapshot({force:!0})` → `Ke`
                        if (auto col = find_text_regex(ocr(shot()), "^收起")) {
                            tap(col->cx, col->cy, 400);
                        }
                        auto sst = wait_stable_board(4000);
                        if (sst.has_value()) {
                            CameraDetectResult sc { sst->camera, sst->raw_H, sst->fit };
                            refresh_board_snapshot(sst->img, sc, false);
                            deploy_summons(build_stage_traffic(m_stage_id, m_current_round <= 3));
                            refresh_board_snapshot(shot(), sc, true);
                            if (auto exp = find_text_regex(ocr(shot()), "^.?展开$")) {
                                tap(exp->cx, exp->cy, 250);
                            }
                        }
                        continue;
                    }
                }

                ++third_copy_tries;
                if (third.has_value()) {
                    log_info("整备区满：" + target->name + " 已有两张，买第三张凑精锐");
                }
                if (buy_shop_card(target->x, target->name, m_upgrade_x_Ref.value_or(0))) {
                    m_bought_operators.insert(target->name);
                    continue;
                }
                const UnitSlot* sell = pick_sell_bench_unit(target->name);
                if (sell == nullptr) {
                    log_info("整备区满，想买 " + target->name + " 但没有可卖的过渡期干员");
                }
                else {
                    const std::string sell_slot = sell->slot;
                    const std::string sell_name = sell->name;
                    const int gold_before = m_current_gold;
                    if (sell_unit_slot(sell_slot, gold_before)) {
                        auto after_img = shot();
                        auto after_items = ocr(after_img);
                        auto after_hdr = read_shop_header(after_img, after_items);
                        if (after_hdr.gold.has_value() && *after_hdr.gold > gold_before) {
                            log_info("整备区满：卖掉过渡期干员 " + sell_name + "，腾位买 " + target->name);
                            m_filler_units.erase(sell_name);
                            buy_shop_card(target->x, target->name, m_upgrade_x_Ref.value_or(0));
                            m_bought_operators.insert(target->name);
                            cur_img = after_img;
                            continue;
                        }
                        log_info("卖出 " + sell_name + " 未确认成功");
                    }
                }
            }
        }
        auto steps = plan_shop_Steps(hdr, planning_shop);
        if (units_full_text) {
            steps.erase(
                std::remove_if(steps.begin(), steps.end(), [](const ShopPlanStep& st) {
                    return st.action == "buy" && st.kind != "item";
                }),
                steps.end());
        }
        if (steps.empty() || steps.front().action == "done") {
            // 计划已结束：冻结一张值得留的卡（`bs` / 标准模式下的 `_r` 买不起项）
            if (m_current_round < m_last_round) {
                std::optional<std::string> keep;
                const ShopSimState sim_now = make_shop_sim(hdr);
                if (m_mode == "standard" && m_goal != "medal") {
                    for (const auto& c : rank_shop_candidates(sim_now)) {
                        if (c.unit_price > m_current_gold) {
                            keep = c.unit_name;
                            break;
                        }
                    }
                }
                else {
                    keep = pick_freeze_target(sim_now);
                }
                if (keep.has_value()) {
                    const auto items_now = ocr(cur_img);
                    const bool thawed = has_ocr(items_now, { "解冻" });
                    auto freeze_btn = find_text_regex(items_now, "^冻结$");
                    if (!thawed && freeze_btn.has_value()) {
                        tap(freeze_btn->cx, freeze_btn->cy, 250);
                        log_info("冻结商店，保留 " + *keep);
                    }
                    else {
                        log_info("想冻结保留 " + *keep + "，但" + (thawed ? "商店已是冻结状态" : "没找到「冻结」按钮"));
                    }
                }
            }
            break;
        }

        bool acted = false;
        for (const auto& step : steps) {
            if (need_exit()) {
                return;
            }
            if (step.action == "done") {
                break;
            }
            if (step.action == "upgrade") {
                log_info("商店动作: 升级商店 -> " + step.target + " (费用:" + std::to_string(step.cost) + ")");
                if (upgrade_shop(hdr)) {
                    acted = true;
                    ++m_current_shop_level; // `je` L10204：`i.level++, Oe = !0, Ce.upgrades++`
                }
                // `ct` 返回 !1 时 JS 只 `break` 掉本批的 `for (const De of re)`，
                // 外层 80 批循环继续；只有 `ct` 抛 `et`（pc 专属，MAA 不可达）
                // 才会被 catch 成「跳过本回合剩余商店操作」并终止整轮。
                break;
            }
            if (step.action == "swap") {
                log_info("商店动作: 出售 " + step.sell_name + " 换购 " + step.target);
                if (sell_unit_slot(step.sell_slot, m_current_gold)) {
                    if (buy_shop_card(step.x, step.target, m_upgrade_x_Ref.value_or(0))) {
                        m_filler_units.erase(step.sell_name);
                        m_bought_operators.insert(step.target);
                        acted = true;
                    }
                    else {
                        // `je` L10197：`Xe.add(Math.round(De.card.x / 100))`
                        failed_card_buckets.insert(static_cast<int>(std::lround(step.x / 100.0)));
                    }
                }
                else {
                    // `je` L10179：卖出未确认 → 同样把该卡从后续规划里剔除
                    failed_card_buckets.insert(static_cast<int>(std::lround(step.x / 100.0)));
                }
                break;
            }
            if (step.action == "buy") {
                log_info("商店动作: 购买 " + step.target + " (" + step.reason + ")");
                if (buy_shop_card(step.x, step.target, m_upgrade_x_Ref.value_or(0))) {
                    acted = true;
                    // 注意：`bought/upgrades/itemBought/fillerBought` 已在 `plan_shop_Steps`
                    // 的推演里计入，这里只维护持久集合，避免重复计数。
                    if (step.kind == "item") {
                        m_bought_items.insert(step.target);
                    }
                    else {
                        m_bought_operators.insert(step.target);
                        if (step.filler) {
                            m_filler_units.insert(step.target);
                            log_info("前期补位：买 " + step.target + "（招第 9 名干员前卖掉）");
                        }
                    }
                    // Immediately refresh snapshot after purchase
                    cur_img = shot();
                    refresh_board_snapshot(cur_img, cur_cam, false);
                }
                else {
                    // `je` L10193：`if (!await Tt(...)) Xe.add(Math.round(De.card.x / 100)); break`
                    failed_card_buckets.insert(static_cast<int>(std::lround(step.x / 100.0)));
                }
            }
            else if (step.action == "refresh") {
                log_info("商店动作: 刷新商店");
                if (hdr.refresh_btn.has_value()) {
                    tap(hdr.refresh_btn->first, hdr.refresh_btn->second, 450);
                }
                else {
                    tap(1786, 686, 450);
                }
                acted = true;
                break;
            }
        }
        // `je` 没有「本批无成功动作就终止整轮」这个分支：
        // 动作失败时 JS 只 `break` 掉内层 `for (const De of re)`，
        // 外层 80 批循环继续，由 `ar === L`（上面的 same_state>=2）与 80 批上限收敛。
        // 旧实现在这里直接 break，导致一次点击失败就把本回合剩余购买/升级/装备/寻呼全跳过。
        (void)acted;
    }

    // Collapse shop if needed to get clear view of board/bench for unit inspection & placement
    cur_img = shot();
    cur_cam = detect_camera(cur_img, goal_only_allowed());
    log_info(std::string(cur_cam.camera.empty() ? "校准失败（跳过部署/装备/召唤）: " : "校准: ") + cur_cam.detail);
    if (!cur_cam.camera.empty()) {
        refresh_board_snapshot(cur_img, cur_cam, false);
        inspect_unknown_units(cur_cam, true);

        // `je` L10223-10230：先跑寻呼模块（`Ur`），task 目标 + 杰西卡才走
        if (m_goal != "medal" && m_current_strategy.find("杰西卡") != std::string::npos) {
            auto stg = std::find_if(m_stages.begin(), m_stages.end(), [&](const StageInfo& st) {
                return st.stage_id == m_stage_id;
            });
            if (stg != m_stages.end()) {
                try_use_jessica_pager(cur_cam);
            }
        }
        // `$e()`：整备区满 / 有无法放置的单位 → `nn`（回收暂存区到候补席）
        if (m_goal != "medal") {
            int bench_n = 0;
            for (const auto& u : m_units) {
                if (u.area == "bench") {
                    ++bench_n;
                }
            }
            int h_cnt = 10;
            for (const auto& stg : m_stages) {
                if (stg.stage_id != m_stage_id) {
                    continue;
                }
                for (const auto& row : stg.rows) {
                    if (row.find('H') == std::string::npos) {
                        continue;
                    }
                    const int cnt = static_cast<int>(std::count(row.begin(), row.end(), 'H'));
                    h_cnt = (cnt > 0) ? cnt : 10;
                    break;
                }
                break;
            }
            const bool cannot_place = has_ocr(ocr(shot()), { "无法放置的单位" });
            if (cannot_place || bench_n >= h_cnt) {
                handle_overflow_units(cur_cam);
            }
        }

        // `we()`：内部完成 部署 → 召唤物 → 装备（`on`）
        //   失败（`wa`）→ `we(!0)` 重试一次，仍失败则保留当前阵容继续
        if (run_auto_place(cur_cam, false) < 0) {
            log_info("部署重排后仍失败，保留当前阵容继续游戏");
            close_detail_panel();
            run_auto_place(cur_cam, false, true);
        }

        // `$e() && strategy==="杰西卡" && vt > 0` → 再跑一次寻呼
        if (m_goal != "medal" && m_current_strategy.find("杰西卡") != std::string::npos && m_pager_uses > 0) {
            try_use_jessica_pager(cur_cam);
        }
        try_use_mob_skill(cur_cam, m_current_gold);
    }

    // 收尾前重读一次商店头：升级/购买后等级与资金已经变了，
    // 沿用循环里最后一次成功的旧值会让「整备完成」日志整体滞后一档。
    {
        cv::Mat tail_img = shot();
        if (!tail_img.empty()) {
            auto tail_items = ocr(tail_img);
            ShopHeaderState tail_hdr = read_shop_header(tail_img, tail_items);
            if (tail_hdr.level.has_value()) {
                m_current_shop_level = *tail_hdr.level;
            }
            if (tail_hdr.gold.has_value()) {
                m_current_gold = *tail_hdr.gold;
            }
        }
    }

    // Click Ready (`准备就绪` `[1780, 170]`)
    log_info(
        "回合 " + std::to_string(std::max(1, m_current_round))
        + " 整备完成 (商店Lv" + std::to_string(m_current_shop_level)
        + " 资金:" + std::to_string(m_current_gold) + ") -> 点击准备就绪");
    tap(1780, 170, 800);
    m_last_prep_round = m_current_round;
    cv::Mat post_ready = shot();
    if (!post_ready.empty()) {
        auto pr_items = ocr(post_ready);
        if (has_ocr(pr_items, { "是否确认准备", "确认准备", "资金将在" })) {
            if (auto no_prompt = find_ocr(pr_items, { "本局不再提示", "不再提示" })) {
                tap(no_prompt->x - 20, no_prompt->y + no_prompt->h * 0.5, 250);
            }
            else {
                tap(860, 602, 250);
            }
            if (auto hit = find_ocr_exact(pr_items, { "确认", "确定" })) {
                tap(hit->cx, hit->cy, 1000);
            }
            else {
                tap(1410, 728, 1000);
            }
        }
    }
}

// ============================================================================
// Screen Classifier (`bi`) & Strategy / Squad / Popup Handlers (`Ot`, `Ft`, `Or`, `Vn`, `K`, `ce`, `ln`)
// ============================================================================

SPAFarmTaskPlugin::ScreenInfo SPAFarmTaskPlugin::classify_screen(const cv::Mat& img) const
{
    // `bi(a, s=false)` —— service.pretty.mjs L4766-4806，逐行对齐（`pc` 分支已按实测结论删除）
    ScreenInfo out;
    out.png = img;
    if (img.empty()) {
        return out;
    }
    out.texts = ocr(img);
    const std::vector<OcrItem>& e = out.texts;

    auto t = [&](const std::string& pattern) {
        return has_text_regex(e, pattern);
    };
    const std::string kQuickRe = "极速模拟|规程等级[0-9]+";

    // `w = t(/极速模拟|规程等级\d+/)`
    bool w = t(kQuickRe);
    std::string level_name;

    // `m()`：左上角 260x80 放大 3 倍做一次 rec，用来确认极速模拟
    auto read_corner = [&]() -> std::string {
        cv::Mat crop = img_crop(img, 140, 20, 260, 80);
        if (crop.empty()) {
            return {};
        }
        crop = img_resize_cubic(crop, 780, 240);
        std::string joined;
        for (const auto& it : ocr(crop)) {
            if (it.score >= 0.65) {
                joined += strip_ws(it.text);
            }
        }
        return joined;
    };

    if (!w && t("确认本局信息|选择策略|回合|休息一下")) {
        const std::string corner = read_corner();
        w = regex_test(corner, kQuickRe);
        if (auto m = regex_first(corner, kQuickRe)) {
            level_name = *m;
        }
    }

    if (level_name.empty()) {
        std::optional<OcrItem> pick;
        for (const auto& it : e) {
            const std::string txt = strip_ws(it.text);
            if (it.y < 120 && it.x < 520 && regex_test(txt, kQuickRe)) {
                pick = it;
                break;
            }
        }
        if (!pick.has_value() && t("请选择模拟难度")) {
            for (const auto& it : e) {
                const std::string txt = strip_ws(it.text);
                if (it.y > 980 && it.x < 400 && regex_test(txt, "规程等级[0-9]+")) {
                    pick = it;
                    break;
                }
            }
        }
        if (pick.has_value()) {
            if (auto m = regex_first(strip_ws(pick->text), kQuickRe)) {
                level_name = *m;
            }
        }
    }

    // `u = combat`：顶部中心的 x/y 回合计数
    auto is_ratio = [](const std::string& s) {
        return regex_test(s, "^[0-9]{1,3}/[0-9]{1,3}$");
    };
    bool combat = false;
    if (w) {
        for (const auto& it : e) {
            if (it.score >= 0.7 && it.x >= 600 && it.x < 1250 && it.y < 100 && is_ratio(strip_ws(it.text))) {
                combat = true;
                break;
            }
        }
    }
    if (!combat && w && !t("准备就绪|休息一下|选择策略|确认本局信息|自选编队|返回主页|开始模拟|个人模拟")) {
        cv::Mat crop = img_crop(img, 650, 0, 540, 105);
        if (!crop.empty()) {
            crop = img_resize_cubic(crop, 1620, 315);
            for (const auto& it : ocr(crop)) {
                if (it.score >= 0.8 && is_ratio(strip_ws(it.text))) {
                    combat = true;
                    break;
                }
            }
        }
    }

    // 状态判定链，顺序与 JS 完全一致
    if (t("确认准备") && t("剩余资金|未使用的|指令")) {
        out.screen = ScreenState::ConfirmReady;
    }
    else if (t("选择策略")) {
        out.screen = ScreenState::Strategy;
    }
    else if (t("确认本局信息")) {
        out.screen = ScreenState::Info;
    }
    else if (t("自选编队")) {
        out.screen = ScreenState::Squad;
    }
    else if (t("返回主页|模拟结束|模拟成功|模拟失败|模拟胜利|模拟完成")) {
        out.screen = ScreenState::Result;
    }
    else if (t("休息") && t("准备就绪")) {
        out.screen = ScreenState::Prep;
    }
    else if (!t("准备就绪") && (t("回合") || combat)) {
        out.screen = ScreenState::Battle;
    }
    else if (t("开始模拟") && (w || t("请选择模拟难度"))) {
        out.screen = ScreenState::Mode;
    }
    else if (t("^个人模拟$") && t("^联合模拟$") && t("协议数据库")) {
        out.screen = ScreenState::Lobby;
    }
    else {
        out.screen = ScreenState::Unknown;
    }

    // `wins`：把所有 OCR 文本拼起来找「已通过模拟N次」
    std::string joined;
    for (const auto& it : e) {
        if (!joined.empty()) {
            joined += "|";
        }
        joined += strip_ws(it.text);
    }
    if (auto m = regex_first_group(joined, "已通过模拟([0-9]+)次")) {
        out.wins = std::stoi(*m);
    }

    out.quick = w;
    out.levelName = level_name;
    out.difficulty = t("请选择模拟难度");
    out.combat = combat;
    out.selected = t("确认选择");
    if (t("模拟成功|模拟胜利|模拟完成")) {
        out.won = true;
    }
    else if (t("模拟失败")) {
        out.won = false;
    }
    if (out.screen == ScreenState::Lobby) {
        out.personal_selected = is_personal_selected(img, e);
    }
    return out;
}

cv::Mat SPAFarmTaskPlugin::extract_strategy_icon(const cv::Mat& img, int gx, int gy)
{
    // Exact 1:1 port of `Ft` / `Or` in service.pretty.mjs lines 4891-4906 (80x80 grayscale icon patch)
    int x0 = std::clamp(gx - 40, 0, std::max(0, img.cols - 80));
    int y0 = std::clamp(gy - 40, 0, std::max(0, img.rows - 80));
    cv::Mat patch = img(cv::Rect(x0, y0, 80, 80));
    cv::Mat gray;
    cv::cvtColor(patch, gray, cv::COLOR_BGR2GRAY);
    return gray;
}

double SPAFarmTaskPlugin::icon_diff(const cv::Mat& a, const cv::Mat& b)
{
    if (a.empty() || b.empty() || a.size() != b.size()) {
        return 255.0;
    }
    double s = 0.0;
    const int n = a.rows * a.cols;
    for (int y = 0; y < a.rows; ++y) {
        const uint8_t* ra = a.ptr<uint8_t>(y);
        const uint8_t* rb = b.ptr<uint8_t>(y);
        for (int x = 0; x < a.cols; ++x) {
            s += std::abs(static_cast<int>(ra[x]) - static_cast<int>(rb[x]));
        }
    }
    return n > 0 ? (s / static_cast<double>(n)) : 255.0;
}

bool SPAFarmTaskPlugin::has_strategy_selected_border(const cv::Mat& img, int gx, int gy)
{
    const int strips[4][4] = {
        { gx - 88, gy - 88, gx + 88, gy - 72 },
        { gx - 88, gy + 72, gx + 88, gy + 88 },
        { gx - 88, gy - 72, gx - 72, gy + 72 },
        { gx + 72, gy - 72, gx + 88, gy + 72 }
    };
    int orange = 0, total = 0;
    for (const auto& st : strips) {
        int x0 = std::clamp(st[0], 0, img.cols);
        int y0 = std::clamp(st[1], 0, img.rows);
        int x1 = std::clamp(st[2], 0, img.cols);
        int y1 = std::clamp(st[3], 0, img.rows);
        for (int y = y0; y < y1; ++y) {
            const auto* row = img.ptr<cv::Vec3b>(y);
            for (int x = x0; x < x1; ++x) {
                int B = row[x][0], G = row[x][1], R = row[x][2];
                ++total;
                if (R > 180 && G > 50 && G < 160 && B < 60 && (R - G) > 40) {
                    ++orange;
                }
            }
        }
    }
    return total > 0 && (static_cast<double>(orange) / total) > 0.25;
}

bool SPAFarmTaskPlugin::has_strategy_badge(const cv::Mat& img, int gx, int gy, int size) const
{
    // `Vn(a, s, e, t = 190)` —— service.pretty.mjs L4891-4906
    //   extract({left: round(s+t-58), top: round(e), width:58, height:65}).removeAlpha().raw()
    //   计数：R>150 && G>95 && G < R*0.94 && B < G*0.62
    //   return 计数 >= 130
    const int left = static_cast<int>(std::lround(static_cast<double>(gx + size - 58)));
    const int top = static_cast<int>(std::lround(static_cast<double>(gy)));
    cv::Mat crop = img_crop(img, left, top, 58, 65);
    if (crop.empty()) {
        return false;
    }
    const RawRgb raw = img_to_rgb(crop);
    int count = 0;
    for (size_t i = 0; i + 2 < raw.data.size(); i += 3) {
        const int R = raw.data[i];
        const int G = raw.data[i + 1];
        const int B = raw.data[i + 2];
        if (R > 150 && G > 95 && G < R * 0.94 && B < G * 0.62) {
            ++count;
        }
    }
    return count >= 130;
}

bool SPAFarmTaskPlugin::handle_strategy_screen(const cv::Mat& img)
{
    // `Ot(I)` —— service.pretty.mjs L8711-8853，逐行对齐（`pc` 分支按实测结论删除）
    //
    // 结构：
    //   最多 40 页 × 每页若干行 × 4 列
    //   每格：先取 12×12 灰度指纹 → 与已见策略比距离（<12 且与次优差 >3 才认）
    //         → 已达标且（目标>3 或 已有刻章）或本轮已试过 → 跳过
    //         → 点击 → 读右侧面板名（`mt`）→ 记进度 → 若未达标则确认选择并返回
    //   整页扫完：用 16×16 指纹判断是否已到列表末尾，否则向下拖拽翻页
    const bool medal_goal = (m_goal != "medal") ? false : true; // JS `be()` 是 "!= medal"，这里取反
    const bool by_task = !medal_goal;                            // JS `be()`
    const int target = m_target;

    auto frame = [&]() -> std::optional<ScreenInfo> {
        cv::Mat shot_img = shot();
        if (shot_img.empty()) {
            return std::nullopt;
        }
        return classify_screen(shot_img);
    };

    ScreenInfo info;
    if (!img.empty()) {
        info = classify_screen(img);
    }
    else if (auto f = frame()) {
        info = *f;
    }
    else {
        return false;
    }

    // 本页已尝试过的策略名（JS 的 `B`，每次调用重置）
    std::set<std::string> tried;
    bool skipped_by_goal = false; // JS 的 `_`
    std::string last_fingerprint;

    // `Ue(I, k)`：写入进度并重算 completed
    auto update_progress = [&](const std::string& name, int wins) {
        m_strategy_clears[name] = std::min(wins, target);
    };

    for (int page = 0; page < 40 && !need_exit(); ++page) {
        std::vector<int> rows = detect_strategy_rows(info.png, info.selected);
        for (int retry = 0; rows.empty() && retry < 8; ++retry) {
            sleep_ms(400);
            auto f = frame();
            if (!f.has_value()) {
                return false;
            }
            if (f->screen != ScreenState::Strategy) {
                log_info("策略选择页面已离开（可能倒计时结束），请重新开始");
                return false;
            }
            info = *f;
            rows = detect_strategy_rows(info.png, info.selected);
        }
        if (rows.empty()) {
            log_info("未识别到策略卡片边框，请保持游戏为 16:9 横屏");
            return false;
        }

        const int panel_x = info.selected ? 586 : 982;
        constexpr int kCellSize = 190;

        for (int row_y : rows) {
            for (int col = 0; col < 4; ++col) {
                if (need_exit()) {
                    return false;
                }
                const int cell_x = panel_x + col * 210;

                // 12×12 灰度指纹
                cv::Mat crop = img_crop(
                    info.png,
                    cell_x + static_cast<int>(std::lround(kCellSize * 0.13)),
                    row_y + static_cast<int>(std::lround(kCellSize * 0.16)),
                    static_cast<int>(std::lround(kCellSize * 0.74)),
                    static_cast<int>(std::lround(kCellSize * 0.74)));
                cv::Mat pixels;
                if (crop.empty()) {
                    continue;
                }
                cv::resize(img_gray(crop), pixels, cv::Size(12, 12), 0.0, 0.0, cv::INTER_AREA);

                // 与已见策略比距离，取最近与次近
                struct Neighbor
                {
                    std::string name;
                    double d = 1e9;
                };
                std::vector<Neighbor> neighbors;
                neighbors.reserve(m_seen_strategy_icons.size());
                for (const auto& seen : m_seen_strategy_icons) {
                    if (seen.icon.empty() || seen.icon.size() != pixels.size()) {
                        continue;
                    }
                    double sum = 0.0;
                    for (int y = 0; y < 12; ++y) {
                        for (int x = 0; x < 12; ++x) {
                            sum += std::abs(static_cast<int>(pixels.at<uint8_t>(y, x))
                                            - static_cast<int>(seen.icon.at<uint8_t>(y, x)));
                        }
                    }
                    neighbors.push_back(Neighbor { seen.name, sum / 144.0 });
                }
                std::sort(neighbors.begin(), neighbors.end(), [](const Neighbor& a, const Neighbor& b) {
                    return a.d < b.d;
                });

                std::string known_name;
                if (!neighbors.empty() && neighbors[0].d < 12.0
                    && (neighbors.size() < 2 || neighbors[1].d - neighbors[0].d > 3.0)) {
                    known_name = neighbors[0].name;
                }

                // 已达标（medal）或本轮已试 → 跳过
                if (known_name.empty()) {
                    // 未知格子仍要点击确认，交给下面的流程
                }
                else {
                    const int passed = m_strategy_clears.count(known_name) ? m_strategy_clears[known_name] : 0;
                    if (!by_task && passed >= target) {
                        const bool enough = (target > 3) ? true : has_strategy_badge(info.png, cell_x, row_y, kCellSize);
                        if (enough) {
                            continue;
                        }
                    }
                    if (tried.count(known_name) > 0) {
                        continue;
                    }
                }

                tap(cell_x + kCellSize / 2.0, row_y + kCellSize / 2.0, 350);
                auto after = frame();
                if (!after.has_value()) {
                    return false;
                }
                info = *after;
                if (info.screen != ScreenState::Strategy) {
                    log_info("策略选择页面已离开（可能倒计时结束），请重新开始");
                    return false;
                }

                std::string picked = strategy_from_panel(info);
                for (int retry = 0; picked.empty() && retry < 2; ++retry) {
                    sleep_ms(400);
                    auto f = frame();
                    if (!f.has_value() || f->screen != ScreenState::Strategy) {
                        break;
                    }
                    info = *f;
                    picked = strategy_from_panel(info);
                }
                if (picked.empty() || tried.count(picked) > 0) {
                    continue;
                }
                tried.insert(picked);

                // 缓存指纹
                if (m_seen_strategy_icons.size() < 128) {
                    bool exists = false;
                    for (const auto& s : m_seen_strategy_icons) {
                        if (s.name == picked) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        m_seen_strategy_icons.push_back(SeenStrategyIcon { picked, pixels.clone() });
                    }
                }

                // 胜场未显示时：先看「未通过模拟」，再看刻章标记
                std::optional<int> wins = info.wins;
                if (!wins.has_value()) {
                    if (has_text_regex(info.texts, "未通过模拟|尚未通过|未通关")) {
                        wins = 0;
                    }
                    else if (has_strategy_badge(info.png, cell_x, row_y, kCellSize)) {
                        if (target > 3 || by_task) {
                            wins = 3;
                        }
                        else {
                            log_info(picked + " 已显示刻章，但胜场数未读到");
                        }
                    }
                }
                if (wins.has_value()) {
                    update_progress(picked, *wins);
                }

                // 目标策略过滤（task-N 只跑指定策略）
                std::vector<std::string> goal_strategies;
                for (const auto& gt : kGoalTasks) {
                    if (gt.id == m_goal) {
                        goal_strategies = gt.strategies;
                        break;
                    }
                }
                if (by_task && !goal_strategies.empty()
                    && std::find(goal_strategies.begin(), goal_strategies.end(), picked) == goal_strategies.end()) {
                    skipped_by_goal = true;
                    continue;
                }

                if (!by_task) {
                    // medal：已经全部达标就收工；否则本策略达标就跳过
                    int completed = 0;
                    for (const auto& sr : m_strategies) {
                        if (!sr.available) {
                            continue;
                        }
                        const int p = m_strategy_clears.count(sr.name) ? m_strategy_clears[sr.name] : 0;
                        if (p >= target) {
                            ++completed;
                        }
                    }
                    int avail = 0;
                    for (const auto& sr : m_strategies) {
                        if (sr.available) {
                            ++avail;
                        }
                    }
                    if (completed == avail) {
                        log_info("极速模拟可用策略均已确认获得刻章");
                        return false;
                    }
                    if (wins.has_value() && *wins >= target) {
                        continue;
                    }
                }

                m_current_strategy = picked;
                m_current_strategy_id.clear();
                for (const auto& sr : m_strategies) {
                    if (sr.name == picked) {
                        m_current_strategy_id = sr.id;
                        break;
                    }
                }
                log_info("选择 " + picked + " · "
                         + (by_task ? m_goal : (wins.has_value() ? "已通过 " + std::to_string(*wins) + "/"
                                                                       + std::to_string(target) + " 次"
                                                                 : std::string("未获刻章（胜场未显示）"))));

                if (auto hit = find_text_regex(info.texts, "^确认选择$")) {
                    tap(hit->cx, hit->cy, 350);
                }
                else {
                    // `W(k, /确认选择/)` 在找不到时会抛；这里按 JS 语义视为失败
                    log_info("画面没有预期按钮：确认选择");
                    return false;
                }
                wait_for(
                    [](const ScreenInfo& s) {
                        return s.screen == ScreenState::Squad || s.screen == ScreenState::Prep;
                    },
                    15000);
                return true;
            }
        }

        // 翻页：16×16 指纹判末尾
        cv::Mat tail = img_crop(info.png, panel_x, 300, 820, 700);
        if (tail.empty()) {
            break;
        }
        cv::Mat tiny;
        cv::resize(tail, tiny, cv::Size(16, 16), 0.0, 0.0, cv::INTER_AREA);
        std::string fingerprint;
        fingerprint.reserve(16 * 16 * 3);
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                const auto& px = tiny.at<cv::Vec3b>(y, x);
                fingerprint.push_back(static_cast<char>(px[0]));
                fingerprint.push_back(static_cast<char>(px[1]));
                fingerprint.push_back(static_cast<char>(px[2]));
            }
        }
        if (fingerprint == last_fingerprint) {
            break;
        }
        last_fingerprint = fingerprint;

        const int max_row = rows.empty() ? 480 : *std::max_element(rows.begin(), rows.end());
        const int scroll = std::max(170, std::min(560, max_row - 300));
        const int distance = static_cast<int>(std::lround(static_cast<double>(scroll) / 0.92));
        drag_segments(panel_x + 790.0, 950.0, panel_x + 790.0, 950.0 - distance, 14, 30, 500);
        sleep_ms(600);
        auto next = frame();
        if (!next.has_value()) {
            return false;
        }
        info = *next;
    }

    // 列表翻到底：task 目标允许直接沿用当前策略
    if (by_task && skipped_by_goal) {
        auto f = frame();
        if (f.has_value() && f->screen == ScreenState::Strategy) {
            const std::string picked = strategy_from_panel(*f);
            if (!picked.empty() && has_text_regex(f->texts, "^确认选择$")) {
                m_current_strategy = picked;
                log_info("选择 " + picked + " · 未找到对应盟约的策略，使用当前策略");
                if (auto hit = find_text_regex(f->texts, "^确认选择$")) {
                    tap(hit->cx, hit->cy, 350);
                }
                wait_for(
                    [](const ScreenInfo& s) {
                        return s.screen == ScreenState::Squad || s.screen == ScreenState::Prep;
                    },
                    15000);
                return true;
            }
        }
    }

    // 是否全部达标
    bool all_done = true;
    for (const auto& sr : m_strategies) {
        if (!sr.available) {
            continue;
        }
        const int p = m_strategy_clears.count(sr.name) ? m_strategy_clears[sr.name] : 0;
        if (p < target) {
            all_done = false;
            break;
        }
    }
    if (all_done) {
        log_info("所有极速模式可用策略均已获得刻章");
        return false;
    }
    log_info("已到策略列表末尾，但尚有策略进度未核实；请检查锁定策略或识别结果");
    return false;
}

std::string SPAFarmTaskPlugin::strategy_from_panel(const ScreenInfo& info) const
{
    // `mt(I)` —— service.pretty.mjs L8598-8606
    //   取 x>1450 && 500<y<640 的文本，拼起来去掉空白与 “”·
    //   再做 12 个字的繁→简替换，然后找「名字包含于该串」的策略
    std::string joined;
    for (const auto& it : info.texts) {
        if (it.x > 1450 && it.y > 500 && it.y < 640) {
            joined += it.text;
        }
    }
    auto clean = [](std::string_view sv) {
        std::string out;
        for (const auto& ch : split_utf8_chars(sv)) {
            if (ch == "\u201c" || ch == "\u201d" || ch == "·" || ch == " ") {
                continue;
            }
            if (ch.size() == 1 && (ch[0] == ' ' || ch[0] == '\t' || ch[0] == '\n' || ch[0] == '\r')) {
                continue;
            }
            out += ch;
        }
        return out;
    };
    // JS 只替换这 12 个常见繁体字
    auto to_simplified = [](std::string s) {
        static const std::vector<std::pair<std::string_view, std::string_view>> kMap = {
            { "傑", "杰" }, { "奧", "奥" }, { "維", "维" }, { "絲", "丝" }, { "羅", "罗" }, { "麗", "丽" },
            { "亞", "亚" }, { "爾", "尔" }, { "諾", "诺" }, { "達", "达" }, { "馬", "马" },
        };
        for (const auto& [from, to] : kMap) {
            size_t pos = 0;
            while ((pos = s.find(from, pos)) != std::string::npos) {
                s.replace(pos, from.size(), to);
                pos += to.size();
            }
        }
        return s;
    };

    const std::string panel = to_simplified(clean(joined));
    for (const auto& sr : m_strategies) {
        // medal 目标只在 available 里找；task 目标在全部策略里找
        if (m_goal == "medal" && !sr.available) {
            continue;
        }
        const std::string name = to_simplified(clean(sr.name));
        if (!name.empty() && panel.find(name) != std::string::npos) {
            return sr.name;
        }
    }
    return {};
}

void SPAFarmTaskPlugin::handle_squad_screen(const cv::Mat& img, const std::vector<OcrItem>& items)
{
    // `squad` 分支 —— service.pretty.mjs L10420-10423
    //   `if (D.screen === "squad") { await W(D, /准备就绪/), await pe(3e3); continue }`
    //   自选编队页只需要点「准备就绪」，没有任何干员打分/禁用位处理。
    (void)img;
    if (auto ready = find_text_regex(items, "准备就绪")) {
        tap(ready->cx, ready->cy, static_cast<int>(std::lround(3000.0 * m_step_scale)));
    }
    else if (auto ready2 = find_text_regex(items, "^准备就绪$")) {
        tap(ready2->cx, ready2->cy, static_cast<int>(std::lround(3000.0 * m_step_scale)));
    }
    else {
        log_info("自选编队页没找到「准备就绪」");
    }
}

void SPAFarmTaskPlugin::handle_info_screen(const cv::Mat& img, const std::vector<OcrItem>& items)
{
    // `ye` L10340-10358 的 info 分支 + `ln()`（L9627-9651）
    // 说明：`ln` 在读取完本局信息后调用，用于把「核心盟约」切到目标盟约更多的分组。
    m_match_info_read = true;
    if (auto info = read_info_screen(img)) {
        // `l`（本局已激活盟约）：只用于筛选「当前可用的聚焦盟约」，
        // 不覆盖 `m_active_core_bonds`（那是 goal 决定的聚焦盟约 `st`，运行期不变）。
        m_active_bond_names.clear();
        std::vector<std::string> active_cores;
        for (const auto& b : info->bonds) {
            if (b.active) {
                m_active_bond_names.push_back(b.name);
            }
            if (b.core && b.active) {
                active_cores.push_back(b.name);
            }
        }
        std::string bond_str;
        for (const auto& cb : m_active_core_bonds) {
            if (!bond_str.empty()) {
                bond_str += "/";
            }
            bond_str += cb;
        }
        std::string active_str;
        for (const auto& ab : m_active_bond_names) {
            if (!active_str.empty()) {
                active_str += "/";
            }
            active_str += ab;
        }
        if (!info->boss.empty()) {
            m_current_boss = info->boss;
        }
        log_info(
            "界面: 确认本局信息 (主抓核心盟约: " + bond_str
            + " | 已激活: " + (active_str.empty() ? "无" : active_str)
            + (info->boss.empty() ? "" : (" | 敌方领袖: " + info->boss)) + ")");
    }
    else {
        log_info("界面: 确认本局信息 -> 点击右下角准备就绪");
    }

    // CRITICAL FIX: `"确认本局信息"` is the top header title! Never click `"确认本局信息"`.
    // In service.pretty.mjs lines 10340-10358:
    // - If `准备就绪` button is visible, click `准备就绪` (bottom-right `(1686, 986)`).
    // - If `已就绪` is visible, wait 500ms.
    // - If `确认本局信息` title is present (pre-match confirmation screen), click `(1686, 986)`.
    // - Otherwise (opened via `本局信息` overlay), close via top-right `(1805, 65)`.
    if (auto ready_btn = find_ocr(items, { "准备就绪" })) {
        tap(ready_btn->cx, ready_btn->cy, 1500);
    }
    else if (has_ocr(items, { "已就绪" })) {
        sleep_ms(500);
    }
    else if (has_ocr(items, { "确认本局信息" })) {
        tap(1686, 986, 1500);
    }
    else {
        tap(1805, 65, 700);
    }

    // `be() && await ln()`：task 目标才切核心盟约分组
    if (m_goal != "medal") {
        ScreenInfo info_state = classify_screen(img);
        switch_core_bond_group(info_state);
    }
}

void SPAFarmTaskPlugin::handle_three_choice_screen(const cv::Mat& img, const std::vector<OcrItem>& items)
{
    // `K(I)` —— service.pretty.mjs L9679-9697
    //   三张卡（x 中心 475/990/1500）：收集 380<y<720 且 |cx-中心|<260 的文本，按 y 再 x 排序拼接
    //   命中装备名 → 按分类打分（output/stack=3, utility=2, defense=1, debuff=-1）
    //   取最高分 → 点 (cx, 540)
    (void)img;
    static constexpr int kCenters[3] = { 475, 990, 1500 };
    static const std::unordered_map<std::string, int> kScore = {
        { "output", 3 }, { "stack", 3 }, { "utility", 2 }, { "defense", 1 }, { "debuff", -1 },
    };

    struct Card
    {
        int cx = 0;
        std::string text;
        int score = 0;
    };
    std::vector<Card> cards;
    cards.reserve(3);

    for (int center : kCenters) {
        std::vector<const OcrItem*> picked;
        for (const auto& it : items) {
            if (it.y <= 380 || it.y >= 720) {
                continue;
            }
            if (std::abs(it.cx - center) >= 260) {
                continue;
            }
            picked.push_back(&it);
        }
        std::sort(picked.begin(), picked.end(), [](const OcrItem* a, const OcrItem* b) {
            if (a->y != b->y) {
                return a->y < b->y;
            }
            return a->x < b->x;
        });
        std::string text;
        for (const auto* it : picked) {
            text += strip_ws(it->text);
        }

        int score = 0;
        for (const auto& item : m_items) {
            if (!item.name.empty() && text.find(item.name) != std::string::npos) {
                const std::string cat = classify_item_category(item.name, item.effect);
                auto sit = kScore.find(cat);
                score = (sit != kScore.end()) ? sit->second : 0;
                break;
            }
        }
        cards.push_back(Card { center, text, score });
    }

    if (cards.empty()) {
        return;
    }
    std::stable_sort(cards.begin(), cards.end(), [](const Card& a, const Card& b) {
        return a.score > b.score;
    });
    const Card& best = cards.front();
    log_info("三选一：选「" + best.text.substr(0, 48) + "」");
    tap(best.cx, 540, 1000);
}

void SPAFarmTaskPlugin::handle_decision_screen(const cv::Mat& img, const std::vector<OcrItem>& items)
{
    // `ce(I)` —— service.pretty.mjs L9698-9721
    //   三张卡分别打分：
    //     含【目标盟约】且有「层数+N」 → 100 + N
    //     含「资金」            → 50 + 「获得N资金」(缺省 1)
    //     含「层数」            → 10
    //   取最高 → 点 (cx, 540) → 若还有「战术决策」+「确认」则再点确认
    (void)img;
    static constexpr int kCenters[3] = { 475, 990, 1500 };

    auto bracketed = [](const std::string& s) {
        std::vector<std::string> out;
        const std::string open = "【";
        const std::string close = "】";
        size_t pos = 0;
        while (true) {
            const size_t a = s.find(open, pos);
            if (a == std::string::npos) {
                break;
            }
            const size_t b = s.find(close, a + open.size());
            if (b == std::string::npos) {
                break;
            }
            out.push_back(s.substr(a + open.size(), b - (a + open.size())));
            pos = b + close.size();
        }
        return out;
    };

    struct Card
    {
        int cx = 0;
        std::string text;
        int score = 0;
    };
    std::vector<Card> cards;
    cards.reserve(3);

    for (int center : kCenters) {
        std::vector<const OcrItem*> picked;
        for (const auto& it : items) {
            if (it.y <= 380 || it.y >= 720) {
                continue;
            }
            if (std::abs(it.cx - center) >= 260) {
                continue;
            }
            picked.push_back(&it);
        }
        std::sort(picked.begin(), picked.end(), [](const OcrItem* a, const OcrItem* b) {
            if (a->y != b->y) {
                return a->y < b->y;
            }
            return a->x < b->x;
        });
        std::string text;
        for (const auto* it : picked) {
            text += strip_ws(it->text);
        }

        const auto names = bracketed(text);
        int layers = 0;
        if (auto d = regex_first_group(text, "层数\\+([0-9]+)")) {
            layers = std::stoi(*d);
        }
        const bool hits_focus = std::any_of(names.begin(), names.end(), [&](const std::string& n) {
            return std::find(m_active_core_bonds.begin(), m_active_core_bonds.end(), n) != m_active_core_bonds.end();
        });

        int score = 0;
        if (hits_focus && layers > 0) {
            score = 100 + layers;
        }
        else if (text.find("资金") != std::string::npos) {
            int gain = 1;
            if (auto d = regex_first_group(text, "获得([0-9]+)资金")) {
                gain = std::stoi(*d);
            }
            score = 50 + gain;
        }
        else if (text.find("层数") != std::string::npos) {
            score = 10;
        }
        cards.push_back(Card { center, text, score });
    }

    if (cards.empty()) {
        return;
    }
    std::stable_sort(cards.begin(), cards.end(), [](const Card& a, const Card& b) {
        return a.score > b.score;
    });
    const Card& best = cards.front();
    log_info("战术决策：选「" + best.text.substr(0, 72) + "」");
    tap(best.cx, 540, 800);

    cv::Mat after = shot();
    if (after.empty()) {
        return;
    }
    auto after_items = ocr(after);
    auto confirm = find_text_regex(after_items, "^确认");
    if (confirm.has_value() && has_text_regex(after_items, "战术决策")) {
        tap(confirm->cx, confirm->cy, 800);
    }
    sleep_ms(800);
}

// ============================================================================
// Main Run Loop (`_run`, 1:1 with `ye` in service.pretty.mjs lines 10256-10530)
// ============================================================================

std::vector<int> SPAFarmTaskPlugin::detect_strategy_rows(const cv::Mat& img, bool selected) const
{
    // `Ft(I)` —— service.pretty.mjs L8683-8719（`pc` 恒 false，不走 `Or`）
    //   k = selected ? 586 : 982
    //   对 x ∈ [160,1050) 竖扫 y ∈ [k+218, k+390)：R∈(27,180) 且 max-min<16 的像素 > 100 → 记列
    //   连续列分段（间隙 >2 断开），每段取中点；只保留 x<850 且右侧 175..200 内有邻居的
    //   再剔除同簇（|dx|<40）里 186 偏差更大的
    std::vector<int> rows;
    if (img.empty()) {
        return rows;
    }
    const RawRgb raw = img_to_rgb(img);
    if (raw.width == 0) {
        return rows;
    }
    const int k = selected ? 586 : 982;

    std::vector<int> cols;
    for (int x = 160; x < 1050; ++x) {
        int v = 0;
        for (int ie = 218; ie < 390; ++ie) {
            const int yy = k + ie;
            if (yy < 0 || yy >= raw.height) {
                continue;
            }
            const size_t idx = (static_cast<size_t>(yy) * raw.width + static_cast<size_t>(x)) * 3;
            const int R = raw.data[idx];
            const int G = raw.data[idx + 1];
            const int B = raw.data[idx + 2];
            const int mx = std::max({ R, G, B });
            const int mn = std::min({ R, G, B });
            if (R > 27 && R < 180 && (mx - mn) < 16) {
                ++v;
            }
        }
        if (v > 100) {
            cols.push_back(x);
        }
    }

    std::vector<std::pair<int, int>> segs;
    for (int x : cols) {
        if (segs.empty() || x - segs.back().second > 2) {
            segs.emplace_back(x, x);
        }
        else {
            segs.back().second = x;
        }
    }

    std::vector<int> centers;
    centers.reserve(segs.size());
    for (const auto& [a, b] : segs) {
        centers.push_back(static_cast<int>(std::lround((a + b) / 2.0)));
    }

    auto tail_gap = [&](int x) -> double {
        double best = std::numeric_limits<double>::infinity();
        for (int v : centers) {
            const int d = v - x;
            if (d >= 175 && d <= 200) {
                best = std::min(best, static_cast<double>(std::abs(v - x - 186)));
            }
        }
        return best;
    };

    std::vector<int> cand;
    for (int x : centers) {
        if (x < 850 && std::isfinite(tail_gap(x))) {
            cand.push_back(x);
        }
    }
    for (int x : cand) {
        bool drop = false;
        for (int v : cand) {
            if (v == x || std::abs(v - x) >= 40) {
                continue;
            }
            if (tail_gap(v) < tail_gap(x) || (tail_gap(v) == tail_gap(x) && v > x)) {
                drop = true;
                break;
            }
        }
        if (!drop) {
            rows.push_back(x);
        }
    }
    return rows;
}

bool SPAFarmTaskPlugin::is_personal_selected(const cv::Mat& img, const std::vector<OcrItem>& items) const
{
    // `wi(a, s)` —— service.pretty.mjs L4753-4765
    //   Te(/^个人模拟$/) 且 score >= .75
    //   取 box 右侧 6px 处 8×round(h*0.5) 的竖条，统计“近白中性色”(min>150 && max-min<40) 占比 > .75
    auto hit = find_text_regex(items, "^个人模拟$");
    if (!hit.has_value() || hit->score < 0.75) {
        return false;
    }
    const int left = static_cast<int>(std::lround(static_cast<double>(hit->x + hit->w) + 6.0));
    const int top = static_cast<int>(std::lround(static_cast<double>(hit->y) + hit->h * 0.25));
    const int height = std::max(1, static_cast<int>(std::lround(static_cast<double>(hit->h) * 0.5)));
    if (left < 0 || top < 0 || left + 8 > img.cols || top + height > img.rows) {
        return false;
    }
    cv::Mat crop = img_crop(img, left, top, 8, height);
    if (crop.empty()) {
        return false;
    }
    const RawRgb raw = img_to_rgb(crop);
    int white = 0;
    const int total = 8 * height;
    for (size_t i = 0; i + 2 < raw.data.size(); i += 3) {
        const int R = raw.data[i];
        const int G = raw.data[i + 1];
        const int B = raw.data[i + 2];
        const int mn = std::min({ R, G, B });
        const int mx = std::max({ R, G, B });
        if (mn > 150 && (mx - mn) < 40) {
            ++white;
        }
    }
    return total > 0 && static_cast<double>(white) / total > 0.75;
}

void SPAFarmTaskPlugin::set_mode(const std::string& mode)
{
    // `ls(a)` L7946-7953：`_t.mode = a; _t.lastRound = a === "standard" ? 14 : 9`
    m_mode = (mode == "standard") ? "standard" : "quick";
    m_last_round = (m_mode == "standard") ? 14 : 9;
}

std::optional<SPAFarmTaskPlugin::ScreenInfo> SPAFarmTaskPlugin::wait_for(
    const std::function<bool(const ScreenInfo&)>& pred,
    int timeout_ms)
{
    // `V(I, k = 1e4)`：每 200ms 抓一帧分类，超时返回 null
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline && !need_exit()) {
        cv::Mat img = shot();
        if (!img.empty()) {
            ScreenInfo info = classify_screen(img);
            if (pred(info)) {
                return info;
            }
        }
        sleep_ms(200);
    }
    return std::nullopt;
}

bool SPAFarmTaskPlugin::handle_lobby_quick(ScreenInfo info)
{
    // `ys(a, s)` —— service.pretty.mjs L8415-8427
    if (info.screen == ScreenState::Lobby) {
        if (!info.personal_selected && !info.quick) {
            // 必须先能确认「同盟模拟 + 搜寻队友」才敢切个人模拟
            bool union_tab = false;
            bool search_team = false;
            for (const auto& it : info.texts) {
                if (it.score >= 0.75 && regex_test(strip_ws(it.text), "^同盟模拟$")) {
                    union_tab = true;
                }
                if (it.score >= 0.75 && regex_test(strip_ws(it.text), "^搜寻队友$")) {
                    search_team = true;
                }
            }
            if (!(union_tab && search_team)) {
                log_info("无法确认个人/联合模拟，等待确认后再切换");
                return false;
            }
            log_info("活动主页 · 从联合切换个人模拟");
            if (auto hit = find_text_regex(info.texts, "^个人模拟$")) {
                tap(hit->cx, hit->cy, 350);
            }
        }
        auto lobby = wait_for(
            [](const ScreenInfo& s) {
                return s.screen == ScreenState::Lobby && s.personal_selected && s.quick;
            },
            15000);
        if (!lobby.has_value()) {
            log_info("个人模拟确认超时");
            return false;
        }
        log_info("个人模拟已确认 · 进入极速模拟");
        if (auto hit = find_text_regex(lobby->texts, "^极速模拟$")) {
            tap(hit->cx, hit->cy, 350);
        }
        auto next = wait_for(
            [](const ScreenInfo& s) {
                return s.quick
                    && (s.screen == ScreenState::Mode || s.screen == ScreenState::Info
                        || s.screen == ScreenState::Strategy);
            },
            15000);
        if (!next.has_value()) {
            log_info("极速模拟页面切换超时");
            return false;
        }
        info = *next;
    }

    if (info.screen == ScreenState::Mode) {
        if (!info.quick) {
            log_info("当前模式不是极速模拟，等待正确模式后继续");
            return false;
        }
        log_info("极速模拟已确认 · 开始模拟");
        if (auto hit = find_text_regex(info.texts, "^开始模拟$")) {
            tap(hit->cx, hit->cy, 350);
        }
        wait_for(
            [](const ScreenInfo& s) {
                return s.quick && (s.screen == ScreenState::Info || s.screen == ScreenState::Strategy);
            },
            20000);
    }
    return true;
}

bool SPAFarmTaskPlugin::handle_lobby_standard(ScreenInfo info, const std::string& level_name)
{
    // `xs(a, s, e = "规程等级1")` —— service.pretty.mjs L8429-8448
    if (info.screen == ScreenState::Lobby) {
        if (!info.personal_selected) {
            bool union_tab = false;
            bool search_team = false;
            for (const auto& it : info.texts) {
                if (it.score >= 0.75 && regex_test(strip_ws(it.text), "^同盟模拟$")) {
                    union_tab = true;
                }
                if (it.score >= 0.75 && regex_test(strip_ws(it.text), "^搜寻队友$")) {
                    search_team = true;
                }
            }
            if (!(union_tab && search_team)) {
                log_info("无法确认个人/联合模拟，等待确认后再切换");
                return false;
            }
            log_info("活动主页 · 从联合切换个人模拟");
            if (auto hit = find_text_regex(info.texts, "^个人模拟$")) {
                tap(hit->cx, hit->cy, 350);
            }
        }
        auto lobby = wait_for(
            [](const ScreenInfo& s) {
                return s.screen == ScreenState::Lobby && s.personal_selected;
            },
            15000);
        if (!lobby.has_value()) {
            log_info("个人模拟确认超时");
            return false;
        }
        log_info("个人模拟已确认 · 进入独立模拟");
        if (auto hit = find_text_regex(lobby->texts, "^独立模拟$")) {
            tap(hit->cx, hit->cy, 350);
        }
        auto next = wait_for(
            [](const ScreenInfo& s) {
                return s.screen == ScreenState::Mode || s.screen == ScreenState::Info
                    || s.screen == ScreenState::Strategy;
            },
            15000);
        if (!next.has_value()) {
            log_info("独立模拟页面切换超时");
            return false;
        }
        info = *next;
    }

    if (info.screen == ScreenState::Mode) {
        const int target = [&]() {
            if (auto d = regex_first_group(level_name, "([0-9]+)")) {
                return std::stoi(*d);
            }
            return 1;
        }();

        for (int i = 0; i < 12 && info.levelName != level_name; ++i) {
            const int cur = [&]() {
                if (auto d = regex_first_group(info.levelName, "([0-9]+)")) {
                    return std::stoi(*d);
                }
                return 99;
            }();
            const int steps = std::min(4, std::max(1, std::abs(cur - target)));
            if (cur > target) {
                drag_segments(1650, 820, 1650, 820 - 200 * steps, 14, 25, 300);
            }
            else {
                drag_segments(1650, 300, 1650, 300 + 200 * steps, 14, 25, 300);
            }
            sleep_ms(700);
            cv::Mat img = shot();
            if (img.empty()) {
                return false;
            }
            info = classify_screen(img);
        }
        if (info.levelName != level_name) {
            log_info("难度页没能选中【" + level_name + "】（当前 " + (info.levelName.empty() ? "未识别" : info.levelName) + "）");
            return false;
        }
        log_info(level_name + "已确认 · 开始模拟");
        if (auto hit = find_text_regex(info.texts, "^开始模拟$")) {
            tap(hit->cx, hit->cy, 350);
        }
        wait_for(
            [](const ScreenInfo& s) {
                return s.quick && (s.screen == ScreenState::Info || s.screen == ScreenState::Strategy);
            },
            20000);
    }
    return true;
}

void SPAFarmTaskPlugin::switch_core_bond_group(const ScreenInfo& info)
{
    // `ln()` —— service.pretty.mjs L9627-9651
    //   在本局信息页找「核心盟约」里属于目标盟约(st)的那几个，
    //   统计左（cx<680）/右两列哪个多，若该列还有未激活的，就点该列上方切换分组
    if (info.screen != ScreenState::Info) {
        return;
    }
    std::set<std::string> core_names;
    for (const auto& b : m_bonds) {
        if (b.is_core) {
            core_names.insert(b.name);
        }
    }

    struct Hit
    {
        const OcrItem* it = nullptr;
        std::string name;
    };
    std::vector<Hit> hits;
    for (const auto& it : info.texts) {
        if (it.x >= 1340 || it.y <= 380 || it.y >= 740) {
            continue;
        }
        const std::string name = strip_ws(it.text);
        if (core_names.count(name) > 0) {
            hits.push_back(Hit { &it, name });
        }
    }

    std::vector<Hit> focus;
    for (const auto& h : hits) {
        if (std::find(m_active_core_bonds.begin(), m_active_core_bonds.end(), h.name)
            != m_active_core_bonds.end()) {
            focus.push_back(h);
        }
    }
    if (focus.empty()) {
        return;
    }

    auto center_x = [](const Hit& h) {
        return h.it->x + h.it->w * 0.5;
    };
    auto side_of = [&](const Hit& h) {
        return center_x(h) < 680.0 ? std::string("left") : std::string("right");
    };
    const int left_cnt = static_cast<int>(std::count_if(focus.begin(), focus.end(), [&](const Hit& h) {
        return side_of(h) == "left";
    }));
    const int right_cnt = static_cast<int>(focus.size()) - left_cnt;
    const std::string side = (right_cnt > left_cnt) ? "right" : "left";

    auto is_active = [&](const std::string& name) {
        auto it = m_active_bond_names.end();
        it = std::find(m_active_bond_names.begin(), m_active_bond_names.end(), name);
        return it != m_active_bond_names.end();
    };

    std::vector<Hit> on_side;
    for (const auto& h : focus) {
        if (side_of(h) == side) {
            on_side.push_back(h);
        }
    }
    if (on_side.empty()) {
        return;
    }
    if (std::all_of(on_side.begin(), on_side.end(), [&](const Hit& h) {
            return is_active(h.name);
        })) {
        return;
    }

    std::vector<Hit> all_side;
    for (const auto& h : hits) {
        if (side_of(h) == side) {
            all_side.push_back(h);
        }
    }
    if (all_side.empty()) {
        return;
    }
    double sum_x = 0.0;
    double sum_y = 0.0;
    for (const auto& h : all_side) {
        sum_x += center_x(h);
        sum_y += h.it->y;
    }
    const double click_x = sum_x / all_side.size();
    const double click_y = sum_y / all_side.size() - 70.0;
    tap(click_x, click_y, 600);

    cv::Mat after = shot();
    if (after.empty()) {
        return;
    }
    ScreenInfo next = classify_screen(after);
    // 重新读本局信息，更新已激活盟约
    if (auto data = read_info_screen(after)) {
        m_active_bond_names.clear();
        for (const auto& b : data->bonds) {
            if (b.active) {
                m_active_bond_names.push_back(b.name);
            }
        }
    }
    const bool all_now_active = std::all_of(on_side.begin(), on_side.end(), [&](const Hit& h) {
        return is_active(h.name);
    });
    std::string names;
    for (const auto& h : on_side) {
        if (!names.empty()) {
            names += "、";
        }
        names += h.name;
    }
    if (all_now_active) {
        log_info("核心盟约改选" + std::string(side == "left" ? "左" : "右") + "组（含 " + names + "）");
    }
    else {
        log_info("核心盟约改选" + std::string(side == "left" ? "左" : "右") + "组后仍未确认 " + names
                 + " 可用，请检查本局信息页");
    }
    (void)next;
}

bool SPAFarmTaskPlugin::_run()
{
    LogTraceFunction;
    load_all_data();
    reset_match_state();
    // 数据到位后重算 `ht`（`en(goal)` 的 stackers 依赖 chess.traits）
    rebuild_stacker_names();
    log_info(
        "启动卫戍协议自动刷章 | 目标:" + m_goal + " | 配置:" + m_profile
        + " | 目标通关:" + std::to_string(m_target) + " | 局数上限:" + std::to_string(m_runs));

    // 说明：本函数是 `ye(I)`（service.pretty.mjs L10230-10460）的接线骨架。
    // 各页面处理函数（`ys`/`Ot`/`je`/`K`/`ce`/`ln`）在 M2 内逐个替换为逐行实现，
    // 全部就位后再按 `ye` 的完整分支顺序（含中途接管 `l` 标志）收口一次。
    double last_garrison_seen = now_sec();
    double last_progress = now_sec();
    bool result_counted = false;
    std::string stop_reason = "已达到设定通关局数上限";
    bool finished_ok = true;

    while (!need_exit()) {
        cv::Mat img = shot();
        if (img.empty()) {
            sleep_ms(1000);
            continue;
        }
        ScreenInfo info = classify_screen(img);
        const auto& items = info.texts;

        // ---- 全局浮层（顺序与 `ye` L10256-10319 一致）----
        if (has_text_regex(items, "暂时离开模拟")) {
            if (auto hit = find_text_regex(items, "^取消$")) {
                tap(hit->cx, hit->cy, 800);
            }
            else {
                tap(480, 727, 800);
            }
            log_info("界面: 暂时离开模拟 -> 取消");
            continue;
        }
        if (has_text_regex(items, "敌方领袖") && has_text_regex(items, "核心盟约")
            && info.screen == ScreenState::Unknown) {
            tap(1805, 65, 600);
            continue;
        }
        if (has_text_regex(items, "数据已更新|即将进行同步")) {
            tap(960, 738, 3000);
            log_info("界面: 游戏数据同步 -> 确认");
            continue;
        }
        if (has_text_regex(items, "今日配给")) {
            tap(960, 960, 1500);
            continue;
        }
        if (has_text_regex(items, "签到|许愿墙|活动公告|系统公告")
            && !has_text_regex(items, "个人模拟|休息一下")) {
            static constexpr int kClose[4][2] = { { 1822, 104 }, { 1868, 152 }, { 1848, 126 }, { 1808, 106 } };
            for (const auto& xy : kClose) {
                tap(xy[0], xy[1], 900);
                cv::Mat after = shot();
                if (!after.empty()
                    && !has_text_regex(ocr(after), "签到|许愿墙|活动公告|系统公告")) {
                    break;
                }
            }
            continue;
        }
        if (has_text_regex(items, "^终端$") && has_text_regex(items, "理智")) {
            tap(1750, 215, 4000);
            log_info("界面: 游戏主页 -> 从 Banner 进入卫戍协议");
            continue;
        }
        if (info.screen == ScreenState::Lobby && has_text_regex(items, "断线重连")) {
            if (auto hit = find_text_regex(items, "断线重连")) {
                tap(hit->cx, hit->cy, 6000);
            }
            continue;
        }
        if (has_text_regex(items, "装备已满，请选择|销毁并替换")) {
            tap(1846, 284, 800);
            continue;
        }
        if (has_text_regex(items, "选定后将无法更改")) {
            if (auto hit = find_text_regex(items, "^确认选择$")) {
                tap(hit->cx, hit->cy, 1200);
            }
            else {
                handle_three_choice_screen(img, items);
            }
            continue;
        }

        if (info.screen != ScreenState::Unknown) {
            last_garrison_seen = now_sec();
        }
        else if (now_sec() - last_garrison_seen > kLeaveTimeoutSec) {
            stop_reason = "连续 30 秒未检测到卫戍协议界面，自动停止";
            finished_ok = false;
            break;
        }

        switch (info.screen) {
        case ScreenState::Lobby:
        case ScreenState::Mode: {
            // `ye` L10321：`X.mode === "standard" ? xs(...) : ys(...)`
            const bool ok = (m_mode == "standard") ? handle_lobby_standard(info) : handle_lobby_quick(info);
            if (!ok) {
                sleep_ms(600);
            }
            last_progress = now_sec();
            break;
        }
        case ScreenState::Info: {
            handle_info_screen(img, items);
            if (auto hit = find_text_regex(items, "准备就绪")) {
                tap(hit->cx, hit->cy, 800);
            }
            result_counted = false;
            last_progress = now_sec();
            break;
        }
        case ScreenState::Strategy: {
            if (handle_strategy_screen(img)) {
                last_progress = now_sec();
            }
            else {
                stop_reason = "全部可用策略均已达成 " + std::to_string(m_target) + " 次通关刻章目标";
                finished_ok = true;
                report_task_summary(stop_reason, finished_ok);
                return true;
            }
            break;
        }
        case ScreenState::Squad: {
            handle_squad_screen(img, items);
            last_progress = now_sec();
            break;
        }
        case ScreenState::ConfirmReady: {
            // `confirm-ready` L10369-10372：本局不再提示 + 确认
            if (auto no_prompt = find_text_regex(items, "本局不再提示")) {
                tap(no_prompt->x - 20, no_prompt->y + no_prompt->h * 0.5, 250);
            }
            if (auto hit = find_text_regex(items, "^确认$")) {
                tap(hit->cx, hit->cy, 1200);
            }
            break;
        }
        case ScreenState::Prep: {
            auto [rnd, is_prep] = read_round_and_prep(img, items);
            (void)is_prep;
            if (rnd.has_value()) {
                m_current_round = *rnd;
            }
            auto cam = detect_camera(img, goal_only_allowed());
            run_prep_phase(img, cam);
            last_progress = now_sec();
            break;
        }
        case ScreenState::Battle: {
            auto [rnd, is_prep] = read_round_and_prep(img, items);
            (void)is_prep;
            if (rnd.has_value() && *rnd != m_current_round) {
                m_current_round = *rnd;
                last_progress = now_sec();
            }
            sleep_ms(2000);
            break;
        }
        case ScreenState::Result: {
            // `result` L10435-10455：每局只记一次，然后返回主页
            if (!result_counted) {
                result_counted = true;
                report_match_finished(info.won.value_or(false), m_current_round);
            }
            if (auto hit = find_text_regex(items, "返回主页")) {
                tap(hit->cx, hit->cy, 2000);
            }
            else {
                sleep_ms(1500);
            }
            break;
        }
        case ScreenState::Unknown:
        default: {
            if (now_sec() - last_progress > kStallTimeoutSec) {
                stop_reason = "超过 10 分钟无实质进展，触发防卡死保护";
                finished_ok = false;
                report_task_summary(stop_reason, finished_ok);
                return false;
            }
            sleep_ms(1200);
            break;
        }
        }

        if (m_runs_completed >= m_runs) {
            break;
        }
    }

    if (need_exit()) {
        stop_reason = "收到用户/上层停止指令";
    }
    report_task_summary(stop_reason, finished_ok);
    return finished_ok;
}

} // namespace asst
