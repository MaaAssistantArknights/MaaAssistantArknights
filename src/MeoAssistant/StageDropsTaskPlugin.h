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
    };
}
