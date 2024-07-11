#pragma once
#include "AbstractRoguelikeTaskPlugin.h"
#include "Config/Roguelike/RoguelikeFoldartalConfig.h"
#include "RoguelikeCollapsalParadigmTaskPlugin.h"

namespace asst
{
    class RoguelikeFoldartalUseTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        // using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        RoguelikeFoldartalUseTaskPlugin(const AsstCallback& callback,
                                             Assistant* inst,
                                             std::string_view task_chain,
                                             std::shared_ptr<RoguelikeConfig> config)
                                             : AbstractRoguelikeTaskPlugin(callback, inst, task_chain, config)
        {
            if (RoguelikeCollapsalParadigmTaskPlugin::enabled(config)) {
                m_clp_pd_plugin = std::make_shared<RoguelikeCollapsalParadigmTaskPlugin>(callback, inst, task_chain, config);
            }
        }
        virtual ~RoguelikeFoldartalUseTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

    private:
        // 遍历配置里的组合
        void use_enable_pair(std::vector<std::string>& list, const asst::RoguelikeFoldartalCombination& usage);
        // 使用板子对结果
        enum class UseBoardResult
        {
            ClickFoldartalError,
            UseBoardResultSuccess,
            UpBoardNotFound,
            DownBoardNotFound,
            StageNotFound,
            CanNotUseConfirm,
            UnknownError,
        };
        // 使用板子对
        UseBoardResult use_board(const std::string& up_board, const std::string& down_board) const;
        // 找到并点击指定板子
        bool search_and_click_board(const std::string& board) const;
        // 找到并点击指定节点
        bool search_and_click_stage() const;
        // 滑到最上面
        void swipe_to_top() const;
        // 缓慢滑动
        void slowly_swipe(bool direction, int swipe_dist = 200) const;
        // 节点类型
        mutable std::string m_stage;
        
        std::shared_ptr<RoguelikeCollapsalParadigmTaskPlugin> m_clp_pd_plugin;
    };
}
