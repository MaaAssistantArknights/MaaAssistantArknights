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

#include "ProcessTaskImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "Resource.h"

asst::ProcessTaskImageAnalyzer::ProcessTaskImageAnalyzer(const cv::Mat& image, std::vector<std::string> tasks_name)
    : AbstractImageAnalyzer(image),
      m_tasks_name(std::move(tasks_name)) {
    ;
}

asst::ProcessTaskImageAnalyzer::~ProcessTaskImageAnalyzer() = default;

bool asst::ProcessTaskImageAnalyzer::match_analyze(std::shared_ptr<TaskInfo> task_ptr) {
    if (!m_match_analyzer) {
        m_match_analyzer = std::make_unique<MatchImageAnalyzer>(m_image);
    }
    const auto match_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(task_ptr);
    m_match_analyzer->set_task_info(*match_task_ptr);

    if (m_match_analyzer->analyze()) {
        m_result = match_task_ptr;
        m_result_rect = m_match_analyzer->get_result().rect;
        return true;
    }
    return false;
}

bool asst::ProcessTaskImageAnalyzer::ocr_analyze(std::shared_ptr<TaskInfo> task_ptr) {
    std::shared_ptr<OcrTaskInfo> ocr_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(task_ptr);

    // 先尝试从缓存的结果里找
    for (const TextRect& tr : m_ocr_cache) {
        TextRect temp = tr;
        for (const auto& [old_str, new_str] : ocr_task_ptr->replace_map) {
            temp.text = utils::string_replace_all(temp.text, old_str, new_str);
        }
        for (const auto& text : ocr_task_ptr->text) {
            bool flag = false;
            if (ocr_task_ptr->need_full_match) {
                if (temp.text == text) {
                    flag = true;
                }
            }
            else if (temp.text.find(text) != std::string::npos) {
                flag = true;
            }
            // 即使找到了，也得在ROI里才行，不然过滤掉
            if (flag && ocr_task_ptr->roi.include(tr.rect)) {
                m_result = ocr_task_ptr;
                m_result_rect = tr.rect;
                log.trace("ProcessTaskImageAnalyzer::ocr_analyze | found in cache", tr.to_string());
                return true;
            }
        }
    }
    if (!m_ocr_analyzer) {
        m_ocr_analyzer = std::make_unique<OcrImageAnalyzer>(m_image);
    }
    m_ocr_analyzer->set_required(ocr_task_ptr->text);
    m_ocr_analyzer->set_replace(ocr_task_ptr->replace_map);

    // 识别区域文字，并加入缓存
    auto analyze_roi = [&](const Rect& roi, bool is_appeared = false) -> bool {
        m_ocr_analyzer->set_roi(roi);
        bool ret = m_ocr_analyzer->analyze();

        const auto& ocr_result = m_ocr_analyzer->get_result();
        m_ocr_cache.insert(m_ocr_cache.end(), ocr_result.begin(), ocr_result.end());
        if (ret) {
            auto& res = ocr_result.front();
            m_result = ocr_task_ptr;
            m_result_rect = res.rect;
            if (!is_appeared) {
                ocr_task_ptr->region_of_appeared.emplace(res.rect);
            }
            log.trace("ProcessTaskImageAnalyzer::ocr_analyze | found", res.to_string(), "roi :", roi.to_string());
            return true;
        }
        return false;
    };

    // 在曾经识别到过的历史区域里识别
    for (const Rect& region : ocr_task_ptr->region_of_appeared) {
        static auto& max_width = resource.cfg().WindowWidthDefault;
        static auto& max_height = resource.cfg().WindowHeightDefault;
        bool ret = analyze_roi(region.center_zoom(2.0, max_width, max_height), true);
        if (ret) {
            log.trace("ProcessTaskImageAnalyzer::ocr_analyze | found in appeared");
            return true;
        }
    }

    // 都没有的话，再完整的识别整个ROI
    return analyze_roi(ocr_task_ptr->roi);
}

void asst::ProcessTaskImageAnalyzer::reset() noexcept {
    m_ocr_cache.clear();
    m_ocr_analyzer = nullptr;
    m_match_analyzer = nullptr;
}

bool asst::ProcessTaskImageAnalyzer::analyze() {
    m_result = nullptr;
    m_result_rect = Rect();

    for (const std::string& task_name : m_tasks_name) {
        auto task_ptr = resource.task().task_ptr(task_name);

        switch (task_ptr->algorithm) {
        case AlgorithmType::JustReturn:
            m_result = task_ptr;
            return true;
        case AlgorithmType::MatchTemplate:
            if (match_analyze(task_ptr)) {
                return true;
            }
            break;
        case AlgorithmType::OcrDetect:
            if (ocr_analyze(task_ptr)) {
                return true;
            }
            break;
        default:
            break;
        }
    }
    return false;
}

void asst::ProcessTaskImageAnalyzer::set_image(const cv::Mat& image) {
    AbstractImageAnalyzer::set_image(image);
    reset();
}