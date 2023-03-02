#pragma once
#include "InfrastAbstractTask.h"

namespace asst
{
    class InfrastDormTask final : public InfrastAbstractTask
    {
    public:
        using InfrastAbstractTask::InfrastAbstractTask;
        virtual ~InfrastDormTask() override = default;

        virtual size_t max_num_of_opers() const noexcept override { return 5ULL; }

        InfrastDormTask& set_notstationed_enabled(bool dorm_notstationed_enabled) noexcept;
        InfrastDormTask& set_trust_enabled(bool m_dorm_trust_enabled) noexcept;

    protected:
        virtual bool swipe_and_select_custom_opers();
        virtual bool click_order_by_mood();
        virtual bool click_filter_menu_not_stationed_button(); // 点击干员选择页面的筛选菜单按钮的“未进驻”按钮
        virtual bool click_filter_menu_cancel_not_stationed_button(); // 取消点击干员选择页面的筛选菜单按钮的“未进驻”按钮

    private:
        virtual bool _run() override;
        // virtual bool click_confirm_button() override;

        bool opers_choose(asst::infrast::CustomRoomConfig const& origin_room_config);

        bool m_dorm_notstationed_enabled = false; // 设置是否启用未进驻筛选
        bool m_dorm_trust_enabled = true;         // 设置是否启用蹭信赖

        int m_max_num_of_dorm = 4;

        enum class NextStep
        {
            Rest,
            RestDone,
            Trust,
            Fill,
            AllDone,
        };
        NextStep m_next_step = NextStep::Rest;

        enum FilterNotstationed
        {
            Unknown,
            Pressed,
            Unpressed,
        };
        FilterNotstationed m_filter_notstationed = FilterNotstationed::Unknown;
    };
}
