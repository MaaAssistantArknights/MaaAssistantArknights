#pragma once
#include "Task/AbstractTaskPlugin.h"

#include <memory>
#include <vector>

#include <meojson/json.hpp>

#include "Config/Miscellaneous/StageDropsConfig.h"
#include "Status.h"
#include "Task/ReportDataTask.h"

namespace asst
{
class ProcessTask;
class ReportDataTask;

class StageDropsTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~StageDropsTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual void set_task_ptr(AbstractTask* ptr) override;

    bool set_enable_penguin(bool enable);
    bool set_penguin_id(std::string id);
    bool set_server(std::string server);
    bool set_specify_quantity(std::unordered_map<std::string, int> quantity);

    bool set_enable_yituliu(bool enable);

private:
    virtual bool _run() override;

    bool recognize_drops();
    void drop_info_callback();
    void set_start_button_delay();
    bool check_stage_valid();
    bool check_specify_quantity() const;
    void stop_task();
    bool upload_to_server(const std::string& subtask, ReportType report_type);
    bool upload_to_penguin(); // 返回值表示该次掉落是否通过企鹅检查
    void upload_to_yituliu();
    static void report_penguin_callback(AsstMsg msg, const json::value& detail, AbstractTask* task_ptr);
    static void report_yituliu_callback(AsstMsg msg, const json::value& detail, AbstractTask* task_ptr);

    static inline constexpr int64_t RecognitionTimeOffset = 20;
    static inline const std::string LastStartTimeKey = Status::ProcessTaskLastTimePrefix + "Fight@StartButton2";
    static inline const std::string LastPRTS1TimeKey = Status::ProcessTaskLastTimePrefix + "Fight@PRTS1";
    static inline const std::string RecognitionRestrictionsKey = "StageDropsTaskPluginRestrictions";

    std::string m_stage_code;
    StageDifficulty m_stage_difficulty = StageDifficulty::Normal;
    int m_stars = 0;
    int m_times = -2; // -2 means recognition failed, -1 means not found
    std::vector<StageDropInfo> m_cur_drops;
    std::unordered_map<std::string, int> m_drop_stats;
    json::value m_cur_info_json;

    mutable bool m_is_annihilation = false;
    bool m_start_button_delay_is_set = false;
    ProcessTask* m_cast_ptr = nullptr;
    std::shared_ptr<ReportDataTask> m_report_penguin_task_ptr = nullptr;
    std::shared_ptr<ReportDataTask> m_report_yituliu_task_ptr = nullptr;
    bool m_enable_penguin = false;
    std::string m_penguin_id;
    bool m_enable_yituliu = false;
    std::string m_server = "CN";
    std::unordered_map<std::string, int> m_specify_quantity;
};
}
