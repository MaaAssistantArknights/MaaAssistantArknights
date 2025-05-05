#pragma once
#include "Config/TaskData.h"
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
class FightSeriesAdjustPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~FightSeriesAdjustPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    void set_close_stone_page_next(bool enable) const
    {
        Task.get("Fight@CloseStonePage")->next =
            enable ? modified_close_stone_page_next : original_close_stone_page_next;
    };

protected:
    virtual bool _run() override;

private:
    // 临时处理
    std::vector<std::string> original_close_stone_page_next = { "Fight@StageSNReturnFlag", "Fight@Stop" };
    std::vector<std::string> modified_close_stone_page_next = { "Fight@StartButton1",
                                                                "Fight@StageSNReturnFlag",
                                                                "Fight@Stop" };
    int get_exceeded_num() const;
};
}
