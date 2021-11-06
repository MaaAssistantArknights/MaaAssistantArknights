/*
    MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
    Copyright (C) 2021 MistEO and Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst {
    class InfrastFacilityImageAnalyzer final : public AbstractImageAnalyzer {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastFacilityImageAnalyzer() = default;
        InfrastFacilityImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;

        virtual bool analyze() override;

        void set_to_be_analyzed(std::vector<std::string> facilities) noexcept {
            m_to_be_analyzed = std::move(facilities);
        }

        int get_quantity(const std::string& name) const {
            if (auto iter = m_result.find(name);
                iter == m_result.cend()) {
                return 0;
            }
            else {
                return iter->second.size();
            }
        }
        Rect get_rect(const std::string& name, int index) const {
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
        const std::unordered_map<std::string, std::vector<MatchRect>>& get_result() const noexcept {
            return m_result;
        }

    private:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override {
            AbstractImageAnalyzer::set_roi(roi);
        }
        virtual void set_image(const cv::Mat& image, const Rect& roi) {
            AbstractImageAnalyzer::set_image(image, roi);
        }

        // key：设施名，value：所有这种设施的当前Rect（例如所有制造站的位置）
        std::unordered_map<std::string, std::vector<MatchRect>> m_result;
        // 需要识别的设施名
        std::vector<std::string> m_to_be_analyzed;
    };
}
