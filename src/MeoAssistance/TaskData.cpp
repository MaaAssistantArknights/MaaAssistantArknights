/*
    MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
    Copyright (C) 2021 MistEO and Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "TaskData.h"

#include <algorithm>

#include <json.h>

#include "AsstDef.h"
#include "GeneralConfiger.h"
#include "TemplResource.h"

bool asst::TaskData::set_param(const std::string& type, const std::string& param, const std::string& value) {
    // 暂时只用到了这些，总的参数太多了，后面要用啥再加上
    if (type == "task.action") {
        if (auto iter = m_all_tasks_info.find(param);
            iter == m_all_tasks_info.cend()) {
            return false;
        }
        else {
            auto& task_info_ptr = iter->second;
            std::string action = value;
            std::transform(action.begin(), action.end(), action.begin(), ::tolower);
            if (action == "clickself") {
                task_info_ptr->action = ProcessTaskAction::ClickSelf;
            }
            else if (action == "clickrand") {
                task_info_ptr->action = ProcessTaskAction::ClickRand;
            }
            else if (action == "donothing" || action.empty()) {
                task_info_ptr->action = ProcessTaskAction::DoNothing;
            }
            else if (action == "stop") {
                task_info_ptr->action = ProcessTaskAction::Stop;
            }
            else if (action == "clickrect") {
                task_info_ptr->action = ProcessTaskAction::ClickRect;
            }
            else {
                m_last_error = "Task " + param + " 's action error: " + action;
                return false;
            }
        }
    }
    else if (type == "task.maxTimes") {
        if (auto iter = m_all_tasks_info.find(param);
            iter == m_all_tasks_info.cend()) {
            return false;
        }
        else {
            iter->second->max_times = std::stoi(value);
        }
    }
    return true;
}

const std::shared_ptr<asst::TaskInfo> asst::TaskData::task_ptr(const std::string& name) const noexcept {
    if (auto iter = m_all_tasks_info.find(name);
        iter != m_all_tasks_info.cend()) {
        return iter->second;
    }
    else {
        return nullptr;
    }
}

const std::unordered_set<std::string>& asst::TaskData::get_templ_required() const noexcept {
    return m_templ_required;
}

std::shared_ptr<asst::TaskInfo> asst::TaskData::task_ptr(std::string name) {
    return m_all_tasks_info[std::move(name)];
}

void asst::TaskData::clear_exec_times() {
    for (auto&& [key, task] : m_all_tasks_info) {
        task->exec_times = 0;
    }
}

bool asst::TaskData::parse(const json::value& json) {
    for (const auto& [name, task_json] : json.as_object()) {
        std::string algorithm_str = task_json.get("algorithm", "matchtemplate");
        std::transform(algorithm_str.begin(), algorithm_str.end(), algorithm_str.begin(), ::tolower);
        AlgorithmType algorithm = AlgorithmType::Invaild;
        if (algorithm_str == "matchtemplate") {
            algorithm = AlgorithmType::MatchTemplate;
        }
        else if (algorithm_str == "justreturn") {
            algorithm = AlgorithmType::JustReturn;
        }
        else if (algorithm_str == "ocrdetect") {
            algorithm = AlgorithmType::OcrDetect;
        }
        // CompareHist是MatchTemplate的衍生算法，不应作为单独的配置参数出现
        // else if (algorithm_str == "comparehist") {}
        else {
            m_last_error = "algorithm error " + algorithm_str;
            return false;
        }

        std::shared_ptr<TaskInfo> task_info_ptr = nullptr;
        switch (algorithm) {
        case AlgorithmType::JustReturn:
            task_info_ptr = std::make_shared<TaskInfo>();
            break;
        case AlgorithmType::MatchTemplate: {
            auto match_task_info_ptr = std::make_shared<MatchTaskInfo>();
            match_task_info_ptr->templ_name = task_json.get("template", name + ".png");
            m_templ_required.emplace(match_task_info_ptr->templ_name);

            match_task_info_ptr->templ_threshold = task_json.get(
                "templThreshold",
                TemplResource::TemplThresholdDefault);
            match_task_info_ptr->hist_threshold = task_json.get(
                "histThreshold",
                TemplResource::HistThresholdDefault);
            match_task_info_ptr->cache = task_json.get("cache", true);
            if (task_json.exist("maskRange")) {
                match_task_info_ptr->mask_range = std::make_pair(
                    task_json.at("maskRange")[0].as_integer(),
                    task_json.at("maskRange")[1].as_integer());
            }

            task_info_ptr = match_task_info_ptr;
        } break;
        case AlgorithmType::OcrDetect: {
            auto ocr_task_info_ptr = std::make_shared<OcrTaskInfo>();
            for (const json::value& text : task_json.at("text").as_array()) {
                ocr_task_info_ptr->text.emplace_back(text.as_string());
            }
            ocr_task_info_ptr->need_full_match = task_json.get("need_match", false);
            if (task_json.exist("ocrReplace")) {
                for (const json::value& rep : task_json.at("ocrReplace").as_array()) {
                    ocr_task_info_ptr->replace_map.emplace(rep.as_array()[0].as_string(), rep.as_array()[1].as_string());
                }
            }
            ocr_task_info_ptr->cache = task_json.get("cache", true);
            task_info_ptr = ocr_task_info_ptr;
        } break;
        }
        task_info_ptr->algorithm = algorithm;
        task_info_ptr->name = name;
        std::string action = task_json.get("action", std::string());
        std::transform(action.begin(), action.end(), action.begin(), ::tolower);
        if (action == "clickself") {
            task_info_ptr->action = ProcessTaskAction::ClickSelf;
        }
        else if (action == "clickrand") {
            task_info_ptr->action = ProcessTaskAction::ClickRand;
        }
        else if (action == "donothing" || action.empty()) {
            task_info_ptr->action = ProcessTaskAction::DoNothing;
        }
        else if (action == "stop") {
            task_info_ptr->action = ProcessTaskAction::Stop;
        }
        else if (action == "clickrect") {
            task_info_ptr->action = ProcessTaskAction::ClickRect;
            const json::value& rect_json = task_json.at("specificRect");
            task_info_ptr->specific_rect = Rect(
                rect_json[0].as_integer(),
                rect_json[1].as_integer(),
                rect_json[2].as_integer(),
                rect_json[3].as_integer());
        }
        else if (action == "stagedrops") {
            task_info_ptr->action = ProcessTaskAction::StageDrops;
        }
        else if (action == "swipetotheleft") {
            task_info_ptr->action = ProcessTaskAction::SwipeToTheLeft;
        }
        else if (action == "swipetotheright") {
            task_info_ptr->action = ProcessTaskAction::SwipeToTheRight;
        }
        else {
            m_last_error = "Task: " + name + " error: " + action;
            return false;
        }

        task_info_ptr->max_times = task_json.get("maxTimes", INT_MAX);
        if (task_json.exist("exceededNext")) {
            const json::array& excceed_next_arr = task_json.at("exceededNext").as_array();
            for (const json::value& excceed_next : excceed_next_arr) {
                task_info_ptr->exceeded_next.emplace_back(excceed_next.as_string());
            }
        }
        else {
            task_info_ptr->exceeded_next.emplace_back("Stop");
        }
        task_info_ptr->pre_delay = task_json.get("preDelay", 0);
        task_info_ptr->rear_delay = task_json.get("rearDelay", 0);
        if (task_json.exist("reduceOtherTimes")) {
            const json::array& reduce_arr = task_json.at("reduceOtherTimes").as_array();
            for (const json::value& reduce : reduce_arr) {
                task_info_ptr->reduce_other_times.emplace_back(reduce.as_string());
            }
        }
        if (task_json.exist("roi")) {
            const json::array& area_arr = task_json.at("roi").as_array();
            task_info_ptr->roi = Rect(
                area_arr[0].as_integer(),
                area_arr[1].as_integer(),
                area_arr[2].as_integer(),
                area_arr[3].as_integer());
        }
        else {
            task_info_ptr->roi = Rect(
                0, 0,
                GeneralConfiger::WindowWidthDefault,
                GeneralConfiger::WindowHeightDefault);
        }

        if (task_json.exist("next")) {
            for (const json::value& next : task_json.at("next").as_array()) {
                task_info_ptr->next.emplace_back(next.as_string());
            }
        }
        if (task_json.exist("rectMove")) {
            const json::array& move_arr = task_json.at("rectMove").as_array();
            task_info_ptr->rect_move = Rect(
                move_arr[0].as_integer(),
                move_arr[1].as_integer(),
                move_arr[2].as_integer(),
                move_arr[3].as_integer());
        }
        else {
            task_info_ptr->rect_move = Rect();
        }

        m_all_tasks_info.emplace(name, task_info_ptr);
    }
    return true;
}
