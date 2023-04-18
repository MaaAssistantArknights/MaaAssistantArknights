#include "OperRecognitionTask.h"

#include "Utils/Ranges.hpp"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"

bool asst::OperRecognitionTask::_run()
{
    LogTraceFunction;
    std::string current_page_last_oper_name = "";
    own_opers.clear();
    
    while (!need_exit()) {
        auto resOpers = analyzer_opers();
        if (resOpers.back().name == current_page_last_oper_name) {
            break;
        }
        else {
            current_page_last_oper_name = resOpers.back().name;
            own_opers.insert(own_opers.end(),resOpers.begin(),resOpers.end());
        }
        swipe_page();
        // callback_analyze_result(true);
    }
    callback_analyze_result(true);
    return true;
}

std::vector<asst::OperRecognitionTask::OperBoxInfo> asst::OperRecognitionTask::analyzer_opers()
{
    std::vector<OperBoxInfo> current_page_opers;
    current_page_opers.clear();
    auto image = ctrler()->get_image();
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");

    OcrWithFlagTemplImageAnalyzer oper_name_analyzer(image);
    // 识别第一行
    //name_analyzer.set_task_info("OperatorNameFlag", "OperatorOCR");
    oper_name_analyzer.set_task_info("OperatorNameFlagLV", "OperatorOCRLV");
    oper_name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
    oper_name_analyzer.analyze();

    auto oper_names_result = oper_name_analyzer.get_result();
    
    //if (opers_result.empty() || opers_res.empty()) {
    if (oper_names_result.empty()) {
        return {};
    }
    
    for (auto& opername : oper_names_result) {
        OperBoxInfo oper_info;
        oper_info.name = std::move(opername.text);
        current_page_opers.push_back(oper_info);
    }
    
    return current_page_opers;
}

void asst::OperRecognitionTask::swipe_page()
{
    ProcessTask(*this, { "OperatorSlowlySwipeToTheRight" }).run();
}

void asst::OperRecognitionTask::callback_analyze_result(bool done)
{
    LogTraceFunction;
    // 获取所有干员名
    const auto& all_oper_names = BattleData.get_all_oper_names();
    //size_t total = all_oper_names.size();
    
    // 获取识别干员名
    const auto own_oper_names = get_own_oper_names();
    // 识别干员个数
    //size_t own_count = own_oper_names.size();
    // 未拥有干员名
    std::unordered_set<std::string> n_own_oper_names;
    n_own_oper_names.clear();
    for (const auto& oper : all_oper_names) {
        if (own_oper_names.find(oper) == own_oper_names.end()) n_own_oper_names.insert(oper);
    }

    json::value info = basic_info_with_what("OperInfo");
    auto& details = info["details"];
    details["done"] = done;
    //details["total"] = total;
    //details["owncount"] = own_count;
    details["have"] = json::array(own_oper_names);
    details["nhave"] = json::array(n_own_oper_names);
    details["all"] = json::array(all_oper_names);
    // details["hroles"] = json::array(roles);

    callback(AsstMsg::SubTaskExtraInfo, info);
}

std::unordered_set<std::string> asst::OperRecognitionTask::get_own_oper_names()
{
    LogTraceFunction;
    std::unordered_set<std::string> own_oper_names;
    
    for (auto& oper_info : own_opers) {
        own_oper_names.insert(std::move(oper_info.name));
    }
    return own_oper_names;
}
