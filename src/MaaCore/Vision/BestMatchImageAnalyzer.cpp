#include "BestMatchImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

bool asst::BestMatchImageAnalyzer::analyze()
{
#ifndef ASST_DEBUG
    MatchImageAnalyzer::set_log_tracing(false);
#endif
    set_use_cache(false);

    MatchRect best_matched;
    for (const auto& templ_info : m_templs) {
        auto&& [name, templ] = templ_info;
        if (templ.empty()) {
            set_templ_name(name);
        }
        else {
            set_templ(templ);
        }

        if (!MatchImageAnalyzer::analyze()) {
            continue;
        }
        const auto& cur_matched = MatchImageAnalyzer::get_result();
        if (best_matched.score < cur_matched.score) {
            best_matched = cur_matched;
            m_result = templ_info;
        }
    }

    if (m_best_log_tracing) {
        Log.trace("The best match is", best_matched.to_string(), m_result.name);
    }
    return best_matched.score > 0;
}

void asst::BestMatchImageAnalyzer::set_log_tracing(bool enable)
{
    m_best_log_tracing = enable;
}

void asst::BestMatchImageAnalyzer::append_templ(std::string name, const cv::Mat& templ)
{
    m_templs.emplace_back(TemplInfo { std::move(name), templ });
}
