#pragma once
#include "AbstractTaskPlugin.h"

#include <future>
#include <vector>

#include <meojson/json.hpp>

namespace asst
{
    class ProcessTask;

    class StageDropsTaskPlugin final : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~StageDropsTaskPlugin() = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;
        virtual void set_task_ptr(AbstractTask* ptr) override;

        bool set_enable_penguid(bool enable);
        bool set_penguin_id(std::string id);
        bool set_server(std::string server);
    private:
        virtual bool _run() override;

        bool recognize_drops();
        void drop_info_callback();
        void set_startbutton_delay();
        bool check_stage_valid();
        void upload_to_penguin();

        static constexpr int64_t RecognizationTimeOffset = 20;
        std::unordered_map<std::string, int> m_drop_stats;
        json::value m_cur_drops;
        bool m_startbutton_delay_setted = false;
        std::vector<std::future<void>> m_upload_pending;
        ProcessTask* m_cast_ptr = nullptr;
        bool m_enable_penguid = false;
        std::string m_penguin_id;
        std::string m_server = "CN";
    };
}
