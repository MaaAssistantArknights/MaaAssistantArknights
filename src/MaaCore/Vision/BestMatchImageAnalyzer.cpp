#include "BestMatchImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

const asst::BestMatchImageAnalyzer::ResultOpt& asst::BestMatchImageAnalyzer::analyze()
{
#ifndef ASST_DEBUG
    MatchImageAnalyzer::set_log_tracing(false);
#endif

    m_best_result = std::nullopt;

    MatchImageAnalyzer::Result best_matched;
    for (const auto& templ_info : m_templs) {
        auto&& [name, templ] = templ_info;

        if (templ.empty()) {
            set_templ(name);
        }
        else {
            set_templ(templ);
        }

        const auto& cur_opt = MatchImageAnalyzer::analyze();
        if (!cur_opt) {
            continue;
        }
        const auto& cur_matched = cur_opt.value();
        if (best_matched.score < cur_matched.score) {
            best_matched = cur_matched;
            m_best_result = templ_info;
        }
    }

    if (!m_best_result) {
        return std::nullopt;
    }

    if (m_best_log_tracing) {
        Log.trace("The best match is", best_matched.to_string(), m_best_result.value().name);
    }
    return m_best_result;
}

void asst::BestMatchImageAnalyzer::set_log_tracing(bool enable)
{
    m_best_log_tracing = enable;
}

void asst::BestMatchImageAnalyzer::append_templ(std::string name, const cv::Mat& templ)
{
    m_templs.emplace_back(TemplInfo { std::move(name), templ });
}

const asst::BestMatchImageAnalyzer::ResultOpt& asst::BestMatchImageAnalyzer::result() const noexcept
{
    return m_best_result;
}
