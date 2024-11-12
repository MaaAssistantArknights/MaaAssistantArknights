#pragma once

#include "Task/AbstractTask.h"

#include <vector>

#include "Common/AsstTypes.h"

namespace asst
{
class CreditShoppingTask final : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~CreditShoppingTask() override = default;

    void set_black_list(std::vector<std::string> black_list);
    void set_white_list(std::vector<std::string> white_list);

    static constexpr int MaxCredit = 300;

    CreditShoppingTask& set_force_shopping_if_credit_full(bool force_shopping_if_credit_full) noexcept;
    CreditShoppingTask& set_only_buy_discount(bool only_buy_discount) noexcept;
    CreditShoppingTask& set_reserve_max_credit(bool reserve_max_credit) noexcept;
    CreditShoppingTask& set_info_credit_full(bool info_credit_full) noexcept;

protected:
    virtual bool _run() override;
    bool m_force_shopping_if_credit_full = false; // 设置是否防止信用值溢出
    bool m_only_buy_discount = false; // 设置只购买折扣信用商品（未打折的白名单物品仍会购买）
    bool m_reserve_max_credit = false; // 设置消耗信用点至300以下时停止购买商品（仍然会购买白名单物品）
    bool m_info_credit_full = false; // 设置是否在不购买黑名单物品阶段通知信用点溢出
    int credit_ocr();

    // 用于在信用商店界面识别商品信息左上角的折扣信息
    int discount_ocr(const Rect& commodity);

    bool credit_shopping(bool white_list_enabled, bool credit_ocr_enabled);

    std::vector<std::string> m_shopping_list;
    bool m_is_white_list = false;
};
}
