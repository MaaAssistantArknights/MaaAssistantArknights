#include "MaatouchController.h"

#include "Utils/Logger.hpp"

bool asst::MaatouchController::inject_input_event(const InputEvent& event)
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

bool asst::MaatouchController::press_esc()
{
    constexpr int EscKeyCode = 111;
    if (!m_minitoucher->key_down(EscKeyCode, 0)) {
        return false;
    }
    if (!m_minitoucher->key_up(EscKeyCode, 0)) {
        return false;
    }
    return true;
}
