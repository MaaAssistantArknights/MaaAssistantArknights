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

    protected:
        virtual bool _run() override;
        bool m_force_shopping_if_credit_full = false; // 设置是否防止信用值溢出
        bool m_only_buy_discount = false; // 只购买折扣信用商品（未打折的白名单物品仍会购买）
        bool m_reserve_max_credit = false; // 设置300以下信用点停止购买商品
        int credit_ocr();

        typedef unsigned char t_vec3b[3];

        // 用于识别信用商品右上角的折扣信息(需要点开具体信用商品)
        int discount_ocr(); 

        bool credit_shopping(bool white_list_enabled, bool credit_ocr_enabled);

        std::vector<std::string> m_shopping_list;
        bool m_is_white_list = false;
    };
}
