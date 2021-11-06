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

#include "TemplResource.h"

#include <array>
#include <filesystem>
#include <string_view>

void asst::TemplResource::append_load_required(std::unordered_set<std::string> required) noexcept {
    m_templs_filename.insert(
        std::make_move_iterator(required.begin()),
        std::make_move_iterator(required.end()));
}

bool asst::TemplResource::load(const std::string& dir) {
    for (const std::string& filename : m_templs_filename) {
        std::string filepath = dir + "\\" + filename;
        if (std::filesystem::exists(filepath)) {
            cv::Mat templ = cv::imread(filepath);
            emplace_templ(filename, std::move(templ));
        }
        else {
            m_last_error = filepath + " not exists";
            return false;
        }
    }

    return true;
}

bool asst::TemplResource::exist_templ(const std::string& key) const noexcept {
    return m_templs.find(key) != m_templs.cend();
}

const cv::Mat& asst::TemplResource::get_templ(const std::string& key) const noexcept {
    if (auto iter = m_templs.find(key);
        iter != m_templs.cend()) {
        return iter->second;
    }
    else {
        const static cv::Mat empty;
        return empty;
    }
}

const std::pair<cv::Mat, cv::Rect>& asst::TemplResource::get_hist(const std::string& key) const noexcept {
    if (auto iter = m_hists.find(key);
        iter != m_hists.cend()) {
        return iter->second;
    }
    else {
        static const std::pair<cv::Mat, cv::Rect> empty;
        return empty;
    }
}

void asst::TemplResource::emplace_templ(std::string key, cv::Mat templ) {
    m_templs.emplace(std::move(key), std::move(templ));
}

void asst::TemplResource::emplace_hist(std::string key, cv::Mat hist, cv::Rect roi) {
    m_hists.emplace(std::move(key), std::make_pair(std::move(hist), std::move(roi)));
}

void asst::TemplResource::clear_hists() noexcept {
    m_hists.clear();
}