#pragma once
#include "AbstractImageAnalyzer.h"

#include <unordered_map>

#include "AsstDef.h"

namespace asst
{
    class InfrastSkillsImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        InfrastSkillsImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;

        virtual ~InfrastSkillsImageAnalyzer() = default;
        virtual bool analyze() override;

        void sort_result();

        const std::vector<InfrastOperSkillInfo>& get_result() const noexcept
        {
            return m_result;
        }
        void set_facility(std::string facility_name) noexcept
        {
            m_facility = std::move(facility_name);
        }

        constexpr static int MaxNumOfSkills = 2; // 单个干员最多有几个基建技能

    private:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override
        {
            AbstractImageAnalyzer::set_roi(roi);
        }
        virtual void set_image(const cv::Mat& image, const Rect& roi)
        {
            AbstractImageAnalyzer::set_image(image, roi);
        }
        bool skills_detect();                              // 检测出所有技能区域
        bool skills_split();                               // 拆分成一个一个的区域
        bool skill_analyze();                              // 识别每个技能区域是啥
        bool selected_analyze(int smiley_x, int smiley_y); // 识别干员是否被选中

        std::string m_facility;
        // skills_detect()的结果，key是hash，value是单个干员的全部技能区域
        std::unordered_map<std::string, Rect> m_skills_detected;
        // skills_split()的结果，key是hash，value中每个Rect是单个技能区域
        std::unordered_map<std::string, std::vector<Rect>> m_skills_splited;
        // skill_analyze()的结果，最终结果
        std::vector<InfrastOperSkillInfo> m_result;
    };
}
