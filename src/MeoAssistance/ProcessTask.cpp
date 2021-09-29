#include "ProcessTask.h"

#include <chrono>
#include <random>

#include <opencv2/opencv.hpp>

#include "AsstAux.h"
#include "Configer.h"
#include "TaskConfiger.h"
#include "WinMacro.h"
#include "Identify.h"

using namespace asst;

bool ProcessTask::run()
{
    if (m_controller_ptr == NULL
        || m_identify_ptr == NULL)
    {
        m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
        return false;
    }

    json::value task_start_json = json::object{
        { "task_type",  "ProcessTask" },
        { "task_chain", m_task_chain},
        { "tasks", json::array(m_cur_tasks_name)}
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    std::shared_ptr<TaskInfo> task_info_ptr = nullptr;

    Rect rect;
    task_info_ptr = match_image(&rect);
    if (need_exit()) {
        return false;
    }
    if (!task_info_ptr) {
        m_callback(AsstMsg::ProcessTaskNotMatched, task_start_json, m_callback_arg);
        return false;
    }

    json::value callback_json = json::object{
        { "name", task_info_ptr->name },
        { "type", static_cast<int>(task_info_ptr->action) },
        { "exec_times", task_info_ptr->exec_times },
        { "max_times", task_info_ptr->max_times },
        { "task_type", "ProcessTask"},
        { "algorithm", static_cast<int>(task_info_ptr->algorithm)}
    };
    m_callback(AsstMsg::TaskMatched, callback_json, m_callback_arg);

    if (task_info_ptr->exec_times >= task_info_ptr->max_times) {
        m_callback(AsstMsg::ReachedLimit, callback_json, m_callback_arg);

        json::value next_json = callback_json;
        next_json["tasks"] = json::array(task_info_ptr->exceeded_next);
        next_json["retry_times"] = m_retry_times;
        next_json["task_chain"] = m_task_chain;
        m_callback(AsstMsg::AppendProcessTask, next_json, m_callback_arg);
        return true;
    }

    // 前置固定延时
    if (!sleep(task_info_ptr->pre_delay)) {
        return false;
    }

    bool need_stop = false;
    switch (task_info_ptr->action) {
    case ProcessTaskAction::ClickRect:
        rect = task_info_ptr->specific_area;
        [[fallthrough]];
    case ProcessTaskAction::ClickSelf:
        exec_click_task(rect);
        break;
    case ProcessTaskAction::ClickRand:
    {
        static const Rect full_rect(0, 0, Configer::WindowWidthDefault - 1, Configer::WindowHeightDefault - 1);
        exec_click_task(full_rect);
    } break;
    case ProcessTaskAction::SwipeToTheLeft:
    case ProcessTaskAction::SwipeToTheRight:
        exec_swipe_task(task_info_ptr->action);
        break;
    case ProcessTaskAction::DoNothing:
        break;
    case ProcessTaskAction::Stop:
        m_callback(AsstMsg::ProcessTaskStopAction, json::object{ {"task_chain", m_task_chain} }, m_callback_arg);
        need_stop = true;
        break;
    case ProcessTaskAction::PrintWindow:
        if (Configer::get_instance().m_options.print_window) {
            sleep(Configer::get_instance().m_options.print_window_delay);
            static const std::string dirname = GetCurrentDir() + "screenshot\\";
            print_window(dirname);
        }
        break;
    default:
        break;
    }

    ++task_info_ptr->exec_times;

    // 减少其他任务的执行次数
    // 例如，进入吃理智药的界面了，相当于上一次点蓝色开始行动没生效
    // 所以要给蓝色开始行动的次数减一
    for (const std::string& reduce : task_info_ptr->reduce_other_times) {
        --TaskConfiger::get_instance().m_all_tasks_info[reduce]->exec_times;
    }

    if (need_stop) {
        return true;
    }

    callback_json["exec_times"] = task_info_ptr->exec_times;
    m_callback(AsstMsg::TaskCompleted, callback_json, m_callback_arg);

    // 后置固定延时
    if (!sleep(task_info_ptr->rear_delay)) {
        return false;
    }

    json::value next_json = callback_json;
    next_json["task_chain"] = m_task_chain;
    next_json["retry_times"] = m_retry_times;
    next_json["tasks"] = json::array(task_info_ptr->next);
    m_callback(AsstMsg::AppendProcessTask, next_json, m_callback_arg);

    return true;
}

std::shared_ptr<TaskInfo> ProcessTask::match_image(Rect* matched_rect)
{
    const cv::Mat& cur_image = m_controller_ptr->get_image();
    if (cur_image.empty()) {
        m_callback(AsstMsg::ImageIsEmpty, json::value(), m_callback_arg);
        return nullptr;
    }

    std::unordered_set<TextArea> ocr_cache;
    // 逐个匹配当前可能的任务
    for (const std::string& task_name : m_cur_tasks_name) {
        if (need_exit()) {
            return nullptr;
        }
        std::shared_ptr<TaskInfo> task_info_ptr = TaskConfiger::get_instance().m_all_tasks_info[task_name];
        if (task_info_ptr == nullptr) {	// 说明配置文件里没这个任务
            m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
            continue;
        }
        json::value callback_json;
        bool matched = false;
        Rect rect;

        switch (task_info_ptr->algorithm)
        {
        case AlgorithmType::JustReturn:
            callback_json["algorithm"] = "JustReturn";
            matched = true;
            break;
        case AlgorithmType::MatchTemplate:
        {
            std::shared_ptr<MatchTaskInfo> match_task_info_ptr =
                std::dynamic_pointer_cast<MatchTaskInfo>(task_info_ptr);
            double templ_threshold = match_task_info_ptr->templ_threshold;
            double hist_threshold = match_task_info_ptr->hist_threshold;
            double add_cache_thres = match_task_info_ptr->cache ? templ_threshold : Identify::NotAddCache;
            cv::Mat identify_image;
            const auto& identify_area = m_controller_ptr->shaped_correct(match_task_info_ptr->identify_area);
            if (identify_area.width == 0) {
                identify_image = cur_image;
            }
            else {
                identify_image = cur_image(make_rect<cv::Rect>(identify_area));
            }
            auto&& [algorithm, score, temp_rect] =
                m_identify_ptr->find_image(identify_image, task_name, add_cache_thres);
            rect = std::move(temp_rect);
            rect.x += identify_area.x;
            rect.y += identify_area.y;
            callback_json["value"] = score;

            if (algorithm == AlgorithmType::MatchTemplate) {
                callback_json["threshold"] = templ_threshold;
                callback_json["algorithm"] = "MatchTemplate";
                if (score >= templ_threshold) {
                    matched = true;
                }
            }
            else if (algorithm == AlgorithmType::CompareHist) {
                callback_json["threshold"] = hist_threshold;
                callback_json["algorithm"] = "CompareHist";
                if (score >= hist_threshold) {
                    matched = true;
                }
            }
            else {
                continue;
            }
        }
        break;
        case AlgorithmType::OcrDetect:
        {
            std::shared_ptr<OcrTaskInfo> ocr_task_info_ptr =
                std::dynamic_pointer_cast<OcrTaskInfo>(task_info_ptr);
            std::vector<TextArea> match_result;

            // 处理ocr匹配结果
            auto proc_match_result = [&](const Rect& rect_offset) -> bool {
                if (!match_result.empty()) {
                    callback_json["text"] = match_result.at(0).text;
                    rect = match_result.at(0).rect;
                    rect.x += rect_offset.x;
                    rect.y += rect_offset.y;
                    matched = true;
                    return true;
                }
                else {
                    matched = false;
                    return false;
                }
            };

            bool need_match = ocr_task_info_ptr->need_match;
            const auto& identify_area = m_controller_ptr->shaped_correct(ocr_task_info_ptr->identify_area);

            // 先从缓存里找找有没有（同一张图片，之前其他任务识别的），找到了直接就不识别了
            std::vector<TextArea> temp_match;
            if (need_match) {
                temp_match = text_match(ocr_cache, ocr_task_info_ptr->text, ocr_task_info_ptr->replace_map);
            }
            else {
                temp_match = text_search(ocr_cache, ocr_task_info_ptr->text, ocr_task_info_ptr->replace_map);
            }
            // 即使在缓存里找到了，也得在识别区域里才行，不然过滤掉
            for (const TextArea& textarea : temp_match) {
                if (identify_area.include(textarea.rect)) {
                    match_result.emplace_back(textarea);
                }
            }
            if (proc_match_result(Rect())) {
                callback_json["algorithm"] = "OcrDetectCache";
                break;
            }

            auto identify_roi = [&](const Rect roi) -> bool {
                cv::Mat roi_image = cur_image(make_rect<cv::Rect>(roi));
                std::vector<TextArea> all_text_area = ocr_detect(roi_image);
                // 校正选区坐标，并加入ocr识别缓存
                for (TextArea& textarea : all_text_area) {
                    textarea.rect.x += roi.x;
                    textarea.rect.y += roi.y;
                    ocr_cache.emplace(textarea);
                }
                if (need_match) {
                    match_result = text_match(all_text_area, ocr_task_info_ptr->text, ocr_task_info_ptr->replace_map);
                }
                else {
                    match_result = text_search(all_text_area, ocr_task_info_ptr->text, ocr_task_info_ptr->replace_map);
                }
                return !match_result.empty();
            };

            // 先在历史区域里识别
            for (const auto& history_area : ocr_task_info_ptr->history_area) {
                Rect roi_rect = history_area.center_zoom(2.0, Configer::WindowWidthDefault, Configer::WindowHeightDefault);
                if (identify_roi(roi_rect)) {
                    callback_json["roi"] = make_rect<json::array>(roi_rect);
                    break;
                }
            }
            if (proc_match_result(Rect())) {
                callback_json["algorithm"] = "OcrDetectRoi";
                break;
            }

            // 如果历史区域里没识别到，那再在用完整的区域识别一次
            identify_roi(identify_area);

            // 把本次完整识别到的区域加入历史区域
            if (ocr_task_info_ptr->cache) {
                for (const auto& textarea : match_result) {
                    ocr_task_info_ptr->history_area.emplace_back(textarea.rect);
                }
            }

            if (proc_match_result(Rect())) {
                callback_json["algorithm"] = "OcrDetect";
                break;
            }
            callback_json["algorithm"] = "OcrDetectNotMatched";
        }
        break;
        //CompareHist是MatchTemplate的衍生算法，不应作为单独的配置参数出现
        //case AlgorithmType::CompareHist:
        //	break;
        default:
            // TODO：抛个报错的回调出去
            break;
        }

        callback_json["rect"] = make_rect<json::array>(rect);
        callback_json["name"] = task_name;
        if (matched_rect != NULL) {
            *matched_rect = std::move(rect);
        }
        m_callback(AsstMsg::ImageFindResult, callback_json, m_callback_arg);
        if (matched) {
            m_callback(AsstMsg::ImageMatched, callback_json, m_callback_arg);
            return task_info_ptr;
        }
    }
    return nullptr;
}

// 随机延时功能
bool asst::ProcessTask::delay_random()
{
    if (Configer::get_instance().m_options.control_delay_upper != 0) {
        static std::default_random_engine rand_engine(
            std::chrono::system_clock::now().time_since_epoch().count());
        static std::uniform_int_distribution<unsigned> rand_uni(
            Configer::get_instance().m_options.control_delay_lower,
            Configer::get_instance().m_options.control_delay_upper);

        unsigned rand_delay = rand_uni(rand_engine);

        return sleep(rand_delay);
    }
    return true;
}

void ProcessTask::exec_click_task(const Rect& matched_rect)
{
    if (!delay_random()) {
        return;
    }

    m_controller_ptr->click(matched_rect);
}

void asst::ProcessTask::exec_swipe_task(ProcessTaskAction action)
{
    if (!delay_random()) {
        return;
    }
    const static Rect right_rect(Configer::WindowWidthDefault * 0.8,
        Configer::WindowHeightDefault * 0.4,
        Configer::WindowWidthDefault * 0.1,
        Configer::WindowHeightDefault * 0.2);

    const static Rect left_rect(Configer::WindowWidthDefault * 0.1,
        Configer::WindowHeightDefault * 0.4,
        Configer::WindowWidthDefault * 0.1,
        Configer::WindowHeightDefault * 0.2);

    switch (action)
    {
    case asst::ProcessTaskAction::SwipeToTheLeft:
        m_controller_ptr->swipe(left_rect, right_rect);
        break;
    case asst::ProcessTaskAction::SwipeToTheRight:
        m_controller_ptr->swipe(right_rect, left_rect);
        break;
    default:	// 走不到这里，TODO 报个错
        break;
    }
}