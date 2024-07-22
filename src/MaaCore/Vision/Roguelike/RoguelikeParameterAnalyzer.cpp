#include "RoguelikeParameterAnalyzer.h"

#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"

bool asst::RoguelikeParameterAnalyzer::analyze()
{
    LogTraceFunction;    
}

int asst::RoguelikeParameterAnalyzer::update_chaos(const cv::Mat& image)
{
    return get_number(image, "Sami@Roguelike@SpecialValRecognition");
}

int asst::RoguelikeParameterAnalyzer::update_idea_count(const cv::Mat& image)
{
    return get_number(image, "Sarkaz@Roguelike@SpecialValRecognition");
}

int asst::RoguelikeParameterAnalyzer::get_number(const cv::Mat& image, const std::string& task_name)
{
    int val = 0;
    OCRer analyzer(image);
    analyzer.set_task_info(task_name);
    analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);
    analyzer.set_use_char_model(true);

    if (!analyzer.analyze()) {
        return 0;
    }
    utils::chars_to_number(analyzer.get_result().front().text, val);

    return val;
}

int asst::RoguelikeParameterAnalyzer::update_hp(const cv::Mat& image)
{
    int hp_val;
    asst::OCRer analyzer(image);
    analyzer.set_task_info("Roguelike@HpRecognition");

    auto res_vec_opt = analyzer.analyze();
    if (!res_vec_opt) {
        return -1;
    }
    return utils::chars_to_number(res_vec_opt->front().text, hp_val) ? hp_val : 0;
}
