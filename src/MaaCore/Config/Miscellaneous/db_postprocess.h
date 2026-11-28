#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

// DBNet text-detection post-processing: turn the probability map into text
// quadrilaterals and crop them with a perspective transform.
//
// The parameters follow the PaddleOCR DB defaults (box_thresh = 0.6,
// unclip_ratio = 2.0). The polygon expansion is implemented geometrically
// instead of pulling in ClipperLib: the input always comes from minAreaRect,
// so growing a rectangle by d along its normal is exactly equivalent to adding
// 2d to its width and height, which matches the ClipperOffset + minAreaRect
// combination of the reference implementation.
namespace db_postprocess
{

// Sort predicate comparing two points by ascending X, used by get_mini_boxes.
inline bool x_sort_fp32(const cv::Point2f& a, const cv::Point2f& b)
{
    if (a.x != b.x) {
        return a.x < b.x;
    }
    return false;
}

// Reorder the four corners of a rotated rect into a stable
// [top-left, top-right, bottom-right, bottom-left] order.
//
// @param box the rotated rect produced by minAreaRect
// @param[out] ssid the longer side of the rect, used for the min-size filter
// @return the four corners in the canonical order
inline std::vector<cv::Point2f> get_mini_boxes(const cv::RotatedRect& box, float& ssid)
{
    ssid = std::max(box.size.width, box.size.height);

    cv::Point2f pts[4];
    box.points(pts);
    std::vector<cv::Point2f> array(pts, pts + 4);
    std::sort(array.begin(), array.end(), x_sort_fp32);

    cv::Point2f idx1 = array[0], idx2 = array[1], idx3 = array[2], idx4 = array[3];
    if (array[3].y <= array[2].y) {
        idx2 = array[3];
        idx3 = array[2];
    }
    else {
        idx2 = array[2];
        idx3 = array[3];
    }
    if (array[1].y <= array[0].y) {
        idx1 = array[1];
        idx4 = array[0];
    }
    else {
        idx1 = array[0];
        idx4 = array[1];
    }

    return { idx1, idx2, idx3, idx4 };
}

// Mean probability inside the quadrilateral, used to reject weak detections.
//
// @param box the four corners in canonical order
// @param pred the DB probability map (CV_32FC1, values in [0, 1])
// @return the mean probability over the polygon interior
inline float box_score_fast(const std::vector<cv::Point2f>& box, const cv::Mat& pred)
{
    const int width = pred.cols;
    const int height = pred.rows;

    float box_x[4] = { box[0].x, box[1].x, box[2].x, box[3].x };
    float box_y[4] = { box[0].y, box[1].y, box[2].y, box[3].y };

    int xmin = std::clamp(static_cast<int>(std::floor(*std::min_element(box_x, box_x + 4))), 0, width - 1);
    int xmax = std::clamp(static_cast<int>(std::ceil(*std::max_element(box_x, box_x + 4))), 0, width - 1);
    int ymin = std::clamp(static_cast<int>(std::floor(*std::min_element(box_y, box_y + 4))), 0, height - 1);
    int ymax = std::clamp(static_cast<int>(std::ceil(*std::max_element(box_y, box_y + 4))), 0, height - 1);

    cv::Mat mask = cv::Mat::zeros(ymax - ymin + 1, xmax - xmin + 1, CV_8UC1);

    cv::Point root_point[4];
    for (int i = 0; i < 4; ++i) {
        root_point[i] = cv::Point(static_cast<int>(box[i].x) - xmin, static_cast<int>(box[i].y) - ymin);
    }
    const cv::Point* ppt[1] = { root_point };
    int npt[] = { 4 };
    cv::fillPoly(mask, ppt, npt, 1, cv::Scalar(1));

    cv::Mat cropped_img;
    pred(cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1)).copyTo(cropped_img);

    return static_cast<float>(cv::mean(cropped_img, mask)[0]);
}

// Grow a text box by area * unclip_ratio / perimeter along its normal.
//
// @param box the four corners in canonical order
// @param unclip_ratio expansion ratio; the PaddleOCR default is 2.0
// @return the expanded rotated rect; degenerate input yields a 1x1 box so the
//         caller can filter it out
inline cv::RotatedRect unclip(const std::vector<cv::Point2f>& box, float unclip_ratio)
{
    float area = 0.0F;
    float dist = 0.0F;
    for (int i = 0; i < 4; ++i) {
        area += box[i].x * box[(i + 1) % 4].y - box[i].y * box[(i + 1) % 4].x;
        dist += std::hypot(box[i].x - box[(i + 1) % 4].x, box[i].y - box[(i + 1) % 4].y);
    }
    area = std::abs(area * 0.5F);
    if (dist <= 1e-4F) {
        return cv::RotatedRect(cv::Point2f(0, 0), cv::Size2f(1, 1), 0);
    }
    const float distance = area * unclip_ratio / dist;

    const cv::Point2f& p0 = box[0];
    const cv::Point2f& p1 = box[1];
    const cv::Point2f& p3 = box[3];
    const float box_w = std::hypot(p1.x - p0.x, p1.y - p0.y);
    const float box_h = std::hypot(p3.x - p0.x, p3.y - p0.y);
    if (box_w <= 1e-4F || box_h <= 1e-4F) {
        return cv::RotatedRect(cv::Point2f(0, 0), cv::Size2f(1, 1), 0);
    }

    const cv::Point2f center(
        (box[0].x + box[1].x + box[2].x + box[3].x) * 0.25F,
        (box[0].y + box[1].y + box[2].y + box[3].y) * 0.25F);
    // Use the top edge direction as the rotation angle so size.width stays
    // aligned with that edge.
    const float angle = std::atan2(p1.y - p0.y, p1.x - p0.x) * 180.0F / static_cast<float>(CV_PI);

    return cv::RotatedRect(center, cv::Size2f(box_w + 2.0F * distance, box_h + 2.0F * distance), angle);
}

// Main DB entry point: extract text boxes from the binarised probability map.
//
// @param pred the detection probability map (CV_32FC1, values in [0, 1])
// @param bitmap the map thresholded into a mask (CV_8UC1, non-zero = text)
// @param box_thresh mean-probability threshold; boxes below it are dropped
// @param unclip_ratio polygon expansion ratio
// @return text quadrilaterals, each ordered [top-left, top-right, bottom-right,
//         bottom-left]
inline std::vector<std::vector<cv::Point2f>> boxes_from_bitmap(
    const cv::Mat& pred,
    const cv::Mat& bitmap,
    float box_thresh = 0.6F,
    float unclip_ratio = 2.0F,
    int max_candidates = 3000)
{
    constexpr int min_size = 3;

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bitmap, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    // Cap the candidate count so a pathological frame full of text-like noise
    // cannot make post-processing time explode.
    const int num_contours = std::min(static_cast<int>(contours.size()), max_candidates);
    std::vector<std::vector<cv::Point2f>> boxes;

    for (int i = 0; i < num_contours; ++i) {
        if (contours[i].size() <= 2) {
            continue;
        }
        float ssid = 0.0F;
        cv::RotatedRect box = cv::minAreaRect(contours[i]);
        auto array = get_mini_boxes(box, ssid);
        if (ssid < min_size) {
            continue;
        }

        const float score = box_score_fast(array, pred);
        if (score < box_thresh) {
            continue;
        }

        cv::RotatedRect clipbox = unclip(array, unclip_ratio);
        if (clipbox.size.height < 1.001F && clipbox.size.width < 1.001F) {
            continue;
        }

        float clip_ssid = 0.0F;
        auto cliparray = get_mini_boxes(clipbox, clip_ssid);
        if (clip_ssid < min_size + 2) {
            continue;
        }

        boxes.push_back(cliparray);
    }
    return boxes;
}

// Perspective-crop a quadrilateral into a horizontal text-line image, using the
// PaddleOCR reference behaviour (rotate 90 degrees when the crop is much taller
// than wide).
//
// @param src the original BGR screenshot
// @param points the four corners in canonical order
// @return the rectified text line; empty when the crop area is degenerate
inline cv::Mat get_rotate_crop_image(const cv::Mat& src, const std::vector<cv::Point2f>& points)
{
    int left = static_cast<int>(std::min({ points[0].x, points[1].x, points[2].x, points[3].x }));
    int right = static_cast<int>(std::max({ points[0].x, points[1].x, points[2].x, points[3].x }));
    int top = static_cast<int>(std::min({ points[0].y, points[1].y, points[2].y, points[3].y }));
    int bottom = static_cast<int>(std::max({ points[0].y, points[1].y, points[2].y, points[3].y }));

    left = std::clamp(left, 0, src.cols - 1);
    right = std::clamp(right, 0, src.cols - 1);
    top = std::clamp(top, 0, src.rows - 1);
    bottom = std::clamp(bottom, 0, src.rows - 1);

    if (right <= left || bottom <= top) {
        return {};
    }

    cv::Mat img_crop;
    src(cv::Rect(left, top, right - left + 1, bottom - top + 1)).copyTo(img_crop);

    cv::Point2f pts[4];
    for (int i = 0; i < 4; ++i) {
        pts[i] = cv::Point2f(points[i].x - left, points[i].y - top);
    }

    const int crop_w = static_cast<int>(std::round(std::hypot(pts[0].x - pts[1].x, pts[0].y - pts[1].y)));
    const int crop_h = static_cast<int>(std::round(std::hypot(pts[0].x - pts[3].x, pts[0].y - pts[3].y)));

    if (crop_w <= 3 || crop_h <= 3) {
        return {};
    }

    cv::Point2f pts_std[4] = { cv::Point2f(0.0F, 0.0F),
                               cv::Point2f(static_cast<float>(crop_w), 0.0F),
                               cv::Point2f(static_cast<float>(crop_w), static_cast<float>(crop_h)),
                               cv::Point2f(0.0F, static_cast<float>(crop_h)) };

    cv::Mat transform = cv::getPerspectiveTransform(pts, pts_std);
    cv::Mat dst;
    cv::warpPerspective(img_crop, dst, transform, cv::Size(crop_w, crop_h), cv::INTER_LINEAR, cv::BORDER_REPLICATE);

    if (static_cast<float>(dst.rows) >= static_cast<float>(dst.cols) * 1.5F) {
        cv::Mat rotated;
        cv::transpose(dst, rotated);
        cv::flip(rotated, rotated, 0);
        return rotated;
    }
    return dst;
}

} // namespace db_postprocess
