#pragma once

#include "ControllerAPI.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "Platform/PlatformFactory.h"

#include "Common/AsstMsg.h"
#include "InstHelper.h"

namespace asst
{
class WDAController : public ControllerAPI, protected InstHelper
{
public:
    WDAController(const AsstCallback& callback, Assistant* inst, PlatformType type);
    WDAController(const WDAController&) = delete;
    WDAController(WDAController&&) = delete;
    virtual ~WDAController();

    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;
    virtual bool inited() const noexcept override;

    virtual const std::string& get_uuid() const override;

    virtual size_t get_pipe_data_size() const noexcept override;

    virtual size_t get_version() const noexcept override;

    virtual bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;

    virtual bool start_game(const std::string& client_type) override;
    virtual bool stop_game(const std::string& client_type) override;

    virtual bool click(const Point& p) override;

    virtual bool input(const std::string& text) override;

    virtual bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        bool extra_swipe = false,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    virtual bool inject_input_event([[maybe_unused]] const InputEvent& event) override { return false; }

    virtual bool press_esc() override;

    virtual ControlFeat::Feat support_features() const noexcept override { return ControlFeat::NONE; }

    virtual std::pair<int, int> get_screen_res() const noexcept override;

    WDAController& operator=(const WDAController&) = delete;
    WDAController& operator=(WDAController&&) = delete;

    virtual void back_to_home() noexcept override;

protected:
    AsstCallback m_callback;

private:
    boost::asio::io_context m_context;
    std::string m_host;
    uint16_t m_port = 8100;
    std::string m_session_id;
    std::pair<int, int> m_screen_size = { 0, 0 };
    bool m_connected = false;

    struct HttpResponse
    {
        int status_code = 0;
        std::string body;
        bool success() const { return status_code >= 200 && status_code < 300; }
    };

    HttpResponse http_get(const std::string& path);
    HttpResponse http_post(const std::string& path, const std::string& json_body);
    HttpResponse http_delete(const std::string& path);

    bool check_wda_status();
    bool create_session();
    bool fetch_screen_size();
    bool destroy_session();

    std::string build_w3c_tap_action(int x, int y);
    std::string build_w3c_swipe_action(int x1, int y1, int x2, int y2, int duration_ms);

    bool decode_base64_png(const std::string& base64_data, cv::Mat& output);
    static std::string base64_decode(const std::string& encoded);
};
} // namespace asst
