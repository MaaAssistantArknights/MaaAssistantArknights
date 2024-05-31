#include "MinitouchController.h"

#include <future>

#include "Common/AsstTypes.h"
#include "Config/GeneralConfig.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

asst::MinitouchController::~MinitouchController()
{
    LogTraceFunction;

    release_minitouch();
}

void asst::MinitouchController::release_minitouch([[maybe_unused]] bool force)
{
    m_minitoucher.reset();
    m_minitouch_handler.reset();
}

bool asst::MinitouchController::call_and_hup_minitouch()
{
    LogTraceFunction;
    release_minitouch(true);

    std::string cmd = m_use_maa_touch ? m_adb.call_maatouch : m_adb.call_minitouch;
    Log.info(cmd);

    std::string pipe_str;

    m_minitouch_handler = m_platform_io->interactive_shell(cmd);
    if (!m_minitouch_handler) {
        Log.error("unable to start minitouch");
        return false;
    }

    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < 3s;
    };

    const auto start_time = std::chrono::steady_clock::now();
    while (true) {
        if (need_exit()) {
            release_minitouch(true);
            return false;
        }

        pipe_str += m_minitouch_handler->read(3);

        if (!check_timeout(start_time)) {
            Log.info("unable to find $ from pipe_str:", Logger::separator::newline, pipe_str);
            release_minitouch(true);
            return false;
        }

        if (pipe_str.find('$') != std::string::npos) {
            break;
        }
    }

    Log.info("pipe str", Logger::separator::newline, pipe_str);

    convert_lf(pipe_str);
    size_t s_pos = pipe_str.find('^');
    size_t e_pos = pipe_str.find('\n', s_pos);
    if (s_pos == std::string::npos || e_pos == std::string::npos) {
        Log.error("Failed to find ^ in minitouch pipe");
        release_minitouch(true);
        return false;
    }
    std::string key_info = pipe_str.substr(s_pos + 1, e_pos - s_pos - 1);
    Log.info("minitouch key props", key_info);
    int size_1 = 0, size_2 = 0;
    std::stringstream ss;
    ss << key_info;
    ss >> m_minitouch_props.max_contacts;
    ss >> size_1;
    ss >> size_2;
    ss >> m_minitouch_props.max_pressure;

    // 有些模拟器在竖屏分辨率时，这里的输出是反过来的
    // 考虑到应该没人竖屏玩明日方舟，所以取较大值为 x，较小值为 y
    m_minitouch_props.max_x = std::max(size_1, size_2);
    m_minitouch_props.max_y = std::min(size_1, size_2);

    m_minitouch_props.x_scaling = static_cast<double>(m_minitouch_props.max_x) / m_width;
    m_minitouch_props.y_scaling = static_cast<double>(m_minitouch_props.max_y) / m_height;

    m_minitoucher = std::make_unique<Minitoucher>(
        std::bind(&MinitouchController::input_to_minitouch, this, std::placeholders::_1), m_minitouch_props);

    return true;
}

std::optional<std::string> asst::MinitouchController::reconnect(const std::string& cmd, int64_t timeout,
                                                                bool recv_by_socket)
{
    LogTraceFunction;

    auto ret = AdbController::reconnect(cmd, timeout, recv_by_socket);
    if (!ret) {
        return std::nullopt;
    }

    call_and_hup_minitouch();
    return ret;
}

bool asst::MinitouchController::input_to_minitouch(const std::string& cmd)
{
    if (!(m_minitouch_handler && m_minitouch_handler->write(cmd))) {
        Log.error("Failed to write to minitouch, try restart minitouch and re-write");
        return call_and_hup_minitouch() && m_minitouch_handler->write(cmd);
    }
    return true;
}

void asst::MinitouchController::set_swipe_with_pause(bool enable) noexcept
{
    m_swipe_with_pause_enabled = enable;
}

bool asst::MinitouchController::use_swipe_with_pause() const noexcept
{
    return m_swipe_with_pause_enabled && (!m_adb.press_esc.empty() || m_use_maa_touch);
}

bool asst::MinitouchController::click(const Point& p)
{
    if (!m_minitoucher) {
        Log.error("minitoucher is not initialized");
        return false;
    }

    if (p.x < 0 || p.x >= m_width || p.y < 0 || p.y >= m_height) {
        Log.error("click point out of range");
    }

    Log.trace(m_use_maa_touch ? "maatouch" : "minitouch", "click:", p);
    bool ret = m_minitoucher->down(p.x, p.y) && m_minitoucher->up();
    if (ret) m_minitoucher->extra_sleep();
    return ret;
}

bool asst::MinitouchController::swipe(const Point& p1, const Point& p2, int duration, bool extra_swipe, double slope_in,
                                      double slope_out, bool with_pause)
{
    if (!m_minitoucher) {
        Log.error("minitoucher is not initialized");
        return false;
    }

    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    // 起点不能在屏幕外，但是终点可以
    if (x1 < 0 || x1 >= m_width || y1 < 0 || y1 >= m_height) {
        Log.warn("swipe point1 is out of range", x1, y1);
        x1 = std::clamp(x1, 0, m_width - 1);
        y1 = std::clamp(y1, 0, m_height - 1);
    }

    Log.trace(m_use_maa_touch ? "maatouch" : "minitouch", "swipe", p1, p2, duration, extra_swipe, slope_in, slope_out);
    if (!m_minitoucher->down(x1, y1)) return false;

    constexpr int TimeInterval = Minitoucher::DefaultSwipeDelay;

    auto cubic_spline = [](double slope_0, double slope_1, double t) {
        const double a = slope_0;
        const double b = -(2 * slope_0 + slope_1 - 3);
        const double c = -(-slope_0 - slope_1 + 2);
        return a * t + b * std::pow(t, 2) + c * std::pow(t, 3);
    }; // TODO: move this to math.hpp

    bool need_pause = with_pause && use_swipe_with_pause();
    const auto& opt = Config.get_options();
    std::future<void> pause_future;
    auto minitouch_move = [&](int _x1, int _y1, int _x2, int _y2, int _duration) -> bool {
        for (int cur_time = TimeInterval; cur_time < _duration; cur_time += TimeInterval) {
            double progress = cubic_spline(slope_in, slope_out, static_cast<double>(cur_time) / duration);
            int cur_x = static_cast<int>(std::lerp(_x1, _x2, progress));
            int cur_y = static_cast<int>(std::lerp(_y1, _y2, progress));
            if (need_pause && std::sqrt(std::pow(cur_x - _x1, 2) + std::pow(cur_y - _y1, 2)) >
                                  opt.swipe_with_pause_required_distance) {
                need_pause = false;
                if (m_use_maa_touch) {
                    constexpr int EscKeyCode = 111;
                    if (!m_minitoucher->key_down(EscKeyCode)) return false;
                    if (!m_minitoucher->key_up(EscKeyCode, 0)) return false;
                }
                else {
                    pause_future = std::async(std::launch::async, [&]() { press_esc(); });
                }
            }
            if (cur_x < 0 || cur_x > m_minitouch_props.max_x || cur_y < 0 || cur_y > m_minitouch_props.max_y) {
                continue;
            }
            if (!m_minitoucher->move(cur_x, cur_y)) return false;
        }
        if (_x2 >= 0 && _x2 <= m_minitouch_props.max_x && _y2 >= 0 && _y2 <= m_minitouch_props.max_y) {
            if (!m_minitoucher->move(_x2, _y2)) return false;
        }
        return true;
    };

    if (!minitouch_move(x1, y1, x2, y2, duration ? duration : opt.minitouch_swipe_default_duration)) return false;

    if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
        if (!m_minitoucher->wait(opt.minitouch_swipe_extra_end_delay)) return false; // 停留终点
        if (!minitouch_move(x2, y2, x2, y2 - opt.minitouch_extra_swipe_dist, opt.minitouch_extra_swipe_duration))
            return false;
    }
    if (!m_minitoucher->up()) return false;
    m_minitoucher->extra_sleep();
    return true;
}

bool asst::MinitouchController::inject_input_event(const InputEvent& event)
{
    LogTraceFunction;

    if (!m_minitoucher) {
        Log.error("minitoucher is not initialized");
        return false;
    }

    switch (event.type) {
    case InputEvent::Type::KEY_DOWN:
        return m_minitoucher->key_down(event.keycode, 0, false);
    case InputEvent::Type::KEY_UP:
        return m_minitoucher->key_up(event.keycode, 0, false);
    case InputEvent::Type::TOUCH_DOWN:
        return m_minitoucher->down(event.point.x, event.point.y, 0, false, event.pointerId);
    case InputEvent::Type::TOUCH_UP:
        return m_minitoucher->up(0, false, event.pointerId);
    case InputEvent::Type::TOUCH_MOVE:
        return m_minitoucher->move(event.point.x, event.point.y, 0, false, event.pointerId);
    case InputEvent::Type::TOUCH_RESET:
        return m_minitoucher->reset();
    case InputEvent::Type::WAIT_MS:
        return m_minitoucher->wait(event.milisec);
    case InputEvent::Type::COMMIT:
        return m_minitoucher->commit();
    case InputEvent::Type::UNKNOWN:
    default:
        Log.error("unknown input event type");
        return false;
    }
}

asst::ControlFeat::Feat asst::MinitouchController::support_features() const noexcept
{
    auto feat = ControlFeat::PRECISE_SWIPE;
    if (use_swipe_with_pause()) {
        feat |= ControlFeat::SWIPE_WITH_PAUSE;
    }
    return feat;
}

void asst::MinitouchController::clear_info() noexcept
{
    AdbController::clear_info();
    m_minitouch_available = false;
    m_minitouch_props = decltype(m_minitouch_props)();
}

bool asst::MinitouchController::probe_minitouch(const AdbCfg& adb_cfg,
                                                std::function<std::string(const std::string&)> cmd_replace)
{
    LogTraceFunction;

    std::string_view touch_program;
    if (m_use_maa_touch) {
        touch_program = "maatouch";
        m_minitouch_props.orientation = 0;
    }
    else {
        std::string abilist = call_command(cmd_replace(adb_cfg.abilist)).value_or(std::string());
        for (const auto& abi : Config.get_options().minitouch_programs_order) {
            if (abilist.find(abi) != std::string::npos) {
                touch_program = abi;
                break;
            }
        }
        std::string orientation_str = call_command(cmd_replace(adb_cfg.orientation)).value_or("0");
        if (!orientation_str.empty()) {
            char first = orientation_str.front();
            if (first == '0' || first == '1' || first == '2' || first == '3') {
                m_minitouch_props.orientation = static_cast<int>(first - '0');
            }
        }
    }
    Log.info("touch_program", touch_program, "orientation", m_minitouch_props.orientation);

    if (touch_program.empty()) return false;

    auto minitouch_cmd_rep = [&](const std::string& cfg_cmd) -> std::string {
        using namespace asst::utils::path_literals;
        return utils::string_replace_all(
            cmd_replace(cfg_cmd),
            {
                { "[minitouchLocalPath]",
                  utils::path_to_utf8_string(ResDir.get() / "minitouch"_p / touch_program / "minitouch"_p) },
                { "[minitouchWorkingFile]", m_uuid },
            });
    };

    if (!call_command(minitouch_cmd_rep(adb_cfg.push_minitouch))) return false;
    if (!call_command(minitouch_cmd_rep(adb_cfg.chmod_minitouch))) return false;

    m_adb.call_minitouch = minitouch_cmd_rep(adb_cfg.call_minitouch);
    m_adb.call_maatouch = minitouch_cmd_rep(adb_cfg.call_maatouch);

    if (!call_and_hup_minitouch()) return false;

    return true;
}

bool asst::MinitouchController::connect(const std::string& adb_path, const std::string& address,
                                        const std::string& config)
{
    LogTraceFunction;

    release_minitouch();

#ifdef ASST_DEBUG
    if (config == "DEBUG") {
        m_inited = true;
        return true;
    }
#endif

    auto ret = AdbController::connect(adb_path, address, config);

    if (!ret) {
        return false;
    }

    auto get_info_json = [&]() -> json::value {
        return json::object {
            { "uuid", m_uuid },
            { "details",
              json::object {
                  { "adb", adb_path },
                  { "address", address },
                  { "config", config },
              } },
        };
    };

    std::string display_id;
    std::string nc_address = "10.0.2.2";
    uint16_t nc_port = 0;

    auto cmd_replace = [&](const std::string& cfg_cmd) -> std::string {
        return utils::string_replace_all(cfg_cmd, {
                                                      { "[Adb]", adb_path },
                                                      { "[AdbSerial]", address },
                                                      { "[DisplayId]", display_id },
                                                      { "[NcPort]", std::to_string(nc_port) },
                                                      { "[NcAddress]", nc_address },
                                                  });
    };

    auto adb_ret = Config.get_adb_cfg(config);

    if (!adb_ret) {
        return false;
    }

    const auto& adb_cfg = adb_ret.value();

    m_minitouch_available = probe_minitouch(adb_cfg, cmd_replace);

    if (!m_minitouch_available) {
        json::value info = get_info_json() | json::object {
            { "what", "TouchModeNotAvailable" },
            { "why", "" },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }

    return true;
}

void asst::MinitouchController::back_to_home() noexcept
{
    AdbController::back_to_home();
    return;
}
