#pragma once
#include "Vision/VisionHelper.h"

namespace asst
{
    class InfrastFacilityImageAnalyzer final : public VisionHelper
    {
    public:
        using VisionHelper::VisionHelper;
        virtual ~InfrastFacilityImageAnalyzer() override = default;
        InfrastFacilityImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;

        bool analyze();

        void set_to_be_analyzed(std::vector<std::string> facilities) noexcept
        {
            m_to_be_analyzed = std::move(facilities);
        }

        size_t get_quantity(const std::string& name) const
        {
            if (auto iter = m_result.find(name); iter == m_result.cend()) {
                return 0;
            }
            else {
                return iter->second.size();
            }
        }
        Rect get_rect(const std::string& name, int index) const
        {
            if (auto iter = m_result.find(name); iter == m_result.cend()) {
                return {};
            }
            else {
                if (index < 0 || static_cast<size_t>(index) >= iter->second.size()) {
                    return {};
                }
                else {
                    return iter->second.at(index).rect;
                }
            }
        }
        const std::unordered_map<std::string, std::vector<MatchRect>>& get_result() const noexcept { return m_result; }

    private:
        // 该分析器不支持外部设置ROI
        using VisionHelper::set_roi;

        // key：设施名，value：所有这种设施的当前Rect（例如所有制造站的位置）
        std::unordered_map<std::string, std::vector<MatchRect>> m_result;
        // 需要识别的设施名
        std::vector<std::string> m_to_be_analyzed;
    };
}
