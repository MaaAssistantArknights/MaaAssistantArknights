#include "KarlanBoardAnalyzer.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <functional>
#include <string_view>

#include "Config/TemplResource.h"
#include "Vision/OCRer.h"

namespace asst::karlan
{
// Layout and icon strips use the same 960 x 540 coordinate space.
static cv::Mat gray_template(const std::string& name)
{
    const auto& source = TemplResource::get_instance().get_templ(
        (name == "Combo" ? "MiniGame@OS@" : "MiniGame/OS/MiniGame@OS@") + name + ".png");
    cv::Mat gray;
    if (!source.empty()) {
        cv::cvtColor(source, gray, cv::COLOR_BGR2GRAY);
    }
    return gray;
}

static double match(const cv::Mat& region, const cv::Mat& templ)
{
    if (templ.empty() || region.cols < templ.cols || region.rows < templ.rows) {
        return -1;
    }
    cv::Mat scores;
    cv::matchTemplate(region, templ, scores, cv::TM_CCOEFF_NORMED);
    double score = 0;
    cv::minMaxLoc(scores, nullptr, &score);
    return score;
}

BoardAnalyzer::BoardAnalyzer(const cv::Mat& image) :
    m_image(image)
{
    if (image.empty() || image.cols * 9 != image.rows * 16) {
        return;
    }
    cv::resize(image, m_color, { 960, 540 }, 0, 0, cv::INTER_AREA);
    cv::cvtColor(m_color, m_gray, cv::COLOR_BGR2GRAY);
}

std::optional<int> BoardAnalyzer::stage() const
{
    const auto icons = gray_template("StageIcons");
    if (m_gray.empty() || icons.size() != cv::Size(54, 240)) {
        return std::nullopt;
    }
    double best = 0, runner_up = 0;
    int result = 0;
    for (int i = 0; i < 10; ++i) {
        const double icon = match(m_gray({ 474, 2, 31, 28 }), icons({ 0, 24 * i, 27, 24 }));
        const double goal = match(m_gray({ 886, 19, 31, 28 }), icons({ 27, 24 * i, 27, 24 }));
        if (icon < 0.75 || goal < 0.76) {
            continue;
        }
        const double quality = (icon + goal) / 2;
        if (quality > best) {
            runner_up = best;
            best = quality;
            result = i + 1;
        }
        else {
            runner_up = std::max(runner_up, quality);
        }
    }
    return best - runner_up > 0.025 ? std::optional(result) : std::nullopt;
}

bool BoardAnalyzer::rules_visible() const
{
    if (m_gray.empty()) {
        return false;
    }
    const auto top = cv::mean(m_gray({ 365, 80, 215, 6 }))[0];
    return top > 235 && top - cv::mean(m_gray({ 365, 71, 215, 5 }))[0] > 24;
}

int BoardAnalyzer::slot_x(int slot)
{
    return cvRound(62 + 73.6 * slot);
}

bool BoardAnalyzer::selected(int slot) const
{
    return !m_gray.empty() && cv::mean(m_gray({ slot_x(slot) - 15, 479, 31, 5 }))[0] > 162;
}

std::vector<int> BoardAnalyzer::hand() const
{
    std::vector<int> cards;
    const auto digits = gray_template("CardDigits");
    if (m_gray.empty() || digits.size() != cv::Size(95, 34)) {
        return cards;
    }
    for (int slot = 0; slot < 8; ++slot) {
        const int x = slot_x(slot) - 15;
        cv::Mat roi;
        if (selected(slot)) {
            cv::bitwise_not(m_gray({ x, 425, 32, 49 }), roi);
        }
        else {
            roi = m_gray({ x, 438, 32, 46 });
        }
        double best = 0.73;
        int card = 0;
        for (int n = 0; n < 5; ++n) {
            const double score = match(roi, digits({ n * 19, 0, 19, 34 }));
            if (score > best) {
                best = score;
                card = n + 1;
            }
        }
        if (card == 0) {
            break;
        }
        cards.emplace_back(card);
    }
    return cards;
}

struct Node
{
    Tile tile;
    cv::Point point;
    float score;
};

static std::vector<Node> map_nodes(const cv::Mat& gray, const cv::Mat& icons, const Rules& rules)
{
    const cv::Rect map { 150, 105, 575, 237 };
    std::vector<Node> candidates;
    for (const auto resource : rules.resources) {
        for (int locked = 0; locked <= static_cast<int>(rules.locks); ++locked) {
            cv::Mat scores, peaks;
            cv::matchTemplate(
                gray(map),
                icons({ 21 * static_cast<int>(resource), 21 * locked, 21, 21 }),
                scores,
                cv::TM_CCOEFF_NORMED);
            cv::dilate(scores, peaks, cv::Mat::ones(21, 21, CV_8U));
            std::vector<cv::Point> hits;
            cv::findNonZero((scores >= peaks) & (scores > 0.59), hits);
            for (const auto& p : hits) {
                candidates.push_back(
                    { { resource, locked != 0 }, p + map.tl() + cv::Point(10, 10), scores.at<float>(p) });
            }
        }
    }
    std::ranges::sort(candidates, std::greater<>(), &Node::score);
    std::vector<Node> nodes;
    for (const auto& candidate : candidates) {
        if (std::ranges::none_of(nodes, [&](const auto& n) { return cv::norm(n.point - candidate.point) <= 32; })) {
            nodes.emplace_back(candidate);
            if (nodes.size() == 10) {
                break;
            }
        }
    }
    if (nodes.size() != 10) {
        return {};
    }
    std::ranges::sort(nodes, {}, [](const auto& n) { return n.point.x; });
    const auto left = nodes.front().point, right = nodes.back().point;
    if (left.x == right.x) {
        return {};
    }
    const auto upper = [&](const Node& n) {
        const double line = left.y + (right.y - left.y) * (n.point.x - left.x) / static_cast<double>(right.x - left.x);
        return n.point.y < std::clamp(line, 210.0, 222.0);
    };
    // The route starts at the rightmost tile and follows the lower row to the left.
    std::vector<Node> route { nodes.back() };
    for (int i = 8; i > 0; --i) {
        if (!upper(nodes[i])) {
            route.emplace_back(nodes[i]);
        }
    }
    route.emplace_back(nodes.front());
    for (int i = 1; i < 9; ++i) {
        if (upper(nodes[i])) {
            route.emplace_back(nodes[i]);
        }
    }
    return route;
}

static double position_score(const cv::Mat& gray, cv::Point p)
{
    const auto digits = gray({ p.x - 9, p.y + 11, 19, 15 });
    cv::Mat sorted;
    cv::sort(digits.clone().reshape(1, 1), sorted, cv::SORT_ASCENDING);
    const auto surround = sorted.at<uchar>(0, static_cast<int>(0.65 * (sorted.cols - 1)));
    return cv::countNonZero(digits < 118) * std::max(0, surround - 172) / 70.0;
}

static std::string read_text(const cv::Mat& image, cv::Rect area)
{
    const double scale = image.cols / 960.0;
    const cv::Rect rect { cvRound(area.x * scale),
                          cvRound(area.y * scale),
                          cvRound((area.x + area.width) * scale) - cvRound(area.x * scale),
                          cvRound((area.y + area.height) * scale) - cvRound(area.y * scale) };
    cv::Mat enlarged;
    cv::resize(image(rect), enlarged, {}, 3.0 / scale, 3.0 / scale, cv::INTER_CUBIC);
    OCRer analyzer(enlarged);
    analyzer.set_without_det(true);
    analyzer.set_log_tracing(false);
    auto result = analyzer.analyze();
    return result && result->size() == 1 ? result->front().text : std::string();
}

static std::optional<int> number(std::string_view text)
{
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || error != std::errc() || end != text.data() + text.size() || value < 0 || value > 999) {
        return std::nullopt;
    }
    return value;
}

static std::optional<Orders> read_orders(
    const cv::Mat& image,
    const cv::Mat& gray,
    const cv::Mat& color,
    const cv::Mat& icons,
    const Rules& rule)
{
    Orders orders;
    for (int row = 0; row < 3; ++row) {
        const int y = 75 + 56 * row;
        for (int col = 0; col < 2; ++col) {
            const int x = col == 0 ? 750 : 835;
            const int bar_x = x + 21;
            const auto bar = color({ bar_x - 6, y + 5, 48, 10 });
            int longest = 0, present = 0;
            for (int dy = 0; dy < bar.rows; ++dy) {
                int blue = 0, dark = 0;
                for (int dx = 0; dx < bar.cols; ++dx) {
                    const auto& bgr = bar.at<cv::Vec3b>(dy, dx);
                    blue += bgr[0] - bgr[2] > 60 && bgr[0] - bgr[1] > 70;
                    dark += dx >= 6 && dx < 42 && (bgr[0] + bgr[1] + bgr[2]) < 3 * 200;
                }
                longest = std::max(longest, blue);
                present = std::max(present, dark);
            }
            if (present < 27) {
                continue;
            }
            const double fraction = longest / 44.0;
            const auto roi = gray({ x - 14, y - 11, 29, 29 });
            auto kind = Resource::Done;
            double best = match(roi, icons({ 105, 42, 21, 21 }));
            for (auto resource : rule.resources) {
                const double score = match(roi, icons({ 21 * static_cast<int>(resource), 42, 21, 21 }));
                if (score > best) {
                    kind = resource;
                    best = score;
                }
            }
            if (kind == Resource::Done && best > 0.76 && fraction > 0.94) {
                orders[row].push_back({ kind, 1, 1 });
                continue;
            }
            const int center = col == 0 ? 789 : 873;
            auto text = read_text(image, { center - 30, y - 8, 60, 15 });
            std::erase(text, ' ');
            const auto slash = text.find('/');
            if (slash == std::string::npos) {
                return std::nullopt;
            }
            const auto amount = number(std::string_view(text).substr(0, slash));
            const auto target = number(std::string_view(text).substr(slash + 1));
            if (!amount || !target || *target == 0 || *amount > *target ||
                std::abs(static_cast<double>(*amount) / *target - fraction) > 0.09 ||
                (*amount < *target && (kind == Resource::Done || best < 0.72))) {
                return std::nullopt;
            }
            orders[row].push_back({ kind, *amount, *target });
        }
        if (orders[row].empty()) {
            return std::nullopt;
        }
    }
    return orders;
}

std::optional<Board> BoardAnalyzer::read(const Rules& rule, std::optional<int> expected, int last_yield) const
{
    Board board;
    board.cards = hand();
    const auto icons = gray_template("ResourceIcons");
    if (board.cards.empty() || icons.size() != cv::Size(126, 63)) {
        return std::nullopt;
    }
    const auto nodes = map_nodes(m_gray, icons, rule);
    if (nodes.empty()) {
        return std::nullopt;
    }
    auto combo = gray_template("Combo");
    if (combo.empty()) {
        return std::nullopt;
    }
    cv::resize(combo, combo, {}, 0.75, 0.75, cv::INTER_AREA);
    for (size_t i = 0; i < board.cards.size(); ++i) {
        if (match(m_gray({ slot_x(static_cast<int>(i)) - 40, 326, 81, 85 }), combo) > 0.80) {
            board.combos.set(board.cards[i]);
        }
    }
    const auto chest = rule.chests ? gray_template("Chest") : cv::Mat();
    const auto boost = rule.double_yield ? gray_template("DoubleYield") : cv::Mat();
    std::array<double, 10> scores {};
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto p = nodes[i].point;
        scores[i] = position_score(m_gray, p);
        board.tiles[i] = nodes[i].tile;
        board.tiles[i].boosted = rule.double_yield && match(m_gray({ p.x + 7, p.y - 13, 40, 28 }), boost) > 0.82;
        board.chests[i] = rule.chests && match(m_gray({ p.x + 9, p.y - 15, 30, 33 }), chest) > 0.79;
    }
    const int observed = static_cast<int>(std::ranges::max_element(scores) - scores.begin());
    if (!expected && scores[observed] < 5) {
        return std::nullopt;
    }
    board.position = (!expected || (scores[observed] > 15 && scores[*expected] < 4)) ? observed : *expected;
    board.production = last_yield;
    if (rule.double_yield) {
        board.production = rule.initial_yield;
    }
    else if (observed == board.position && scores[observed] > 5 && !board.tiles[observed].locked) {
        const auto p = nodes[observed].point;
        if (const auto value = number(read_text(m_image, { p.x - 13, p.y + 11, 27, 19 }));
            value && *value > 0 && *value <= 32) {
            board.production = *value;
        }
    }
    const auto orders = read_orders(m_image, m_gray, m_color, icons, rule);
    if (!orders) {
        return std::nullopt;
    }
    board.orders = *orders;
    return board;
}
}
