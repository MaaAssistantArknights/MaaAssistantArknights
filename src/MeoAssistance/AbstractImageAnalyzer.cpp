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

#include "AbstractImageAnalyzer.h"

#include "AsstUtils.hpp"

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat& image)
    : m_image(image), m_roi(empty_rect_to_full(Rect(), image))
#ifdef LOG_TRACE
      ,
      m_image_draw(image.clone())
#endif
{
}

asst::AbstractImageAnalyzer::AbstractImageAnalyzer(const cv::Mat& image, const Rect& roi)
    : m_image(image),
      m_roi(empty_rect_to_full(roi, image))
#ifdef LOG_TRACE
      ,
      m_image_draw(image.clone())
#endif
{
    ;
}

void asst::AbstractImageAnalyzer::set_image(const cv::Mat& image) {
    m_image = image;
#ifdef LOG_TRACE
    m_image_draw = image.clone();
#endif
}

void asst::AbstractImageAnalyzer::set_image(const cv::Mat& image, const Rect& roi) {
    set_image(image);
    m_roi = empty_rect_to_full(roi, image);
}

void asst::AbstractImageAnalyzer::set_roi(const Rect& roi) noexcept {
    m_roi = empty_rect_to_full(roi, m_image);
}

asst::Rect asst::AbstractImageAnalyzer::empty_rect_to_full(const Rect& rect, const cv::Mat& image) noexcept {
    return rect.empty() ? Rect(0, 0, image.cols, image.rows) : rect;
}

std::string asst::AbstractImageAnalyzer::calc_hash() const {
    return calc_hash(m_roi);
}

std::string asst::AbstractImageAnalyzer::calc_hash(const Rect& roi) const {
    constexpr static int HashKernelSize = 16;
    cv::Mat image_roi = m_image(utils::make_rect<cv::Rect>(roi));

    cv::resize(image_roi, image_roi, cv::Size(HashKernelSize, HashKernelSize));
    cv::cvtColor(image_roi, image_roi, cv::COLOR_BGR2GRAY);
    std::stringstream hash_value;
    uchar* pix = image_roi.data;
    int tmp_dec = 0;
    for (int ro = 0; ro < 256; ro++) {
        tmp_dec = tmp_dec << 1;
        if ((*pix) >= 128)
            tmp_dec++;
        if (ro % 4 == 3) {
            hash_value << std::hex << tmp_dec;
            tmp_dec = 0;
        }
        pix++;
    }
    return hash_value.str();
}
