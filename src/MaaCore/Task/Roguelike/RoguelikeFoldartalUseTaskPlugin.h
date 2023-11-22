#pragma once
#include "AbstractRoguelikeTaskPlugin.h"
#include "Config/Roguelike/RoguelikeFoldartalConfig.h"

namespace asst
{
    class RoguelikeFoldartalUseTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        virtual ~RoguelikeFoldartalUseTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

    private:
        // 遍历配置里的组合
        bool search_enable_pair(std::vector<std::string>& list, const asst::RoguelikeFoldartalCombination& usage);
        // 查询是否有能使用的板子对
        bool board_pair(const std::string& up_board, const std::string& down_board);
        // 使用板子对
        bool use_board(const std::string& up_board, const std::string& down_board);
        // 找到并点击指定板子
        bool search_and_click_board(const std::string& board);
        // 找到并点击指定节点
        bool search_and_click_stage();
        // 滑到最上面
        void swipe_to_top();
        // 往下滑
        void slowly_swipe(bool to_down, int swipe_dist = 200);
        // 节点类型
        mutable std::string m_stage;
    };
}
