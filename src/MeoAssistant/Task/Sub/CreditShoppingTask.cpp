#include "CreditShoppingTask.h"

#include <algorithm>
#include <vector>

#include "Controller.h"
#include "ImageAnalyzer/CreditShopImageAnalyzer.h"
#include "ImageAnalyzer/General/MatchImageAnalyzer.h"
#include "ImageAnalyzer/General/OcrImageAnalyzer.h"
#include "TaskData.h"
#include "Utils/Logger.hpp"

void asst::CreditShoppingTask::set_black_list(std::vector<std::string> black_list)
{
    m_shopping_list = std::move(black_list);
    m_is_white_list = false;
}

void asst::CreditShoppingTask::set_white_list(std::vector<std::string> black_list)
{
    m_shopping_list = std::move(black_list);
    m_is_white_list = true;
}

asst::CreditShoppingTask& asst::CreditShoppingTask::set_force_shopping_if_credit_full(bool force_shopping_if_credit_full) noexcept
{
    m_force_shopping_if_credit_full = force_shopping_if_credit_full;
    return *this;
}

int asst::CreditShoppingTask::credit_ocr()
{
    cv::Mat credit_image = m_ctrler->get_image();
    OcrImageAnalyzer credit_analyzer(credit_image);
    credit_analyzer.set_task_info("CreditShop-CreditOcr");
    credit_analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);

    if (!credit_analyzer.analyze()) {
        Log.trace("ERROR:!credit_analyzer.analyze():");
        return -1;
    }

    const std::string& credit = credit_analyzer.get_result().front().text;

    if (credit.empty() || !ranges::all_of(credit, [](char c) -> bool { return std::isdigit(c); })) {
        return -1;
    }

    Log.trace("credit:", credit);

    return std::stoi(credit);
}

bool asst::CreditShoppingTask::credit_shopping(bool white_list_enabled, bool credit_ocr_enabled)
{
    const cv::Mat image = m_ctrler->get_image();

    CreditShopImageAnalyzer shop_analyzer(image);
    if (white_list_enabled) {
        if (m_is_white_list) {
            shop_analyzer.set_white_list(m_shopping_list);
        }
        else {
            shop_analyzer.set_black_list(m_shopping_list);
        }
    }

    if (!shop_analyzer.analyze()) {
        return false;
    }
    const auto& shopping_list = shop_analyzer.get_result();

    for (const Rect& commodity : shopping_list) {
        if (need_exit()) {
            return false;
        }
        m_ctrler->click(commodity);

        static int pre_delay = 1000;
        static int rare_delay = 1000;

        sleep(pre_delay);

        // “购买商品”按钮
        static Rect buy_it_rect;
        if (buy_it_rect.empty()) {
            const auto buy_it_task_ptr = Task.get("CreditShop-BuyIt");

            const cv::Mat buy_image = m_ctrler->get_image();
            MatchImageAnalyzer buy_it_analyzer(buy_image);

            buy_it_analyzer.set_task_info(buy_it_task_ptr);
            if (!buy_it_analyzer.analyze()) {
                // 没识别到“购买商品”按钮，不应该出现这种情况，TODO 报错
                return false;
            }
            buy_it_rect = buy_it_analyzer.get_result().rect;
            pre_delay = buy_it_task_ptr->pre_delay;
            rare_delay = buy_it_task_ptr->rear_delay;
        }

        if (need_exit()) {
            return false;
        }
        m_ctrler->click(buy_it_rect);
        sleep(rare_delay);

        // 识别是否信用不足无法购买
        const cv::Mat prompt_image = m_ctrler->get_image();

        OcrImageAnalyzer prompt_analyzer(prompt_image);

        prompt_analyzer.set_task_info("CreditShop-NoMoney");
        if (prompt_analyzer.analyze()) {
            click_return_button();
            break;
        }

        // 这里随便点一下都行，把购买完弹出来物品的界面取消掉
        m_ctrler->click(buy_it_rect);
        sleep(rare_delay);

        if (credit_ocr_enabled) {
            int credit = credit_ocr();
            if (credit <= MaxCredit) {//信用值不再溢出，停止购物
                break;
            }
        }

    }

    return true;
}

bool asst::CreditShoppingTask::_run()
{
    Log.trace("CreditShopping: m_is_white_list:", m_is_white_list, " m_force_shopping_if_credit_full: ", m_force_shopping_if_credit_full);

    if (!m_force_shopping_if_credit_full) {
        return credit_shopping(true, false);
    }
    else {
        int credit = credit_ocr();//识别信用值，防止信用值溢出

        if (credit > MaxCredit) {//信用值溢出
            return credit_shopping(false, true);
        }
    }
    return true;
}
