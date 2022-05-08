#ifndef PENGUIN_DEPOT_HPP_
#define PENGUIN_DEPOT_HPP_

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <optional>

#include "json.hpp"
#include "core.hpp"
#include "recognize.hpp"

using dict = nlohmann::ordered_json;

// extern void show_img(cv::Mat src);

namespace penguin
{
enum CircleFlags
{
    X = 0,
    Y = 1,
    R = 2
};

class Depot : public Widget
{
public:
    Depot() = default;
    Depot(const cv::Mat& img)
        : Widget(img) {}
    Depot& analyze()
    {
        _get_max_diameter();
        _preprocess();
        _get_item_list();
        return *this;
    }
    const dict report([[maybe_unused]] bool debug = false)
    {
        dict _report = dict::object();
        return _report;
    }

private:
    cv::Mat _img_ext;
    int _max_diameter;
    int _item_diameter;
    std::vector<Widget_Item> _item_list;

    void _get_max_diameter()
    {
        cv::Mat img_bin = _img;
        img_bin.adjustROI(0, 0, -width / 4, -width / 4);
        cv::cvtColor(img_bin, img_bin, cv::COLOR_BGR2GRAY);
        cv::threshold(img_bin, img_bin, 127, 255, cv::THRESH_BINARY_INV);

        auto sp = separate(img_bin, DirectionFlags::TOP);
        for (auto& range : sp)
        {
            int length = range.end - range.start;
            if (length > _max_diameter)
            {
                _max_diameter = length;
            }
        }
    }
    void _preprocess()
    {
        _img_ext = _img;
        auto img_edge =
            cv::Mat(height, _max_diameter, CV_8UC3, cv::Scalar(0, 0, 0));
        cv::hconcat(img_edge, _img_ext, _img_ext);
        cv::hconcat(_img_ext, img_edge, _img_ext);
    }
    void _get_item_list()
    {
        cv::Mat img_bin = _img_ext;
        cv::cvtColor(img_bin, img_bin, cv::COLOR_BGR2GRAY);

        cv::Mat img_blur;
        cv::medianBlur(img_bin, img_blur, 5);
        std::vector<cv::Vec3f> item_circles;
        cv::HoughCircles(img_blur, item_circles, cv::HOUGH_GRADIENT_ALT, 1,
                         _max_diameter / 2, 200, 0.4, _max_diameter / 4,
                         _max_diameter / 2);
        std::cout << item_circles.size() << std::endl;

        auto comp = [this](cv::Vec3f p1, cv::Vec3f p2) {
            if (p1[X] == p2[X] && p1[Y] == p2[Y])
                return p1[R] > p2[R];
            else if (abs(p1[Y] - p2[Y]) < _max_diameter)
                return p1[X] < p2[X];
            else
                return p1[Y] < p2[Y];
        };
        std::sort(item_circles.begin(), item_circles.end(), comp);
        if (item_circles.size() >= 2)
        { // delete concentric circles
            for (auto it = std::next(item_circles.cbegin());
                 it != item_circles.cend();)
            {
                const auto& c2 = *it;
                const auto& c1 = *std::prev(it);
                if (c1[X] == c2[X] && c1[Y] == c2[Y])
                {
                    it = item_circles.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
        _item_diameter = static_cast<int>(round(cv::mean(item_circles)[R] * 2));

        for (const cv::Vec3i& c : item_circles)
        {
            auto center = cv::Point(c[X], c[Y]);
            int radius = c[R];
            cv::circle(img_blur, center, 1, cv::Scalar(0, 100, 100), 3,
                       cv::LINE_AA);
            circle(img_blur, center, radius, cv::Scalar(255, 0, 255), 3,
                   cv::LINE_AA);
        }

        // show_img(img_blur);

        int radius = _item_diameter / 2;
        int offset = static_cast<int>(radius * 1.2);
        ItemTemplates templs;
        for (const cv::Vec3i& c : item_circles)
        {
            std::string label = "item";
            auto p1 = cv::Point(c[X] - offset, c[Y] - offset);
            auto p2 = cv::Point(c[X] + offset, c[Y] + offset);
            auto itemimg = _img_ext(cv::Rect(p1, p2));
            Widget_Item item {itemimg, _item_diameter, label, this};
            item.analyze(templs, WITHOUT_EXCEPTION);
            std::cout << item.itemId() << ": " << item.confidence()
                      << std::endl;
            if (item.confidence() > _CONFIDENCE_THRESHOLD)
            {
                _item_list.push_back(item);
            }
        }

        for (auto& item : _item_list)
        {
            auto item_rect = cv::Rect(item.x, item.y, item.width, item.height);
            cv::rectangle(_img_ext, item_rect, cv::Scalar(0, 0, 255), 2);
            cv::putText(_img_ext, std::to_string(item.quantity()),
                        cv::Point(item.x, item.y + item.height - 10),
                        cv::FONT_ITALIC, 1, cv::Scalar(0, 0, 255), 2);
        }
        // show_img(_img_ext);
    }
};
} // namespace penguin

#endif // PENGUIN_DEPOT_HPP_