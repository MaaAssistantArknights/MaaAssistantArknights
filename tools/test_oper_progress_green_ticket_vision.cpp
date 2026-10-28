// Link against a Debug MaaCore build and its OpenCV dependency.
// Arguments: repository root, writable log directory.

#include "AsstCaller.h"
#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Vision/Miscellaneous/MaterialSynthesisImageAnalyzer.h"
#include <cassert>
#include <iostream>

int main(int argc, char** argv)
{
    assert(argc == 3);
    assert(AsstSetUserDir(argv[2]));
    assert(AsstLoadResource(argv[1]));
    auto handle = AsstCreate();
    assert(handle);
    assert(asst::ItemData.get_item_formula("30013").at("30012") == 5);
    assert(asst::Task.get("GreenTicket@Store@ReplenishSecondFloor")->next.empty());
    assert(asst::Task.get("GreenTicket@Store@ReplenishPurchasedConfirm")->next.empty());

    const auto& templ = asst::TemplResource::get_instance().get_templ("30073");
    assert(!templ.empty());
    const asst::Rect item_roi { 414, 505, 188, 184 };
    const asst::Rect quantity_roi { 460, 658, 96, 32 };
    auto observe = [&](const std::string& quantity, const std::vector<std::string>& candidates) {
        cv::Mat image(720, 1280, CV_8UC3, cv::Scalar(32, 32, 32));
        templ.copyTo(image(cv::Rect(454, 546, templ.cols, templ.rows)));
        cv::putText(
            image,
            quantity,
            cv::Point(473, 681),
            cv::FONT_HERSHEY_SIMPLEX,
            0.65,
            cv::Scalar(255, 255, 255),
            1,
            cv::LINE_AA);
        return asst::MaterialSynthesisImageAnalyzer::observe_missing_material(
            image,
            "OperProgress@RefillMaterial",
            candidates,
            item_roi,
            quantity_roi);
    };
    const auto missing = observe("0/6", asst::ItemData.get_ordered_material_item_id());
    assert(missing && missing->item_id == "30073" && missing->owned == 0 && missing->required == 6);
    assert(!observe("6", { "30073" }));
    assert(!observe("0/6/9", { "30073" }));
    assert(!observe("6/6", { "30073" }));
    assert(!observe("0/6", { "30073", "30073" }));

    for (int value : { 1, 17, 99 }) {
        cv::Mat dialog(720, 1280, CV_8UC3, cv::Scalar(255, 255, 255));
        cv::putText(
            dialog,
            std::to_string(value),
            cv::Point(906, 439),
            cv::FONT_HERSHEY_SIMPLEX,
            0.7,
            cv::Scalar(0, 0, 0),
            1,
            cv::LINE_AA);
        const auto selected =
            asst::MaterialSynthesisImageAnalyzer::read_number(dialog, "GreenTicket@Store@ReplenishPurchaseQuantity");
        assert(selected == value);
    }
    AsstDestroy(handle);
    std::cout << "Vision regressions passed: valid shortfall, incomplete ratio, malformed ratio, sufficient stock, "
                 "ambiguous icon, 1/17/99 selected counts.\n";
}
