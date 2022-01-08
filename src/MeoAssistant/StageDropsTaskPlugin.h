#pragma once
#include "AbstractTaskPlugin.h"

#include <future>
#include <vector>

#include <meojson/json.h>

namespace asst
{
    class ProcessTask;

    class StageDropsTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~StageDropsTaskPlugin() = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;
        virtual bool run(AbstractTask* ptr) override;

    private:
        void recognize_drops();
        void drop_info_callback();
        void set_startbutton_delay(ProcessTask* ptr);
        void upload_to_penguin();

        std::unordered_map<std::string, int> m_drop_stats;
        json::value m_cur_drops;
        bool m_startbutton_delay_setted = false;
        std::vector<std::future<void>> m_upload_pending;
    };
}
