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

#include "AbstractResource.h"

#include <unordered_map>
#include <unordered_set>

#include <opencv2/opencv.hpp>

namespace asst {
    class TemplResource : public AbstractResource {
    public:
        constexpr static double TemplThresholdDefault = 0.9;
        constexpr static double HistThresholdDefault = 0.9;

        virtual ~TemplResource() = default;

        void append_load_required(std::unordered_set<std::string> required) noexcept;
        virtual bool load(const std::string& dir) override;

        bool exist_templ(const std::string& key) const noexcept;
        const cv::Mat& get_templ(const std::string& key) const noexcept;
        const std::pair<cv::Mat, cv::Rect>& get_hist(const std::string& key) const noexcept;

        void emplace_templ(std::string key, cv::Mat templ);
        void emplace_hist(std::string key, cv::Mat hist, cv::Rect roi);
        void clear_hists() noexcept;

    private:
        std::unordered_set<std::string> m_templs_filename;
        std::unordered_map<std::string, cv::Mat> m_templs;
        std::unordered_map<std::string, std::pair<cv::Mat, cv::Rect>> m_hists;
    };
}
