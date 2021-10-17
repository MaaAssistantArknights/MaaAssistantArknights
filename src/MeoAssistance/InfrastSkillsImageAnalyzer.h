#pragma once
#include "AbstractImageAnalyzer.h"

#include "AsstDef.h"
#include "InfrastConfiger.h"

namespace asst {
    class InfrastSkillsImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastSkillsImageAnalyzer() = default;
        virtual bool analyze() override;

        const std::vector<std::vector<InfrastSkill>>& get_result() const noexcept {
            return m_skills_result;
        }

        constexpr static int MaxNumOfSkills = 2;    // 单个干员最多有几个基建技能
    protected:
        bool skills_detect();        // 检测出所有技能区域
        bool skills_split();         // 拆分成一个一个的区域
        bool skill_analyze();        // 识别每个技能区域是啥

        std::vector<Rect> m_skills_detected;                    // skills_detect()的结果，每个Rect是单个干员的全部技能
        std::vector<std::vector<Rect>> m_skills_splited;        // skills_split()的结果，每个Rect是单个技能
        std::vector<std::vector<InfrastSkill>> m_skills_result; // skill_analyze()的结果，最终结果
    };
}