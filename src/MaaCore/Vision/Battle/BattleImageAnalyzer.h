#pragma once
#include "Vision/VisionHelper.h"

#include "Common/AsstBattleDef.h"

namespace asst
{
    class BattleImageAnalyzer : public VisionHelper
    {
    public:
        struct ObjectOfInterest
        {
            bool flag = true;
            bool deployment = false;
            bool kills = false;
            bool costs = false;
            bool in_detail = false;
        };

        struct Result
        {
            ObjectOfInterest object_of_interest;

            std::vector<battle::DeploymentOper> deployment;
            // kills / total_kills
            std::optional<std::pair<int, int>> kills;
            std::optional<int> costs;

            bool in_detail = false;
            bool pause_button = false;
        };

        using ResultOpt = std::optional<Result>;

    public:
        using VisionHelper::VisionHelper;
        virtual ~BattleImageAnalyzer() override = default;

        void set_object_to_analyze(ObjectOfInterest obj);
        void set_total_kills_prompt(int prompt);

        const ResultOpt& analyze();
        const ResultOpt& result() const noexcept;

    protected:
        bool hp_flag_analyze();
        bool kills_flag_analyze();
        bool pause_button_analyze();

        std::vector<battle::DeploymentOper> deployment_analyze(); // 识别干员
        battle::Role oper_role_analyze(const Rect& roi);
        bool oper_cooling_analyze(const Rect& roi);
        int oper_cost_analyze(const Rect& roi);
        bool oper_available_analyze(const Rect& roi);

        std::optional<std::pair<int, int>> kills_analyze(); // 识别击杀数
        std::optional<int> costs_analyze();                 // 识别费用
        bool in_detail_analyze();                           // 识别是否在详情页

        ObjectOfInterest m_object_of_interest; // 待识别的目标
        int m_total_kills_prompt = 0; // 之前的击杀总数，因为击杀数经常识别不准所以依赖外部传入作为参考
    };
} // namespace asst
