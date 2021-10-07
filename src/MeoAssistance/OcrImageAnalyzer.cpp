#include "OcrImageAnalyzer.h"

#include "Resource.h"
#include "AsstAux.h"
#include "Logger.hpp"

bool asst::OcrImageAnalyzer::analyze()
{
    std::vector<TextRectProc> preds_vec;

    if (!m_replace.empty()) {
        TextRectProc text_replace = [&](TextRect& tr) -> bool {
            for (const auto& [old_str, new_str] : m_replace) {
                tr.text = StringReplaceAll(tr.text, old_str, new_str);
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