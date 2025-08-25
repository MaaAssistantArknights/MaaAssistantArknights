#include "MaatouchController.h"

#include "Utils/Logger.hpp"

bool asst::MaatouchController::call_and_hup_minitouch()
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

    m_minitoucher = std::make_unique<Maatoucher>(
        std::bind(&MaatouchController::input_to_minitouch, this, std::placeholders::_1),
        m_minitouch_props);

    return true;
}

bool asst::MaatouchController::inject_input_event(const InputEvent& event)
{
    LogTraceFunction;

    if (!m_minitoucher) {
        Log.error("minitoucher is not initialized");
        return false;
    }

    auto* maatoucher = dynamic_cast<Maatoucher*>(m_minitoucher.get());
    if (!maatoucher) {
        return false;
    }

    switch (event.type) {
    case InputEvent::Type::KEY_DOWN:
        return maatoucher->key_down(event.keycode, 0, false);
    case InputEvent::Type::KEY_UP:
        return maatoucher->key_up(event.keycode, 0, false);
    case InputEvent::Type::TOUCH_DOWN:
        return maatoucher->down(event.point.x, event.point.y, 0, false, event.pointerId);
    case InputEvent::Type::TOUCH_UP:
        return maatoucher->up(0, false, event.pointerId);
    case InputEvent::Type::TOUCH_MOVE:
        return maatoucher->move(event.point.x, event.point.y, 0, false, event.pointerId);
    case InputEvent::Type::TOUCH_RESET:
        return maatoucher->reset();
    case InputEvent::Type::WAIT_MS:
        return maatoucher->wait(event.milisec);
    case InputEvent::Type::COMMIT:
        return maatoucher->commit();
    case InputEvent::Type::UNKNOWN:
    default:
        Log.error("unknown input event type");
        return false;
    }
}

bool asst::MaatouchController::press_esc()
{
    auto* maatoucher = dynamic_cast<Maatoucher*>(m_minitoucher.get());
    if (!maatoucher) {
        return false;
    }

    constexpr int EscKeyCode = 111;
    if (!maatoucher->key_down(EscKeyCode, 0)) {
        return false;
    }
    if (!maatoucher->key_up(EscKeyCode, 0)) {
        return false;
    }
    return true;
}
