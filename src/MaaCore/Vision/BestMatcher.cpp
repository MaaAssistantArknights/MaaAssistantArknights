#include "BestMatcher.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Matcher.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

using namespace asst;

void BestMatcher::append_templ(std::string name, const cv::Mat& templ)
{
    m_templs.emplace_back(TemplInfo { std::move(name), templ });
}

BestMatcher::ResultOpt BestMatcher::analyze() const
{
    Matcher match_analyzer(m_image, m_roi);
    match_analyzer.set_params(m_params);
#ifdef ASST_DEBUG
    match_analyzer.set_log_tracing(m_log_tracing);
#else
    match_analyzer.set_log_tracing(false);
#endif

    Result result;
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
        if (result.score < cur_matched.score) {
            result = Result { .rect = cur_matched.rect, .score = cur_matched.score, .templ_info = templ_info };
        }
    }

    if (!result.score) {
        return std::nullopt;
    }

    if (m_log_tracing) {
        Log.trace("The best match is", result.to_string(), result.templ_info.name);
    }
    m_result = std::move(result);
    return m_result;
}
