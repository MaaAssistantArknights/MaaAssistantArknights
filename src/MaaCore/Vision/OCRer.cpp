#include "OCRer.h"

#include <shared_mutex>
#include <unordered_map>

#include <boost/regex.hpp>

#include "Config/Miscellaneous/OcrConfig.h"
#include "Config/Miscellaneous/OcrPack.h"
#include "Config/TaskData.h"
#include "MaaUtils/Encoding.h"
#include "Utils/Logger.hpp"

using namespace asst;

OCRer::ResultsVecOpt OCRer::analyze() const
{
    OcrPack* ocr_ptr = nullptr;
    if (m_params.use_char_model) {
        ocr_ptr = &CharOcr::get_instance();
    }
    else {
        ocr_ptr = &WordOcr::get_instance();
    }
    ResultsVec raw_results = ocr_ptr->recognize(make_roi(m_image, m_roi), m_params.without_det, m_roi);
    ocr_ptr = nullptr;

    /* post process */
    ResultsVec results_vec;
    for (Result& res : raw_results) {
        if (res.text.empty() || std::isnan(res.score) || std::isinf(res.score)) {
            continue;
        }

        postproc_rect_(res);
        postproc_trim_(res);
        postproc_replace_(res);

        if (!filter_and_replace_by_required_(res)) {
            continue;
        }

        results_vec.emplace_back(std::move(res));
    }

    if (results_vec.empty()) {
        return std::nullopt;
    }

    Log.trace("Proceed", results_vec);

    m_result = std::move(results_vec);
    return m_result;
}

void OCRer::postproc_rect_(Result& res) const
{
    if (m_params.without_det) {
        res.rect = m_roi;
    }
    else {
        res.rect.x += m_roi.x;
        res.rect.y += m_roi.y;
    }
}

void OCRer::postproc_trim_(Result& res) const
{
    utils::string_trim(res.text);
}

static const boost::wregex& gen_regex(const std::wstring& pattern)
{
    static std::shared_mutex mtx;
    static std::unordered_map<std::wstring, boost::wregex> s_cache;

    {
        std::shared_lock slock(mtx);
        if (auto it = s_cache.find(pattern); it != s_cache.end()) {
            return it->second;
        }
    }

    std::unique_lock ulock(mtx);
    return s_cache.emplace(pattern, boost::wregex(pattern)).first->second;
}

void OCRer::postproc_replace_(Result& res) const
{
    if (m_params.replace.empty()) {
        return;
    }

    std::wstring text_u16 = MAA_NS::to_u16(res.text);
    for (const auto& [regex, new_str] : m_params.replace) {
        std::wstring regex_u16 = MAA_NS::to_u16(regex);
        std::wstring new_str_u16 = MAA_NS::to_u16(new_str);
        if (m_params.replace_full) {
            if (boost::regex_search(text_u16, gen_regex(regex_u16))) {
                text_u16 = new_str_u16;
            }
        }
        else {
            text_u16 = boost::regex_replace(text_u16, gen_regex(regex_u16), new_str_u16);
        }
    }
    res.text = MAA_NS::from_u16(text_u16);
}

bool OCRer::filter_and_replace_by_required_(Result& res) const
{
    if (m_params.required.empty()) {
        return true;
    }
    auto& ocr_config = OcrConfig::get_instance();
    auto equ_text = ocr_config.process_equivalence_class(res.text);

    if (m_params.full_match) {
        auto required = m_params.required | std::views::transform([&](const auto& str) { return str.second; });
        return std::ranges::find(required, equ_text) != required.end();
    }
    else {
        auto is_sub = [&](const auto& p) -> bool {
            if (equ_text.find(p.second) == std::string::npos) {
                return false;
            }
            res.text = p.first;
            return true;
        };
        return std::ranges::find_if(m_params.required, is_sub) != m_params.required.cend();
    };
}
