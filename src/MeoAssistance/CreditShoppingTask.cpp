#include "CreditShoppingTask.h"

#include <vector>
#include <algorithm>

#include <opencv2/opencv.hpp>

#include "Identify.h"
#include "WinMacro.h"
#include "Configer.h"

bool asst::CreditShoppingTask::run()
{
    if (m_controller_ptr == nullptr
        || m_identify_ptr == nullptr)
    {
        m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
        return false;
    }

    cv::Mat image = m_controller_ptr->get_image();

    // 识别信用点的图标
    auto find_res_vec = m_identify_ptr->find_all_images(image, "CreditPoint", Configer::TemplThresholdDefault, false);

    // 按位置排个序
    std::sort(find_res_vec.begin(), find_res_vec.end(), [](
        const auto& lhs, const auto& rhs) -> bool {
            if (std::abs(lhs.rect.y - rhs.rect.y) < 5) {	// y差距较小则理解为是同一排的，按x排序
                return lhs.rect.x < rhs.rect.x;
            }
            else {
                return lhs.rect.y < rhs.rect.y;
            }
        });

    // 计算哪些商品是需要买的
    std::vector<Rect> need_to_buy;
    for (const auto& res : find_res_vec) {
        cv::Rect name_rect;
        name_rect.x = res.rect.x - 80;
        name_rect.y = res.rect.y - 210;
        name_rect.width = 230;
        name_rect.height = 70;
        if (name_rect.x < 0) {
            name_rect.x = 0;
        }
        // 过滤掉不买的
        // TODO，哪些要买哪些不买，提供个接口出去
        static const std::vector<std::string> not_to_buy_names = { "碳", "家具" };
        const auto& ocr_res = m_identify_ptr->ocr_detect(image(name_rect));
        auto find_iter = std::find_first_of(ocr_res.cbegin(), ocr_res.cend(), not_to_buy_names.cbegin(), not_to_buy_names.cend(),
            [](const auto& lhs, const std::string& rhs) -> bool {
                return lhs.text.find(rhs) != std::string::npos;
            });
        if (find_iter != ocr_res.cend()) {
#ifdef  LOG_TRACE
            cv::putText(image, "Not to buy", cv::Point(res.rect.x, res.rect.y), 1, 2, cv::Scalar(255, 0, 0));
#endif //  LOG_TRACE
            continue;
        }
        Rect commodity_rect(res.rect.x - 60, res.rect.y - 180, 220, 220);
        // 识别“售罄”
        auto sold_out_res = m_identify_ptr->find_image(image(make_rect<cv::Rect>(commodity_rect)), "SoldOut");

#ifdef  LOG_TRACE
        cv::rectangle(image, make_rect<cv::Rect>(commodity_rect), cv::Scalar(0, 0, 255));
        if (sold_out_res.score >= Configer::TemplThresholdDefault) {
            cv::putText(image, "Sold Out", cv::Point(commodity_rect.x, commodity_rect.y), 1, 2, cv::Scalar(255, 0, 0));
        }
#endif //  LOG_TRACE

        if (sold_out_res.score >= Configer::TemplThresholdDefault) {
            continue;
        }
        need_to_buy.emplace_back(commodity_rect.center_zoom(0.8));
    }

    for (const Rect& commodity_rect : need_to_buy) {
        if (need_exit()) {
            return false;
        }
        m_controller_ptr->click(commodity_rect);
        sleep(1000);
        image = m_controller_ptr->get_image();

        // “购买商品”按钮
        static Rect buy_it_rect;
        if (buy_it_rect.width == 0) {
            auto buy_it_res = m_identify_ptr->find_image(image, "BuyIt", Configer::TemplThresholdDefault);
            if (buy_it_res.score < Configer::TemplThresholdDefault) {
                // 没事别到“购买商品”按钮，不应该出现这种情况，TODO 报错
                return false;
            }
            buy_it_rect = buy_it_res.rect;
        }
        if (need_exit()) {
            return false;
        }
        m_controller_ptr->click(buy_it_rect);
        sleep(1000);
        // 识别是否信用不足无法购买
        image = m_controller_ptr->get_image();
        static const std::vector<std::string> shopping_finished = { "信用不足", "无法购买" };
        const auto& ocr_res = ocr_detect(image(cv::Rect(940, 60, 339, 110)));
        auto find_iter = std::find_first_of(ocr_res.cbegin(), ocr_res.cend(), shopping_finished.cbegin(), shopping_finished.cend(),
            [](const auto& lhs, const std::string& rhs) -> bool {
                return lhs.text.find(rhs) != std::string::npos;
            });
        if (find_iter != ocr_res.cend()) {
            click_return_button();
            return true;
        }

        // 这里随便点一下都行，把购买完弹出来物品的界面取消掉
        m_controller_ptr->click(buy_it_rect);
        sleep(1000);
    }

    return true;
}