#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class InfrastFacilityImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastFacilityImageAnalyzer() = default;
        InfrastFacilityImageAnalyzer(const cv::Mat image, const Rect& roi) = delete;

        virtual bool analyze() override;

        void set_to_be_analyzed(std::vector<std::string> facilities) noexcept
        {
            m_to_be_analyzed = std::move(facilities);
        }

        size_t get_quantity(const std::string& name) const
        {
            if (auto iter = m_result.find(name);
                iter == m_result.cend()) {
                return 0;
            }
            else {
                return iter->second.size();
            }
        }
        Rect get_rect(const std::string& name, int index) const
        {
            if (auto iter = m_result.find(name);
                iter == m_result.cend()) {
                return Rect();
            }
            else {
                if (index < 0 || index >= iter->second.size()) {
                    return Rect();
                }
                else {
                    return iter->second.at(index).rect;
                }
            }
        }
        const std::unordered_map<std::string, std::vector<MatchRect>>& get_result() const noexcept
        {
            return m_result;
        }

    private:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override
        {
            AbstractImageAnalyzer::set_roi(roi);
        }
        virtual void set_image(const cv::Mat image, const Rect& roi)
        {
            AbstractImageAnalyzer::set_image(image, roi);
        }

        // key：设施名，value：所有这种设施的当前Rect（例如所有制造站的位置）
        std::unordered_map<std::string, std::vector<MatchRect>> m_result;
        // 需要识别的设施名
        std::vector<std::string> m_to_be_analyzed;
    };
}
