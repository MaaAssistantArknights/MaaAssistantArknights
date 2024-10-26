#include "CreditShopImageAnalyzer.h"

#include "Utils/Ranges.hpp"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/OCRer.h"

void asst::CreditShopImageAnalyzer::set_black_list(std::vector<std::string> black_list)
{
    Log.info(__FUNCTION__, black_list);

    m_shopping_list = std::move(black_list);
    m_is_white_list = false;
}

void asst::CreditShopImageAnalyzer::set_white_list(std::vector<std::string> black_list)
{
    Log.info(__FUNCTION__, black_list);

    m_shopping_list = std::move(black_list);
    m_is_white_list = true;
}

bool asst::CreditShopImageAnalyzer::analyze()
{
    m_commodities.clear();
    m_need_to_buy.clear();
    m_result.clear();

    return commodities_analyze() && whether_to_buy_analyze() && sold_out_analyze();
}

bool asst::CreditShopImageAnalyzer::commodities_analyze()
{
    // 识别信用点的图标
    const auto commodity_task_ptr = Task.get("CreditShop-Commodities");
    MultiMatcher mm_analyzer(m_image);
    mm_analyzer.set_task_info(commodity_task_ptr);

    if (!mm_analyzer.analyze()) {
        save_img(utils::path("debug") / utils::path("other"));
        return false;
    }
    auto credit_points_result = mm_analyzer.get_result();
    if (credit_points_result.empty()) {
        save_img(utils::path("debug") / utils::path("other"));
        return false;
    }

    sort_by_horizontal_(credit_points_result);

    m_commodities.reserve(credit_points_result.size());
    for (const MatchRect& mr : credit_points_result) {
        Rect commodity;
        commodity.x = mr.rect.x + commodity_task_ptr->rect_move.x;
        commodity.y = mr.rect.y + commodity_task_ptr->rect_move.y;
        commodity.width = commodity_task_ptr->rect_move.width;
        commodity.height = commodity_task_ptr->rect_move.height;
        m_commodities.emplace_back(commodity);
    }

    return true;
}

bool asst::CreditShopImageAnalyzer::whether_to_buy_analyze()
{
    Log.info(__FUNCTION__, m_shopping_list, "mode", m_is_white_list);

    const auto product_name_task_ptr = Task.get<OcrTaskInfo>("CreditShop-ProductName");

    for (const Rect& commodity : m_commodities) {
        // 商品名的区域
        Rect name_roi = product_name_task_ptr->roi;
        name_roi.x += commodity.x;
        name_roi.y += commodity.y;

        OCRer ocr_analyzer(m_image);
        ocr_analyzer.set_roi(name_roi);
        ocr_analyzer.set_replace(product_name_task_ptr->replace_map);
        ocr_analyzer.set_required(m_shopping_list);
        if (ocr_analyzer.analyze()) {
            // 黑名单模式，有识别结果说明这个商品不买，直接跳过
            if (!m_is_white_list && !m_shopping_list.empty()) {
                continue;
            }
        }
        // 白名单模式，没有识别结果说明这个商品不买，直接跳过
        else if (m_is_white_list) {
            continue;
        }

#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(commodity), cv::Scalar(0, 0, 255), 2);
#endif
        const std::string& name =
            ocr_analyzer.get_result().empty() ? std::string() : ocr_analyzer.get_result().front().text;
        Log.info("need to buy", name);
        m_need_to_buy.emplace_back(commodity, name);
    }

    if (m_is_white_list) {
        ranges::sort(m_need_to_buy, std::less {}, [&](const auto& pair) {
            return ranges::find(m_shopping_list, pair.second);
        });
    }

    return !m_need_to_buy.empty();
}

bool asst::CreditShopImageAnalyzer::sold_out_analyze()
{
    // 识别是否售罄
    Matcher sold_out_analyzer(m_image);
    sold_out_analyzer.set_task_info("CreditShop-SoldOut");

    for (const auto& commodity : m_need_to_buy | views::keys) {
        sold_out_analyzer.set_roi(commodity);
        if (sold_out_analyzer.analyze()) {
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(commodity), cv::Scalar(0, 0, 255));
            cv::putText(m_image_draw, "Sold Out", cv::Point(commodity.x, commodity.y), 1, 2, cv::Scalar(255, 0, 0));
#endif //  ASST_DEBUG

            // 如果识别到了售罄，那这个商品就不用买了，跳过
            continue;
        }
        m_result.emplace_back(commodity);
    }

    return !m_result.empty();
}
