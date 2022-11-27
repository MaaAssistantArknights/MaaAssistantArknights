#pragma once
#include "Task/AbstractTaskPlugin.h"

#include <memory>
#include <vector>

#include <meojson/json.hpp>

#include "Config/Miscellaneous/StageDropsConfig.h"
#include "Status.h"

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

        bool set_enable_penguid(bool enable);
        bool set_penguin_id(std::string id);
        bool set_server(std::string server);
        bool set_specify_quantity(std::unordered_map<std::string, int> quantity);

    private:
        virtual bool _run() override;

        bool recognize_drops();
        void drop_info_callback();
        void set_start_button_delay();
        bool check_stage_valid();
        bool check_specify_quantity() const;
        void stop_task();
        void upload_to_penguin();
        static void report_penguin_callback(AsstMsg msg, const json::value& detail, AbstractTask* task_ptr);

        static inline constexpr int64_t RecognitionTimeOffset = 20;
        static inline const std::string LastStartTimeKey = Status::ProcessTaskLastTimePrefix + "Fight@StartButton2";
        static inline const std::string RecognitionRestrictionsKey = "StageDropsTaskPluginRestrictions";

        std::string m_stage_code;
        StageDifficulty m_stage_difficulty = StageDifficulty::Normal;
        int m_stars = 0;
        std::vector<StageDropInfo> m_cur_drops;
        std::unordered_map<std::string, int> m_drop_stats;
        json::value m_cur_info_json;

        mutable bool m_is_annihilation = false;
        bool m_start_button_delay_is_set = false;
        ProcessTask* m_cast_ptr = nullptr;
        std::shared_ptr<ReportDataTask> m_report_penguin_task_ptr = nullptr;
        bool m_enable_penguid = false;
        std::string m_penguin_id;
        std::string m_server = "CN";
        std::unordered_map<std::string, int> m_specify_quantity;
    };
}
