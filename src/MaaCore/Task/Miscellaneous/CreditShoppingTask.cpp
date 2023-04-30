#include "CreditShoppingTask.h"

#include <algorithm>
#include <vector>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/CreditShopImageAnalyzer.h"
#include "Vision/OCRer.h"

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

asst::CreditShoppingTask& asst::CreditShoppingTask::set_force_shopping_if_credit_full(
    bool force_shopping_if_credit_full) noexcept
{
    m_force_shopping_if_credit_full = force_shopping_if_credit_full;
    return *this;
}

int asst::CreditShoppingTask::credit_ocr()
{
    cv::Mat credit_image = ctrler()->get_image();
    OCRer credit_analyzer(credit_image);
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
    const cv::Mat& image = ctrler()->get_image();

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
        ctrler()->click(commodity);

        ProcessTask(*this, { "CreditShop-BuyIt" }).run();

        if (ProcessTask(*this, { "CreditShop-NoMoney" }).set_task_delay(0).set_retry_times(0).run()) {
            break;
        }
        if (need_exit()) {
            return false;
        }
        if (credit_ocr_enabled) {
            int credit = credit_ocr();
            if (credit <= MaxCredit) { // 信用值不再溢出，停止购物
                break;
            }
        }
    }

    return true;
}

bool asst::CreditShoppingTask::_run()
{
    Log.trace("CreditShopping: m_is_white_list:", m_is_white_list,
              " m_force_shopping_if_credit_full: ", m_force_shopping_if_credit_full);

    if (!m_force_shopping_if_credit_full) {
        return credit_shopping(true, false);
    }
    else {
        int credit = credit_ocr(); // 识别信用值，防止信用值溢出

        if (credit > MaxCredit) { // 信用值溢出
            return credit_shopping(false, true);
        }
    }
    return true;
}
