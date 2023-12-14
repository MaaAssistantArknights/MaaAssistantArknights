#include "InfrastFacilityImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/MultiMatcher.h"

bool asst::InfrastFacilityImageAnalyzer::analyze()
{
    const static std::unordered_map<std::string, std::string> facility_task_name = {
        { "Dorm", "InfrastDorm" },           { "Control", "InfrastControl" },       { "Mfg", "InfrastMfg" },
        { "Trade", "InfrastTrade" },         { "Power", "InfrastPower" },           { "Office", "InfrastOffice" },
        { "Reception", "InfrastReception" }, { "Processing", "InfrastProcessing" }, { "Training", "InfrastTraining" }
    };
    // 因为基建的缩放是不确定的，有可能是正常大小，也可能是最小化的
    // 所以对每种情况都进行一下识别，取其中得分最高的
    const static std::vector<std::string> task_name_suffix = { "", "Mini" };

    int cor_suffix_index = -1;

    for (const auto& key : m_to_be_analyzed) {
        auto find_iter = facility_task_name.find(key);
        if (find_iter == facility_task_name.cend()) {
            Log.error("facility name error", key);
            continue;
        }
        std::string task_name = find_iter->second;
        std::vector<MatchRect> cur_facility_result;
        // 已知基建缩放状态的时候，只识别这个缩放状态下的就行了
        // 否则识别所有状态，直到找出正确的当前缩放状态
        if (cor_suffix_index >= 0) {
            MultiMatcher mm_analyzer(m_image);
            mm_analyzer.set_task_info(task_name + task_name_suffix.at(cor_suffix_index));
            if (!mm_analyzer.analyze()) {
                continue;
            }
            cur_facility_result = mm_analyzer.get_result();
        }
        else {
            double max_score = 0;
            for (size_t i = 0; i != task_name_suffix.size(); ++i) {
                MultiMatcher mm_analyzer(m_image);
                mm_analyzer.set_task_info(task_name + task_name_suffix.at(i));
                if (!mm_analyzer.analyze()) {
                    continue;
                }
                const auto& cur_res = mm_analyzer.get_result();
                auto cur_max_iter = ranges::max_element(cur_res, std::less {}, std::mem_fn(&MatchRect::score));
                if (cur_max_iter == cur_res.cend()) {
                    continue;
                }
                if (double cur_score = cur_max_iter->score; max_score < cur_score) {
                    max_score = cur_score;
                    cur_facility_result = cur_res;
                    cor_suffix_index = static_cast<int>(i);
                }
            }
        }
        if (cur_facility_result.empty()) {
            continue;
        }

        sort_by_horizontal_(cur_facility_result);

#ifdef ASST_DEBUG
        cv::RNG rng(time(0));
        cv::Scalar rand_color(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
        int index = 0;
        for (const MatchRect& mr : cur_facility_result) {
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(mr.rect), rand_color, 2);
            cv::Point text_point(mr.rect.x + 2, mr.rect.y + mr.rect.height / 2);
            cv::putText(m_image_draw, task_name + std::to_string(index++), text_point, 1, 1, cv::Scalar(0, 0, 255), 1);
        }
#endif
        m_result.emplace(key, std::move(cur_facility_result));
    }

    return !m_result.empty();
}
