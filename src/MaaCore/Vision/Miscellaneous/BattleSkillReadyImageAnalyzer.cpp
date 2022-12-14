#include "BattleSkillReadyImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include <algorithm>
#include <functional>
#include <vector>

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"

bool asst::BattleSkillReadyImageAnalyzer::analyze()
{
    auto task_ptr = Task.get<MatchTaskInfo>("BattleSkillReady");
    const cv::Mat& templ = TemplResource::get_instance().get_templ(task_ptr->templ_name);

    auto key_color = [](cv::InputArray src, cv::OutputArray dst, const cv::Scalar& color,
                        const cv::Scalar& tolerance = {}) {
        cv::inRange(src, color - tolerance, color + tolerance, dst);
    };
    auto preprocess = [&](cv::Mat& img) {
        cv::Mat mask_y2;
        key_color(img, mask_y2, { 40, 94, 103 }, { 10, 10, 10 }); // BGR
        img.setTo(cv::Scalar { 2, 216, 255 }, mask_y2);           // BGR
        cv::Mat mask_y1;
        key_color(img, mask_y1, { 2, 216, 255 }, { 20, 20, 20 });
        cv::dilate(mask_y1, mask_y1, cv::getStructuringElement(cv::MORPH_RECT, { 4, 4 }));
        cv::bitwise_not(mask_y1, mask_y1);
        img.setTo(cv::Scalar { 0, 0, 0 }, mask_y1);
    };

    cv::Mat tmp = templ.clone();
    cv::Mat img = m_image.clone()(make_rect<cv::Rect>(m_roi));

    cv::Mat template_mask;
    key_color(tmp, template_mask, { 0, 255, 0 }, { 0, 0, 0 });
    cv::bitwise_not(template_mask, template_mask);

    preprocess(img);
    preprocess(tmp);

    cv::Mat match;
    cv::matchTemplate(img, tmp, match, cv::TM_SQDIFF, template_mask);
    match /= template_mask.cols * template_mask.rows;
    cv::sqrt(match, match);

    // you may use something similar to MultiMatchImageAnalyzer instead
    double min_val = 255.;
    cv::minMaxLoc(match, &min_val);
    return min_val < 130.;
}

void asst::BattleSkillReadyImageAnalyzer::set_base_point(const Point& pt)
{
    auto task_ptr = Task.get<MatchTaskInfo>("BattleSkillReady");
    const Rect& skill_roi_move = task_ptr->rect_move;

    set_roi(Rect(pt.x, pt.y, 0, 0).move(skill_roi_move));
}
