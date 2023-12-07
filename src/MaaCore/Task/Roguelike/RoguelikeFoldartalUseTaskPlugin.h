#pragma once
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
        void use_enable_pair(std::vector<std::string>& list, const asst::RoguelikeFoldartalCombination& usage);
        // 使用板子对结果
        enum class use_board_result
        {
            click_foldartal_error,
            use_board_result_success,
            up_board_not_found,
            down_board_not_found,
            stage_not_found,
            can_not_use_confirm,
            unknown_error,
        };
        // 使用板子对
        use_board_result use_board(const std::string& up_board, const std::string& down_board) const;
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
    };
}
