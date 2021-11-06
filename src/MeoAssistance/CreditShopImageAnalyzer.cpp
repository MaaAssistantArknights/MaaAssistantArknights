﻿/*
    MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
    Copyright (C) 2021 MistEO and Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "CreditShopImageAnalyzer.h"

#include "MatchImageAnalyzer.h"
#include "MultiMatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "Resource.h"

#include "AsstUtils.hpp"

bool asst::CreditShopImageAnalyzer::analyze() {
    m_commoditys.clear();
    m_need_to_buy.clear();
    m_result.clear();

    return commoditys_analyze() && whether_to_buy_analyze() && sold_out_analyze();
}

bool asst::CreditShopImageAnalyzer::commoditys_analyze() {
    const auto commodity_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("CreditShop-Commoditys"));

    // 识别信用点的图标
    MultiMatchImageAnalyzer mm_annlyzer(
        m_image,
        commodity_task_ptr->roi,
        commodity_task_ptr->templ_name,
        commodity_task_ptr->templ_threshold);

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

bool asst::CreditShopImageAnalyzer::whether_to_buy_analyze() {
    const auto not_to_buy_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
        resource.task().task_ptr("CreditShop-NotToBuy"));

    for (const Rect& commodity : m_commoditys) {
        // 商品名的区域
        Rect name_roi = not_to_buy_task_ptr->roi;
        name_roi.x += commodity.x;
        name_roi.y += commodity.y;

        OcrImageAnalyzer ocr_analyzer(m_image, name_roi);
        ocr_analyzer.set_required(not_to_buy_task_ptr->text);
        if (ocr_analyzer.analyze()) {
            //因为是不买的，有识别结果说明这个商品不买，直接跳过
            continue;
        }
        const auto& ocr_res = ocr_analyzer.get_result();

#ifdef LOG_TRACE
        cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(commodity), cv::Scalar(0, 0, 255), 2);
#endif
        m_need_to_buy.emplace_back(commodity);
    }
    return !m_need_to_buy.empty();
}

bool asst::CreditShopImageAnalyzer::sold_out_analyze() {
    const auto sold_out_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
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
#ifdef LOG_TRACE
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(commodity), cv::Scalar(0, 0, 255));
            cv::putText(m_image_draw, "Sold Out", cv::Point(commodity.x, commodity.y), 1, 2, cv::Scalar(255, 0, 0));
#endif //  LOG_TRACE

            // 如果识别到了售罄，那这个商品就不用买了，跳过
            continue;
        }
        m_result.emplace_back(commodity);
    }

    return !m_result.empty();
}