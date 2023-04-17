#include "RoleRecognitionTask.h"

#include "Utils/Ranges.hpp"

#include "Config/Miscellaneous/RoleConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"

bool asst::RoleRecognitionTask::_run()
{
    LogTraceFunction;
    lastRole = "";
    roles.clear();
    // lastRole = RoleData.get_role_tag();
    while (!need_exit()) {
        auto resRole = analyzer_opers();
        if (resRole.back().text == lastRole) {
            break;
        }
        else {
            lastRole = resRole.back().text;
            for (const auto& res : resRole) {
                const std::string& name = res.text;
                roles.insert(name);
            }
        }
        swipe_page();
        // callback_analyze_result(true);
    }
    callback_analyze_result(true);
    return true;
}

std::vector<asst::TextRect> asst::RoleRecognitionTask::analyzer_opers()
{
    std::vector<std::string> cRole;
    cRole.clear();
    auto image = ctrler()->get_image();
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");

    OcrWithFlagTemplImageAnalyzer name_analyzer(image);
    // 识别第一行
    name_analyzer.set_task_info("RoleOperNameFlag", "RoleOCR");
    name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
    name_analyzer.analyze();

    ////识别第二行
    OcrWithFlagTemplImageAnalyzer name_analyzer2(image);
    name_analyzer2.set_task_info("RoleOperNameFlag1", "RoleOCR");
    name_analyzer2.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
    name_analyzer2.analyze();

    auto opers_result = name_analyzer.get_result();
    auto opers_res = name_analyzer2.get_result();

    if (opers_result.empty() || opers_res.empty()) {
        return {};
    }
    opers_result.insert(opers_result.end(), opers_res.begin(), opers_res.end());
    // 按位置排个序
    // ranges::sort(opers_result, [](const TextRect& lhs, const TextRect& rhs) -> bool {
    //    if (std::abs(lhs.rect.x - rhs.rect.x) < 5) { // x差距较小则理解为是同一列的，按y排序
    //        return lhs.rect.y < rhs.rect.y;
    //    }
    //    else {
    //        return lhs.rect.x < rhs.rect.x; // 否则按x排序
    //    }
    //});
    // save_img(utils::path("debug") / utils::path("role"));

    /*for (const auto& res : opers_result) {
        const std::string& name = res.text;
        json::value info = basic_info_with_what("RoleInfo");
        auto& details = info["details"];
        details["name"] = name;
        callback(AsstMsg::SubTaskExtraInfo, info);
    }*/
    return opers_result;
}

void asst::RoleRecognitionTask::swipe_page()
{
    ProcessTask(*this, { "RoleSlowlySwipeToTheRight" }).run();
}
bool asst::RoleRecognitionTask::isLastRole(std::string name)
{
    return name == lastRole;
}
void asst::RoleRecognitionTask::callback_analyze_result(bool done)
{
    LogTraceFunction;
    // 获取所有role
    const auto& all_roles = RoleData.get_all_role_name();
    size_t total = all_roles.size();
    // 如果有U-Official,识别出来为U-official,需要替换
    if (roles.find("U-official") != roles.end()) {
        roles.erase("U-official");
        roles.insert("U-Official");
    }
    // 去掉不存在的干员名
    for (const auto& r : roles) {
        if (all_roles.find(r) == all_roles.end()) roles.erase(r);
    }

    // 识别干员个数
    size_t roleHave = roles.size();
    // 未拥有干员名
    std::list<std::string> noRoles;
    noRoles.clear();
    for (const auto& r : all_roles) {
        if (roles.find(r) == roles.end()) noRoles.push_back(r);
    }

    json::value info = basic_info_with_what("RoleInfo");
    auto& details = info["details"];
    details["done"] = done;
    details["total"] = total;
    details["have"] = roleHave;
    details["nroles"] = json::array(noRoles);
    // details["allroles"] = json::array(all_roles);
    // details["hroles"] = json::array(roles);

    callback(AsstMsg::SubTaskExtraInfo, info);
}
