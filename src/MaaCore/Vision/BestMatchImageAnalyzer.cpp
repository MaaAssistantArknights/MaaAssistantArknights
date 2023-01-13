#include "BestMatchImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

bool asst::BestMatchImageAnalyzer::analyze()
{
    set_log_tracing(false);
    set_use_cache(false);

    MatchRect best_matched;
    for (const auto& [name, templ] : m_templs) {
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
            m_result_name = name;
        }
    }
    m_result = best_matched;

    Log.trace("The best match is", best_matched.to_string(), m_result_name);
    return m_result.score > 0;
}

void asst::BestMatchImageAnalyzer::append_templ(std::string name, const cv::Mat& templ)
{
    m_templs.emplace_back(name, templ);
}
