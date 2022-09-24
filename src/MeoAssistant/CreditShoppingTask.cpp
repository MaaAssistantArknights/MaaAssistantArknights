#include "CreditShoppingTask.h"

#include <algorithm>
#include <vector>
#include <regex>

#include "Controller.h"
#include "CreditShopImageAnalyzer.h"
#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "TaskData.h"
#include "Logger.hpp"

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

std::string asst::CreditShoppingTask::creditOcr()
{
    cv::Mat credit_image = m_ctrler->get_image();
    OcrImageAnalyzer credit_analyzer(credit_image);
    credit_analyzer.set_task_info("CreditShop-CreditOcr");
    if (!credit_analyzer.analyze()) {
        Log.trace("ERROR:!credit_analyzer.analyze():");
        return "";
    }
    std::string credit = credit_analyzer.get_result().front().text;
    std::regex rule("[^0-9]"); // 只保留数字
    credit = std::regex_replace(credit, rule, "");
    Log.trace("credit:", credit);

    return credit;
}

bool asst::CreditShoppingTask::_run()
{
    const cv::Mat image = m_ctrler->get_image();

    CreditShopImageAnalyzer shop_analyzer(image);
    if (m_is_white_list) {
        shop_analyzer.set_white_list(m_shopping_list);
    }
    else {
        shop_analyzer.set_black_list(m_shopping_list);
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
    }

    if (m_save_credit_enabled) {//识别信用值，防止信用值溢出
        
        std::string credit = creditOcr();

        if (credit != "" && atoi(credit.c_str()) > 300) {//信用值溢出

            const cv::Mat image2 = m_ctrler->get_image();

            CreditShopImageAnalyzer shop_analyzer2(image2);

            if (!shop_analyzer2.analyze()) {
                return false;
            }
            const auto& shopping_list2 = shop_analyzer2.get_result();

            for (const Rect& commodity : shopping_list2) {
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

                credit = creditOcr();
                if (credit == "" || atoi(credit.c_str()) <= 300) {//信用值不再溢出，停止购物
                    break;
                }
            }
        }
    }
    return true;
}
