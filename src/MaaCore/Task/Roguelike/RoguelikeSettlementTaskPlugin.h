#pragma once
#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{
// 肉鸽结算图保存及统计
class RoguelikeSettlementTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeSettlementTaskPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;

private:
    virtual bool _run() override;
    bool get_settlement_info(json::value& info, const cv::Mat& image);
    bool wait_for_whole_page();
    void save_img(const cv::Mat& image, const std::filesystem::path& path, std::string name);

    mutable bool m_game_pass = false;
};
} // namespace asst
