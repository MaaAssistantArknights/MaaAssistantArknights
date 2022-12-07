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

    protected:
        virtual bool _run() override;
        bool m_force_shopping_if_credit_full = false; // 设置是否防止信用值溢出
        int credit_ocr();
        bool credit_shopping(bool white_list_enabled, bool credit_ocr_enabled);

        std::vector<std::string> m_shopping_list;
        bool m_is_white_list = false;
    };
}
