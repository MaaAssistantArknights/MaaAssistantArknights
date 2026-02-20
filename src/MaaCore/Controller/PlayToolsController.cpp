#include "PlayToolsController.h"

#include <boost/asio.hpp>
#include <chrono>
#include <numeric>
#include <ranges>

#include "Config/GeneralConfig.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "SwipeHelper.hpp"

using boost::asio::ip::tcp;
namespace socket_ops = boost::asio::detail::socket_ops;

asst::PlayToolsController::PlayToolsController(
    const AsstCallback& callback,
    Assistant* inst,
    PlatformType type [[maybe_unused]]) :
    InstHelper(inst),
    m_callback(callback),
    m_socket(m_context)
{
    LogTraceFunction;
}

asst::PlayToolsController::~PlayToolsController()
{
    close();
}

bool asst::PlayToolsController::connect(
    const std::string& adb_path [[maybe_unused]],
    const std::string& address,
    const std::string& config [[maybe_unused]])
{
    if (m_address != address) {
        close();
        m_address = address;
    }

    return open();
}

bool asst::PlayToolsController::inited() const noexcept
{
    return m_socket.is_open() && m_screen_size.first > 0;
}

const std::string& asst::PlayToolsController::get_uuid() const
{
    const static std::string uuid("com.hypergryph.arknights");
    return uuid;
}

size_t asst::PlayToolsController::get_pipe_data_size() const noexcept
{
    return size_t();
}

size_t asst::PlayToolsController::get_version() const noexcept
{
    return size_t();
}

bool asst::PlayToolsController::screencap(cv::Mat& image_payload, bool allow_reconnect [[maybe_unused]])
{
    using namespace std::chrono;
    LogTraceFunction;

    auto start_time = high_resolution_clock::now();
    bool screencap_ret = false;

    open();
    uint32_t image_size = 0;

    auto request_start = high_resolution_clock::now();
    try {
        constexpr char request[6] = { 0, 4, 'S', 'C', 'R', 'N' };
        boost::asio::write(m_socket, boost::asio::buffer(request));
        boost::asio::read(m_socket, boost::asio::buffer(&image_size, sizeof(image_size)));
        image_size = socket_ops::network_to_host_long(image_size);
    }
    catch (const std::exception& e) {
        Log.error("Cannot get screencap:", e.what());
        auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
        recordScreencapCost(duration.count(), false);
        return false;
    }
    auto request_time = duration_cast<milliseconds>(high_resolution_clock::now() - request_start).count();

    if (image_size == 0) {
        Log.error("Cannot get screencap: invalid image size");
        auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
        recordScreencapCost(duration.count(), false);
        return false;
    }

    try {
        std::vector<uint8_t> buffer(image_size);
        
        auto recv_start = high_resolution_clock::now();
        boost::asio::read(m_socket, boost::asio::buffer(buffer, image_size));
        auto recv_time = duration_cast<milliseconds>(high_resolution_clock::now() - recv_start).count();
        
        auto convert_start = high_resolution_clock::now();
        image_payload = cv::Mat(m_screen_size.second, m_screen_size.first, CV_8UC4, buffer.data());
        cv::cvtColor(image_payload, image_payload, cv::COLOR_RGBA2BGR);
        auto convert_time = duration_cast<milliseconds>(high_resolution_clock::now() - convert_start).count();
        
        screencap_ret = true;
        
        // 记录详细的性能分析（前 5 次）
        if (m_screencap_times < 5) {
            Log.info("PlayTools screencap breakdown: request=", request_time, "ms, recv=", recv_time, 
                    "ms, convert=", convert_time, "ms, size=", image_size / 1024, "KB");
        }
    }
    catch (const std::exception& e) {
        Log.error("Cannot get screencap:", e.what());
        auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
        recordScreencapCost(duration.count(), false);
        return false;
    }

    auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
    recordScreencapCost(duration.count(), screencap_ret);
    return true;
}

bool asst::PlayToolsController::start_game(const std::string& client_type [[maybe_unused]])
{
    Log.info("InputText is not supported on iOS");
    return true;
}

bool asst::PlayToolsController::stop_game(const std::string& client_type [[maybe_unused]])
{
    try {
        constexpr char request[6] = { 0, 4, 'T', 'E', 'R', 'M' };
        boost::asio::write(m_socket, boost::asio::buffer(request));
    }
    catch (const std::exception& e) {
        Log.error("Cannot terminate game:", e.what());
        return false;
    }

    return true;
}

bool asst::PlayToolsController::click(const Point& p)
{
    Log.trace("PlayTools click:", p);
    return toucher_down(p) && toucher_up(p);
}

bool asst::PlayToolsController::input([[maybe_unused]] const std::string& text)
{
    Log.info("InputText is not supported on iOS");
    return true;
}

bool asst::PlayToolsController::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause [[maybe_unused]])
{
    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    const auto width = m_screen_size.first;
    const auto height = m_screen_size.second;

    // 起点不能在屏幕外，但是终点可以
    if (x1 < 0 || x1 >= width || y1 < 0 || y1 >= height) {
        Log.warn("swipe point1 is out of range", x1, y1);
        x1 = std::clamp(x1, 0, width - 1);
        y1 = std::clamp(y1, 0, height - 1);
    }

    Log.trace("PlayTools swipe", p1, p2, duration, extra_swipe, slope_in, slope_out);

    toucher_down(p1);

    auto bounds_check = [width, height](int x, int y) {
        return x >= 0 && x <= width && y >= 0 && y <= height;
    };

    auto move_func = [this](int x, int y) {
        toucher_move({ x, y });
        return true;
    };

    auto progressive_move = [&](int _x1, int _y1, int _x2, int _y2, int _duration) {
        interpolate_swipe(
            _x1,
            _y1,
            _x2,
            _y2,
            _duration,
            DefaultSwipeDelay,
            slope_in,
            slope_out,
            move_func,
            bounds_check);
    };

    const auto& opt = Config.get_options();

    progressive_move(x1, y1, x2, y2, duration ? duration : opt.minitouch_swipe_default_duration);

    if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
        toucher_wait(opt.minitouch_swipe_extra_end_delay);
        progressive_move(x2, y2, x2, y2 - opt.minitouch_extra_swipe_dist, opt.minitouch_extra_swipe_duration);
    }

    return toucher_up(p2);
}

bool asst::PlayToolsController::press_esc()
{
    Log.info("ESC is not supported on iOS");
    return false;
}

std::pair<int, int> asst::PlayToolsController::get_screen_res() const noexcept
{
    return m_screen_size;
}

void asst::PlayToolsController::back_to_home() noexcept
{
    Log.info("HOME is not supported on iOS");
    return;
}

bool asst::PlayToolsController::toucher_down(const Point& p, const int delay)
{
    return toucher_commit(TouchPhase::Began, p, delay);
}

bool asst::PlayToolsController::toucher_move(const Point& p, const int delay)
{
    return toucher_commit(TouchPhase::Moved, p, delay);
}

bool asst::PlayToolsController::toucher_up(const Point& p, const int delay)
{
    return toucher_commit(TouchPhase::Ended, p, delay);
}

void asst::PlayToolsController::toucher_wait(const int delay)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
}

void asst::PlayToolsController::close()
{
    m_screen_size = { 0, 0 };
    // 重置截图性能统计数据
    m_screencap_cost.clear();
    m_screencap_times = 0;
    m_screencap_cost_reported = false;
    
    if (m_socket.is_open()) {
        try {
            m_socket.shutdown(tcp::socket::shutdown_both);
        }
        catch (const std::exception& e) {
            Log.warn("Error during socket shutdown in close():", e.what());
        }

        try {
            m_socket.close();
        }
        catch (const std::exception& e) {
            Log.warn("Error during socket close() cleanup:", e.what());
        }
    }
}

bool asst::PlayToolsController::open()
{
    if (m_socket.is_open()) {
        return true;
    }

    std::string host, port;
    std::stringstream ss(m_address);
    std::getline(ss, host, ':');
    std::getline(ss, port);

    tcp::resolver resolver(m_context);

    std::array<uint8_t, 4> buffer;
    constexpr char handshake[4] = { 'M', 'A', 'A', 0 };
    constexpr char signature[4] = { 'O', 'K', 'A', 'Y' };

    try {
        boost::asio::connect(m_socket, resolver.resolve(host, port));
        boost::asio::write(m_socket, boost::asio::buffer(handshake));
        boost::asio::read(m_socket, boost::asio::buffer(buffer, 4));
    }
    catch (const std::exception& e) {
        Log.error("Cannot connect to", m_address, e.what());
        return false;
    }

    if (memcmp(&buffer, signature, 4)) {
        Log.error("Got invalid response:", buffer);
        return false;
    }

    return check_version() && fetch_screen_res();
}

bool asst::PlayToolsController::check_version()
{
    uint32_t version = 0;
    constexpr char request[6] = { 0, 4, 'V', 'E', 'R', 'N' };

    try {
        boost::asio::write(m_socket, boost::asio::buffer(request));
        boost::asio::read(m_socket, boost::asio::buffer(&version, sizeof(version)));
    }
    catch (const std::exception& e) {
        Log.error("Cannot get MaaTools version:", e.what());
        return false;
    }

    version = socket_ops::network_to_host_long(version);
    if (version < MinimalVersion) {
        Log.error("Unsupported MaaTools version:", version);

        json::value details;
        details["what"] = "UnsupportedPlayTools";
        details["why"] = "NeedUpgrade";
        details["uuid"] = get_uuid();
        m_callback(AsstMsg::ConnectionInfo, details, m_inst);

        return false;
    }

    return true;
}

bool asst::PlayToolsController::fetch_screen_res()
{
    uint16_t width = 0, height = 0;
    constexpr char request[6] = { 0, 4, 'S', 'I', 'Z', 'E' };

    try {
        boost::asio::write(m_socket, boost::asio::buffer(request));
        boost::asio::read(m_socket, boost::asio::buffer(&width, sizeof(width)));
        boost::asio::read(m_socket, boost::asio::buffer(&height, sizeof(height)));
    }
    catch (const std::exception& e) {
        Log.error("Cannot get screen resolution:", e.what());
        return false;
    }

    width = socket_ops::network_to_host_short(width);
    height = socket_ops::network_to_host_short(height);

    m_screen_size = { width, height };
    return true;
}

bool asst::PlayToolsController::toucher_commit(const TouchPhase phase, const Point& p, const int delay)
{
    open();
    uint16_t x = socket_ops::host_to_network_short(static_cast<uint16_t>(p.x));
    uint16_t y = socket_ops::host_to_network_short(static_cast<uint16_t>(p.y));
    uint8_t payload[5] = { static_cast<uint8_t>(phase), 0, 0, 0, 0 };
    std::memcpy(payload + 1, &x, sizeof(x));
    std::memcpy(payload + 3, &y, sizeof(y));

    try {
        constexpr char request[6] = { 0, 9, 'T', 'U', 'C', 'H' };
        boost::asio::write(m_socket, boost::asio::buffer(request));
        boost::asio::write(m_socket, boost::asio::buffer(payload, 5));
    }
    catch (const std::exception& e) {
        Log.error("Cannot touch screen:", e.what());
        return false;
    }

    toucher_wait(delay);
    return true;
}

void asst::PlayToolsController::recordScreencapCost(long long cost, bool success)
{
    // 记录截图耗时，仅在首次（开始任务时）统计并显示一次
    m_screencap_cost.emplace_back(success ? cost : -1);
    ++m_screencap_times;

    if (m_screencap_cost.size() > 30) {
        m_screencap_cost.pop_front();
    }

    // 只在首次达到10次截图时统计并显示一次
    if (m_screencap_times > 9 && !m_screencap_cost_reported) {
        m_screencap_cost_reported = true; // 标记已报告，后续不再显示
        
        auto filtered_cost = m_screencap_cost | std::views::filter([](auto num) { return num > 0; });
        if (filtered_cost.empty()) {
            return;
        }

        // 过滤后的有效截图用时次数
        auto filtered_count = m_screencap_cost.size() - std::ranges::count(m_screencap_cost, -1);
        auto [screencap_cost_min, screencap_cost_max] = std::ranges::minmax(filtered_cost);

        json::value info = json::object {
            { "uuid", get_uuid() },
            { "what", "ScreencapCost" },
            { "details",
              json::object {
                  { "min", screencap_cost_min },
                  { "max", screencap_cost_max },
                  { "avg",
                    filtered_count > 0
                        ? std::accumulate(filtered_cost.begin(), filtered_cost.end(), 0ll) / filtered_count
                        : -1 },
              } },
        };

        if (m_screencap_cost.size() > filtered_count) {
            info["details"]["fault_times"] = m_screencap_cost.size() - filtered_count;
        }

        m_callback(AsstMsg::ConnectionInfo, info, inst());
    }
}
