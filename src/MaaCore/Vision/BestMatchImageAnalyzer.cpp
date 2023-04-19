#include "BestMatchImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "MatchImageAnalyzer.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

void asst::BestMatchImageAnalyzer::append_templ(std::string name, const cv::Mat& templ)
{
    m_templs.emplace_back(TemplInfo { std::move(name), templ });
}

const asst::BestMatchImageAnalyzer::ResultOpt& asst::BestMatchImageAnalyzer::analyze()
{
    m_result = std::nullopt;

    MatchImageAnalyzer match_analyzer(m_image, m_roi);
    match_analyzer.set_params(m_params);
#ifdef ASST_DEBUG
    match_analyzer.set_log_tracing(m_log_tracing);
#else
    match_analyzer.set_log_tracing(false);
#endif

    for (const auto& templ_info : m_templs) {
        auto&& [name, templ] = templ_info;

        if (templ.empty()) {
            match_analyzer.set_templ(name);
        }
        else {
            match_analyzer.set_templ(templ);
        }

        const auto& cur_opt = match_analyzer.analyze();
        if (!cur_opt) {
            continue;
        }
        const auto& cur_matched = cur_opt.value();
        if (!m_result || m_result->matched.score < cur_matched.score) {
            m_result = Result { .matched = cur_matched, .templ_info = templ_info };
        }
    }

    if (!m_result) {
        return std::nullopt;
    }

    if (m_log_tracing) {
        Log.trace("The best match is", m_result->matched.to_string(), m_result->templ_info.name);
    }
    return m_result;
}

const asst::BestMatchImageAnalyzer::ResultOpt& asst::BestMatchImageAnalyzer::result() const noexcept
{
    return m_result;
}

void asst::BestMatchImageAnalyzer::_set_roi(const Rect& roi)
{
    AbstractImageAnalyzer::set_roi(roi);
}
