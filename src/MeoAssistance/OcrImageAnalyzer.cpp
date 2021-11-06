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

#include "OcrImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "Logger.hpp"
#include "Resource.h"

bool asst::OcrImageAnalyzer::analyze() {
    m_ocr_result.clear();

    std::vector<TextRectProc> preds_vec;

    if (!m_replace.empty()) {
        TextRectProc text_replace = [&](TextRect& tr) -> bool {
            for (const auto& [old_str, new_str] : m_replace) {
                tr.text = utils::string_replace_all(tr.text, old_str, new_str);
            }
            return true;
        };
        preds_vec.emplace_back(text_replace);
    }

    if (!m_required.empty()) {
        if (m_full_match) {
            TextRectProc required_match = [&](TextRect& tr) -> bool {
                return std::find(m_required.cbegin(), m_required.cend(), tr.text) != m_required.cend();
            };
            preds_vec.emplace_back(required_match);
        }
        else {
            TextRectProc required_search = [&](TextRect& tr) -> bool {
                auto is_sub = [&tr](const std::string& str) -> bool {
                    return tr.text.find(str) != std::string::npos;
                };
                return std::find_if(m_required.cbegin(), m_required.cend(), is_sub) != m_required.cend();
            };
            preds_vec.emplace_back(required_search);
        }
    }

    preds_vec.emplace_back(m_pred);

    TextRectProc all_pred = [&](TextRect& tr) -> bool {
        for (auto pred : preds_vec) {
            if (pred && !pred(tr)) {
                return false;
            }
        }
        return true;
    };
    m_ocr_result = resource.ocr().recognize(m_image, m_roi, all_pred);
    //log.trace("ocr result", m_ocr_result);
    if (!m_ocr_result.empty()) {
        return true;
    }
    else {
        return false;
    }
}