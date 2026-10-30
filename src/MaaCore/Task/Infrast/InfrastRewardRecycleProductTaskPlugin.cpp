#include "InfrastRewardRecycleProductTaskPlugin.h"

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

bool asst::InfrastRewardRecycleProductTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    return msg == AsstMsg::SubTaskStart &&
           details.get("details", "task", std::string()) == "InfrastRewardRecycleProduct";
}

bool asst::InfrastRewardRecycleProductTaskPlugin::_run()
{
    LogTraceFunction;

    std::shared_ptr<cv::Mat> image_ptr = get_hit_image();
    if (!image_ptr || image_ptr->empty()) {
        LogWarn << __FUNCTION__ << "hit image is empty, skip recycle product recognition";
        return true;
    }
    const cv::Mat& image = *image_ptr;

    const auto centers = detect_icon_centers(image);
    if (centers.empty()) {
        LogWarn << __FUNCTION__ << "no item icon detected";
        return true;
    }

    cv::Mat body_mask = cv::Mat::zeros(TemplSize, TemplSize, CV_8UC1);
    cv::circle(body_mask, { TemplSize / 2, TemplSize / 2 }, BodyRadius, cv::Scalar(255), -1);

    std::vector<std::pair<std::string, cv::Mat>> templs;
    for (const auto& item_id : ItemData.get_ordered_material_item_id()) {
        const cv::Mat& origin = TemplResource::get_instance().get_templ(item_id);
        if (origin.empty()) {
            continue;
        }
        cv::Mat templ;
        cv::resize(origin, templ, cv::Size(TemplSize, TemplSize), 0, 0, cv::INTER_AREA);
        templ.setTo(cv::Scalar(0, 0, 0), ~body_mask);
        templs.emplace_back(item_id, std::move(templ));
    }

    const double threshold = Task.get<MatchTaskInfo>("InfrastRecycleProductItem")->templ_thresholds.front();

    std::vector<json::value> drops_vec;
    for (const auto& center : centers) {
        Rect cell_roi(
            center.x - TemplSize / 2 - CellPadding,
            center.y - TemplSize / 2 - CellPadding,
            TemplSize + CellPadding * 2,
            TemplSize + CellPadding * 2);

        std::string best_id;
        double best_score = 0;
        for (const auto& [item_id, templ] : templs) {
            Matcher matcher(image);
            matcher.set_task_info("InfrastRecycleProductItem");
            matcher.set_roi(cell_roi);
            matcher.set_templ(templ);
            matcher.set_threshold(0);
            const auto result_opt = matcher.analyze();
            if (result_opt && result_opt->score > best_score) {
                best_score = result_opt->score;
                best_id = item_id;
            }
        }
        if (best_id.empty() || best_score < threshold) {
            LogWarn << __FUNCTION__ << "| unknown item at" << center.x << center.y << "best:" << best_id
                    << "score:" << best_score;
            continue;
        }

        int quantity = match_quantity(image, center);
        const std::string& name = ItemData.get_item_name(best_id);
        LogInfo << __FUNCTION__ << "| item:" << best_id << "x" << quantity << "score:" << best_score;

        json::value info;
        info["itemId"] = best_id;
        info["itemName"] = name.empty() ? best_id : name;
        info["quantity"] = quantity;
        drops_vec.emplace_back(std::move(info));
    }

    if (drops_vec.empty()) {
        return true;
    }

    json::value info = basic_info_with_what("InfrastRecycleProduct");
    info["details"]["drops"] = json::array(std::move(drops_vec));
    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}

std::vector<cv::Point> asst::InfrastRewardRecycleProductTaskPlugin::detect_icon_centers(const cv::Mat& image)
{
    const Rect& roi = Task.get("InfrastRecycleProductItem")->roi;
    cv::Mat gray;
    cv::cvtColor(image(make_rect<cv::Rect>(roi)), gray, cv::COLOR_BGR2GRAY);
    cv::medianBlur(gray, gray, 5);

    // 同行图标间距约 154，最小圆心距取其八成，避免同一图标的内外圈重复检出
    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT, 1, 120, 100, 30, IconRingRadius - 8, IconRingRadius + 8);

    std::vector<cv::Point> centers;
    centers.reserve(circles.size());
    for (const auto& c : circles) {
        centers.emplace_back(cvRound(c[0]) + roi.x, cvRound(c[1]) + roi.y);
    }
    // 按弹窗布局先行后列排序，与展示顺序一致。先按纵坐标排序，与行首纵坐标差不超过圆环半径的归为同一行，再在行内按横坐标排序；
    // 带容差的单次比较在出现误检圆心时不满足严格弱序，所以分两步排。同行圆心纵坐标误差在几像素内，行距大于图标直径，
    // 以行首为基准分组不会把相邻两行并为一行
    std::ranges::sort(centers, {}, &cv::Point::y);
    for (auto row_begin = centers.begin(); row_begin != centers.end();) {
        const int row_y = row_begin->y;
        const auto row_end = std::ranges::find_if(row_begin, centers.end(), [row_y](const cv::Point& p) {
            return p.y - row_y > IconRingRadius;
        });
        std::ranges::sort(row_begin, row_end, {}, &cv::Point::x);
        row_begin = row_end;
    }
    return centers;
}

int asst::InfrastRewardRecycleProductTaskPlugin::match_quantity(const cv::Mat& image, const cv::Point& center)
{
    // 数量角标位于图标右下角，黑底白字
    RegionOCRer analyzer(image);
    analyzer.set_task_info("NumberOcrReplace");
    analyzer.set_roi(Rect(center.x + 8, center.y + 22, 42, 32));
    analyzer.set_bin_threshold(110, 255);
    if (!analyzer.analyze()) {
        LogWarn << __FUNCTION__ << "quantity ocr failed, fallback to 1";
        return 1;
    }

    const std::string& text = analyzer.get_result().text;
    int quantity = 0;
    for (char c : text) {
        if (c >= '0' && c <= '9') {
            quantity = quantity * 10 + (c - '0');
        }
    }
    if (quantity <= 0) {
        LogWarn << __FUNCTION__ << "invalid quantity text:" << text << ", fallback to 1";
        return 1;
    }
    return quantity;
}
