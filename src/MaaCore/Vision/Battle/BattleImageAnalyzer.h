#pragma once
#include "Vision/AbstractImageAnalyzer.h"

#include "Common/AsstBattleDef.h"

namespace asst
{
    class BattleImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using Target = uint64_t;
        static constexpr Target None = 0;
        static constexpr Target Deployment = 1ULL << 1;
        static constexpr Target Kills = 1ULL << 2;
        static constexpr Target Cost = 1ULL << 3;
        static constexpr Target DetailPage = 1ULL << 4;

        struct Result
        {
            std::vector<battle::DeploymentOper> deployment;
            int kills = 0;
            int total_kills = 0;
            int costs = 0;
            bool in_detail = false;
            bool pause_button = false;
        };

        using ResultOpt = std::optional<Result>;

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~BattleImageAnalyzer() override = default;

        void set_target(Target target);
        void set_pre_total_kills(int pre_total_kills);

        const ResultOpt& analyze();
        const ResultOpt& result() const noexcept;

    protected:
        bool deployment_analyze();  // 识别干员
        bool detail_page_analyze(); // 识别是否在详情页
        battle::Role oper_role_analyze(const Rect& roi);
        bool oper_cooling_analyze(const Rect& roi);
        int oper_cost_analyze(const Rect& roi);
        bool oper_available_analyze(const Rect& roi);

        bool home_analyze(); // 识别蓝色的家门
        // bool skill_analyze();     // 识别技能是否可用
        bool kills_analyze();     // 识别击杀数
        bool cost_analyze();      // 识别费用
        bool vacancies_analyze(); // 识别剩余可部署人数
        bool flag_analyze();
        bool hp_flag_analyze();
        bool kills_flag_analyze();
        bool pause_button_analyze();

        Target m_target = 0; // 待识别的目标
        int m_pre_total_kills = 0; // 之前的击杀总数，因为击杀数经常识别不准所以依赖外部传入作为参考

    private:
        ResultOpt m_result;
    };
} // namespace asst
