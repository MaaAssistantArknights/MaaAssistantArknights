#include "CreditShopImageAnalyzer.h"

#include "MatchImageAnalyzer.h"
#include "MultiMatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "Resource.h"

#include "AsstUtils.hpp"

void asst::CreditShopImageAnalyzer::set_black_list(std::vector<std::string> black_list)
{
    m_shopping_list = std::move(black_list);
    m_is_white_list = false;
}

void asst::CreditShopImageAnalyzer::set_white_list(std::vector<std::string> black_list)
{
    m_shopping_list = std::move(black_list);
    m_is_white_list = true;
}

bool asst::CreditShopImageAnalyzer::analyze()
{
    m_commoditys.clear();
    m_need_to_buy.clear();
    m_result.clear();

    return commoditys_analyze() && whether_to_buy_analyze() && sold_out_analyze();
}

bool asst::CreditShopImageAnalyzer::commoditys_analyze()
{
    // 识别信用点的图标
    const auto commodity_task_ptr = Task.get("CreditShop-Commoditys");
    MultiMatchImageAnalyzer mm_annlyzer(m_image);
    mm_annlyzer.set_task_info(commodity_task_ptr);

    if (!mm_annlyzer.analyze()) {
        return false;
    }
    mm_annlyzer.sort_result();
    auto credit_points_result = mm_annlyzer.get_result();
    if (credit_points_result.empty()) {
        return false;
    }

    m_commoditys.reserve(credit_points_result.size());
    for (const MatchRect& mr : credit_points_result) {
        Rect commodity;
        commodity.x = mr.rect.x + commodity_task_ptr->rect_move.x;
        commodity.y = mr.rect.y + commodity_task_ptr->rect_move.y;
        commodity.width = commodity_task_ptr->rect_move.width;
        commodity.height = commodity_task_ptr->rect_move.height;
        m_commoditys.emplace_back(std::move(commodity));
    }

    return true;
}

bool asst::CreditShopImageAnalyzer::whether_to_buy_analyze()
{
    const auto not_to_buy_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
        Task.get("CreditShop-NotToBuy"));

    for (const Rect& commodity : m_commoditys) {
        // 商品名的区域
        Rect name_roi = not_to_buy_task_ptr->roi;
        name_roi.x += commodity.x;
        name_roi.y += commodity.y;

        OcrImageAnalyzer ocr_analyzer(m_image, name_roi);
        ocr_analyzer.set_required(m_shopping_list);
        if (ocr_analyzer.analyze()) {
            // 黑名单模式，有识别结果说明这个商品不买，直接跳过
            if (!m_is_white_list) {
                continue;
            }
        }
        // 白名单模式，没有识别结果说明这个商品不买，直接跳过
        else if (m_is_white_list) {
            continue;
        }

#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(commodity), cv::Scalar(0, 0, 255), 2);
#endif
        m_need_to_buy.emplace_back(commodity);
    }
    return !m_need_to_buy.empty();
}

bool asst::CreditShopImageAnalyzer::sold_out_analyze()
{
    // 识别是否售罄
    MatchImageAnalyzer sold_out_analyzer(m_image);
    sold_out_analyzer.set_task_info("CreditShop-SoldOut");

    for (const Rect& commodity : m_need_to_buy) {
        sold_out_analyzer.set_roi(commodity);
        if (sold_out_analyzer.analyze()) {
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(commodity), cv::Scalar(0, 0, 255));
            cv::putText(m_image_draw, "Sold Out", cv::Point(commodity.x, commodity.y), 1, 2, cv::Scalar(255, 0, 0));
#endif //  ASST_DEBUG

            // 如果识别到了售罄，那这个商品就不用买了，跳过
            continue;
        }
        m_result.emplace_back(commodity);
    }

    return !m_result.empty();
}
