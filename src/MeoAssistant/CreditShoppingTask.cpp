#include "CreditShoppingTask.h"

#include <algorithm>
#include <vector>

#include <opencv2/opencv.hpp>

#include "Controller.h"
#include "CreditShopImageAnalyzer.h"
#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "Resource.h"

bool asst::CreditShoppingTask::_run()
{
    const cv::Mat image = Ctrler.get_image();

    CreditShopImageAnalyzer shop_analyzer(image);
    if (!shop_analyzer.analyze( m_black_list)) {
        return false;
    }
    const auto& shopping_list = shop_analyzer.get_result();

    for (const Rect& commodity : shopping_list) {
        if (need_exit()) {
            return false;
        }
        Ctrler.click(commodity);

        static int pre_delay = 1000;
        static int rare_delay = 1000;

        sleep(pre_delay);

        // “购买商品”按钮
        static Rect buy_it_rect;
        if (buy_it_rect.empty()) {
            const auto buy_it_task_ptr = Task.get("CreditShop-BuyIt");

            const cv::Mat buy_image = Ctrler.get_image();
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
        Ctrler.click(buy_it_rect);
        sleep(rare_delay);

        // 识别是否信用不足无法购买
        const cv::Mat prompt_image = Ctrler.get_image();

        OcrImageAnalyzer prompt_analyzer(prompt_image);
        prompt_analyzer.set_task_info("CreditShop-NoMoney");
        if (prompt_analyzer.analyze()) {
            click_return_button();
            return true;
        }

        // 这里随便点一下都行，把购买完弹出来物品的界面取消掉
        Ctrler.click(buy_it_rect);
        sleep(rare_delay);
    }
    return true;
}

asst::CreditShoppingTask& asst::CreditShoppingTask::set_black_list(std::vector<int> black_list) noexcept
{
    m_black_list = std::move(black_list);
    return *this;
}
