#include "BestMatcher.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Matcher.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

MAA_VISION_NS_BEGIN

void BestMatcher::append_templ(std::string name, const cv::Mat& templ)
{
    m_templs.emplace_back(TemplInfo { std::move(name), templ });
}

BestMatcher::ResultOpt BestMatcher::analyze() const
{
    Matcher match_analyzer(m_image, m_roi, m_inst);
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
        if (result.matched.score < cur_matched.score) {
            result = Result { .matched = cur_matched, .templ_info = templ_info };
        }
    }

    if (!result.matched.score) {
        return std::nullopt;
    }

    if (m_log_tracing) {
        Log.trace("The best match is", result.matched.to_string(), result.templ_info.name);
    }
    return result;
}

MAA_VISION_NS_END
