#pragma once

#ifdef WITH_THRIFT

#include "ControllerAPI.h"

#include "Platform/PlatformFactory.h"

#include "Common/AsstMsg.h"
#include "InstHelper.h"

#include "ThriftController.h"
#include "ThriftController_types.h"

namespace asst
{
class MaaThriftController : public ControllerAPI, protected InstHelper
{
public:
    MaaThriftController(const AsstCallback& callback, Assistant* inst, [[maybe_unused]] PlatformType type) :
        InstHelper(inst),
        m_callback(callback)
    {
        LogTraceFunction;
    }

    MaaThriftController(const MaaThriftController&) = delete;
    MaaThriftController(MaaThriftController&&) = delete;
    virtual ~MaaThriftController();

    virtual bool connect(
        [[maybe_unused]] const std::string& adb_path,
        [[maybe_unused]] const std::string& address,
        const std::string& config) override;
    virtual bool inited() const noexcept override;

    virtual void set_swipe_with_pause(bool enable) noexcept override;

    virtual const std::string& get_uuid() const override;

    virtual bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;

    virtual bool start_game(const std::string& client_type) override;
    virtual bool stop_game() override;

    virtual bool click(const Point& p) override;

    virtual bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        bool extra_swipe = false,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    virtual bool inject_input_event(const InputEvent& event) override;

    virtual bool press_esc() override;
    virtual ControlFeat::Feat support_features() const noexcept override;

    virtual std::pair<int, int> get_screen_res() const noexcept override;

    MaaThriftController& operator=(const MaaThriftController&) = delete;
    MaaThriftController& operator=(MaaThriftController&&) = delete;

    virtual void back_to_home() noexcept override;

protected:
    virtual void clear_info() noexcept;
    void callback(AsstMsg msg, const json::value& details);

    bool support_precise_swipe() const noexcept;
    bool support_swipe_with_pause() const noexcept;
    bool use_swipe_with_pause() const noexcept;

    AsstCallback m_callback;

    std::string m_config;
    std::string m_uuid;
    ControlFeat::Feat m_support_features = ControlFeat::NONE;
    bool m_inited = false;
    bool m_swipe_with_pause_enabled = false;
    std::pair<int, int> m_screen_size = { 0, 0 };
    std::vector<ThriftController::InputEvent> m_input_events;

    static constexpr int DefaultClickDelay = 50;
    static constexpr int DefaultSwipeDelay = 5;

    bool toucher_down(const Point& p, const int delay = DefaultClickDelay, int contact = 0);
    bool toucher_move(const Point& p, const int delay = DefaultSwipeDelay, int contact = 0);
    bool toucher_up(const Point& p, const int delay = DefaultClickDelay, int contact = 0);

    void toucher_wait(const int delay);

    std::shared_ptr<ThriftController::ThriftControllerClient> client_ = nullptr;
    std::shared_ptr<apache::thrift::transport::TTransport> transport_ = nullptr;

    enum ThriftControllerTypeEnum
    {
        // param format should be "host:port"
        MaaThriftControllerType_Socket = 1,

        // param should be unix domain socket path
        MaaThriftControllerType_UnixDomainSocket = 2,
    };

private:
    static constexpr int MinimalVersion = 2;
    void close();
    bool open();
};
} // namespace asst

#endif // WITH_THRIFT
