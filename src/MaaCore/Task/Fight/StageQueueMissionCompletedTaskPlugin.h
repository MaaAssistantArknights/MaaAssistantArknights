#pragma once
#include "Task/AbstractTaskPlugin.h"

#include <regex>

#include "Config/Miscellaneous/StageDropsConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ReportDataTask.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/StageDropsImageAnalyzer.h"

namespace asst
{
class StageQueueMissionCompletedTaskPlugin : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~StageQueueMissionCompletedTaskPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    void set_drop_stats(std::unordered_map<std::string, int> m_drop_stats);
    std::unordered_map<std::string, int> get_drop_stats();

    bool set_enable_penguin(bool enable);
    bool set_penguin_id(std::string id);
    bool set_server(std::string server);

    bool set_enable_yituliu(bool enable);

private:
    virtual bool _run() override;

    void mission_completed();
    void drop_info_callback(std::string stage_code, StageDifficulty stage_difficulty, StageDropsImageAnalyzer analyzer);
    void upload_to_penguin(std::string stage_code, int stars);
    static void report_penguin_callback(AsstMsg msg, const json::value& detail, AbstractTask* task_ptr);

    std::unordered_map<std::string, int> m_drop_stats;
    json::value m_cur_info_json;
    // 企鹅物流上报用
    bool m_enable_penguin = false;
    std::string m_penguin_id;
    std::string m_server = "CN";
    std::shared_ptr<ReportDataTask> m_report_penguin_task_ptr = nullptr;
    // 一图流上报用
    bool m_enable_yituliu = false;
};
}
