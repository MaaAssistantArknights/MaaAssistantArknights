#include "CreditShopImageAnalyzer.h"

#include "MultiMatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "MatchImageAnalyzer.h"
#include "Resource.h"

#include "AsstUtils.hpp"

bool asst::CreditShopImageAnalyzer::analyze()
{
    m_credit_points.clear();
    m_need_to_buy.clear();
    m_result.clear();

    return credit_point_analyze()
        && commoditys_analyze()
        && sold_out_analyze();
}

bool asst::CreditShopImageAnalyzer::credit_point_analyze()
{
    const static auto credit_point_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("CreditShop-CreditPoint"));

    // 识别信用点的图标
    MultiMatchImageAnalyzer mm_annlyzer(
        m_image,
        credit_point_task_ptr->roi,
        credit_point_task_ptr->templ_name,
        credit_point_task_ptr->templ_threshold);

    if (!mm_annlyzer.analyze()) {
        return false;
    }
    auto credit_points_result = mm_annlyzer.get_result();
    if (credit_points_result.empty()) {
        return false;
    }
    // 按位置排个序
    std::sort(credit_points_result.begin(), credit_points_result.end(),
        [](const MatchRect& lhs, const MatchRect& rhs) -> bool {
            if (std::abs(lhs.rect.y - rhs.rect.y) < 5) {	// y差距较小则理解为是同一排的，按x排序
                return lhs.rect.x < rhs.rect.x;
            }
            else {
                return lhs.rect.y < rhs.rect.y;
            }
        }
    );
    m_credit_points.reserve(credit_points_result.size());
    for (const MatchRect& mr : credit_points_result) {
        m_credit_points.emplace_back(mr.rect);
    }

    return true;
}

bool asst::CreditShopImageAnalyzer::commoditys_analyze()
{
    for (const Rect& credit_point : m_credit_points) {
        // 商品名的区域
        Rect name(credit_point.x - 80, credit_point.y - 210, 230, 70);
        if (name.x < 0) {
            name.x = 0;
        }

        // TODO，哪些要买哪些不买，提供个接口出去
        static const std::vector<std::string> not_to_buy_names = { "碳", "家具" };

        OcrImageAnalyzer ocr_analyzer(m_image, name);
        if (!ocr_analyzer.analyze()) {
            continue;
        }
        const auto& ocr_res = ocr_analyzer.get_result();

        auto find_iter = std::find_first_of(ocr_res.cbegin(), ocr_res.cend(), not_to_buy_names.cbegin(), not_to_buy_names.cend(),
            [](const auto& lhs, const std::string& rhs) -> bool {
                return lhs.text.find(rhs) != std::string::npos;
            });
        if (find_iter != ocr_res.cend()) { // 找到了不需要买的名字
#ifdef  LOG_TRACE
            cv::putText(m_image, "Not to buy", cv::Point(credit_point.x, credit_point.y), 1, 2, cv::Scalar(255, 0, 0));
#endif //  LOG_TRACE
            continue;
        }
        Rect commodity(credit_point.x - 60, credit_point.y - 180, 220, 220);
#ifdef  LOG_TRACE
        cv::rectangle(m_image, utils::make_rect<cv::Rect>(commodity), cv::Scalar(0, 0, 255), 2);
#endif
        m_need_to_buy.emplace_back(commodity);
    }
    return !m_need_to_buy.empty();
}

bool asst::CreditShopImageAnalyzer::sold_out_analyze()
{
    const static auto sold_out_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("CreditShop-SoldOut"));

    // 识别是否售罄
    MatchImageAnalyzer sold_out_analyzer(
        m_image,
        sold_out_task_ptr->roi,
        sold_out_task_ptr->templ_name,
        sold_out_task_ptr->templ_threshold);

    for (const Rect& commodity : m_need_to_buy) {
        sold_out_analyzer.set_roi(commodity);
        if (sold_out_analyzer.analyze()) {
#ifdef  LOG_TRACE
            cv::rectangle(m_image, utils::make_rect<cv::Rect>(commodity), cv::Scalar(0, 0, 255));
            cv::putText(m_image, "Sold Out", cv::Point(commodity.x, commodity.y), 1, 2, cv::Scalar(255, 0, 0));
#endif //  LOG_TRACE

            // 如果识别到了售罄，那这个商品就不用买了，跳过
            continue;
        }
        m_result.emplace_back(commodity);
    }

    return !m_result.empty();
}