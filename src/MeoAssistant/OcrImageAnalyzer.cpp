#include "OcrImageAnalyzer.h"

#include <regex>

#include "Logger.hpp"
#include "Resource.h"
#include "AipOcr.h"

bool asst::OcrImageAnalyzer::analyze()
{
    m_ocr_result.clear();

    std::vector<TextRectProc> preds_vec;

    if (!m_replace.empty()) {
        TextRectProc text_replace = [&](TextRect& tr) -> bool {
            for (const auto& [regex, new_str] : m_replace) {
                tr.text = std::regex_replace(tr.text, std::regex(regex), new_str);
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

    auto& aip_ocr = Resrc.cfg().get_options().aip_ocr;
    bool need_local = true;
    if (aip_ocr.enable) {
        if (aip_ocr.accurate) {
            need_local = !AipOcr::get_instance().request_ocr_accurate(m_image, m_ocr_result, all_pred);
        }
        else {
            need_local = !AipOcr::get_instance().request_ocr_general(m_image, m_ocr_result, all_pred);
        }
    }
    if (need_local) {
        m_ocr_result = Resrc.ocr().recognize(m_image, m_roi, all_pred, m_without_det);
    }
    //log.trace("ocr result", m_ocr_result);
    return !m_ocr_result.empty();
}
