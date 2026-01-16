#pragma once

#include "ControllerAPI.h"

#include <deque>
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

    virtual ControlFeat::Feat support_features() const noexcept override
    {
        // WDA supports precise swipe control via W3C Actions
        // WDA also supports swipe with pause: dragging operators causes game to pause,
        // and we can cancel the pause by clicking the pause button (via BattlePauseCancel task)
        return ControlFeat::PRECISE_SWIPE | ControlFeat::SWIPE_WITH_PAUSE;
    }

    virtual std::pair<int, int> get_screen_res() const noexcept override;

    WDAController& operator=(const WDAController&) = delete;
    WDAController& operator=(WDAController&&) = delete;

    virtual void back_to_home() noexcept override;

protected:
    AsstCallback m_callback;

private:
    boost::asio::io_context m_context;
    std::unique_ptr<boost::asio::ip::tcp::socket> m_socket;  // 持久化HTTP连接
    std::string m_host;
    uint16_t m_port = 8100;
    std::string m_session_id;
    std::string m_uuid;  // 设备UDID

    // 三层坐标系统
    std::pair<int, int> m_wda_logical_size = { 0, 0 };      // WDA逻辑分辨率 (e.g. 812x375 points)
    std::pair<int, int> m_physical_screen_size = { 0, 0 };  // 物理分辨率 (e.g. 2436x1124 pixels)
    std::pair<int, int> m_screen_size = { 0, 0 };           // MAA分辨率 (e.g. 1998x1124 pixels, 16:9)

    // 裁剪偏移
    int m_crop_offset_x = 0;
    int m_crop_offset_y = 0;

    bool m_connected = false;

    // 延迟统计
    std::deque<long long> m_screencap_cost; // 截图用时
    int m_screencap_times = 0;              // 截图次数

    struct HttpResponse
    {
        int status_code = 0;
        std::string body;
        bool success() const { return status_code >= 200 && status_code < 300; }
    };

    HttpResponse http_get(const std::string& path);
    HttpResponse http_post(const std::string& path, const std::string& json_body);
    HttpResponse http_delete(const std::string& path);

    bool ensure_connection();  // 确保socket连接可用
    void close_connection();   // 关闭socket连接

    bool check_wda_status();
    bool create_session();
    bool configure_fast_mode();
    bool fetch_screen_size();
    bool destroy_session();
    bool reconnect();
    void report_screencap_cost();

    std::string build_w3c_tap_action(int x, int y);
    bool perform_drag(int from_x, int from_y, int to_x, int to_y, double duration_sec);

    std::pair<int, int> transform_to_wda_coords(int maa_x, int maa_y) const;

    bool decode_base64_png(const std::string& base64_data, cv::Mat& output);
    static std::string base64_decode(const std::string& encoded);
};
} // namespace asst
