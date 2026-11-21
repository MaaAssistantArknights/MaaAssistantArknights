#include "Controller.h"

#include "Utils/Platform.hpp"

#include <boost/regex.hpp>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "Assistant.h"
#include "MaaUtils/NoWarningCV.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4068)
#endif
#include <zlib/decompress.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "AdbController.h"
#include "ControllerAPI.h"
#include "MaaFwAdbController.h"
#include "MaatouchController.h"
#include "MinitouchController.h"
#include "ModAvdController.h"
#include "PlayToolsController.h"
#if ASST_WITH_EMULATOR_EXTRAS
#include "MumuController.h"
#endif
#ifdef _WIN32
#include "Win32Controller.h"
#endif

#ifdef __ANDROID__
#include "MaaFwAndroidNativeController.h"
#endif

#include "Common/AsstTypes.h"
#include "Utils/Logger.hpp"

asst::Controller::Controller(const AsstCallback& callback, Assistant* inst) :
    InstHelper(inst),
    m_callback(callback)
{
    LogTraceFunction;
}

asst::Controller::~Controller()
{
    LogTraceFunction;
}

std::shared_ptr<asst::ControllerAPI>
    asst::Controller::create_controller(ControllerType type, PlatformType platform_type) const
{
    try {
        switch (type) {
        case ControllerType::Adb:
            return std::make_shared<AdbController>(m_callback, m_inst, platform_type);
        case ControllerType::Minitouch:
            return std::make_shared<MinitouchController>(m_callback, m_inst, platform_type);
        case ControllerType::Maatouch:
            return std::make_shared<MaatouchController>(m_callback, m_inst, platform_type);
        case ControllerType::MacPlayTools:
            return std::make_shared<PlayToolsController>(m_callback, m_inst, platform_type);
        case ControllerType::MaaFwAdb:
            return std::make_shared<MaaFwAdbController>(m_callback, m_inst, platform_type);
        case ControllerType::ModAvd:
            return std::make_shared<ModAvdController>(m_callback, m_inst, platform_type);
#if ASST_WITH_EMULATOR_EXTRAS
        case ControllerType::Mumu:
            return std::make_shared<MumuController>(m_callback, m_inst, platform_type);
#endif
#ifdef __ANDROID__
        case ControllerType::MaaFwAndroidNative:
            Log.debug("Use Android");
            return std::make_shared<MaaFwAndroidNativeController>(m_callback, m_inst);
#endif
        default:
            return nullptr;
        }
    }
    catch (const std::exception& e) {
        Log.error("Unable to create controller: {}", e.what());
        return nullptr;
    }
}

size_t asst::Controller::get_pipe_data_size() const noexcept
{
    return m_controller->get_pipe_data_size();
}

size_t asst::Controller::get_version() const noexcept
{
    return m_controller->get_version();
}

asst::ControllerType asst::Controller::get_controller_type() const noexcept
{
    return m_controller_type;
}

std::pair<int, int> asst::Controller::get_scale_size() const noexcept
{
    return m_scale_size;
}

void asst::Controller::clear_info() noexcept
{
    std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
    m_uuid.clear();
    m_scale_size = { WindowWidthDefault, WindowHeightDefault };
    m_controller = nullptr;
    m_scale_proxy = nullptr;
    m_cache_image.release();
    m_preview_image.release();
}

void asst::Controller::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}

#define CHECK_EXIST(object, return_value)                       \
    if (!object) {                                              \
        Log.error(__FUNCTION__, "|", #object, "is not inited"); \
        return return_value;                                    \
    }

void asst::Controller::sync_params()
{
    if (!m_controller) {
        // 参数没有实时同步，但是在连接时会被同步
        Log.info("skip sync_params, retry when connect");
        return;
    }
    m_controller->set_swipe_with_pause(m_swipe_with_pause);
    m_controller->set_kill_adb_on_exit(m_kill_adb_on_exit);
}

bool asst::Controller::back_to_home()
{
    CHECK_EXIST(m_controller, false);
    m_controller->back_to_home();
    return true;
}

cv::Mat asst::Controller::get_resized_image_cache(bool for_preview) const
{
    const cv::Size d_size(m_scale_size.first, m_scale_size.second);

    std::shared_lock<std::shared_mutex> image_lock(m_image_mutex);
    const auto& image = for_preview && !m_preview_image.empty() ? m_preview_image : m_cache_image;
    if (image.empty()) {
        Log.error("image is empty");
        return { d_size, CV_8UC3 };
    }
    cv::Mat resized_mat;
    cv::resize(image, resized_mat, d_size, 0.0, 0.0, cv::INTER_AREA);
    return resized_mat;
}

bool asst::Controller::start_game(const std::string& client_type)
{
    CHECK_EXIST(m_controller, false);
    return m_controller->start_game(client_type);
}

bool asst::Controller::stop_game(const std::string& client_type)
{
    CHECK_EXIST(m_controller, false);
    return m_controller->stop_game(client_type);
}

bool asst::Controller::click(const Point& p)
{
    CHECK_EXIST(m_controller, false);
    return m_scale_proxy->click(p);
}

bool asst::Controller::click(const Rect& rect)
{
    CHECK_EXIST(m_controller, false);
    return m_scale_proxy->click(rect);
}

bool asst::Controller::input(const std::string& text)
{
    CHECK_EXIST(m_controller, false);
    return m_controller->input(text);
}

bool asst::Controller::resolve_swipe_with_pause(bool with_pause) const noexcept
{
    return with_pause && m_swipe_with_pause;
}

bool asst::Controller::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    SwipeExtraDirection extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause)
{
    CHECK_EXIST(m_controller, false);
    return m_scale_proxy
        ->swipe(p1, p2, duration, extra_swipe, slope_in, slope_out, resolve_swipe_with_pause(with_pause));
}

bool asst::Controller::swipe(
    const Rect& r1,
    const Rect& r2,
    int duration,
    SwipeExtraDirection extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause,
    bool high_resolution_swipe_fix)
{
    CHECK_EXIST(m_controller, false);
    return m_scale_proxy->swipe(
        r1,
        r2,
        duration,
        extra_swipe,
        slope_in,
        slope_out,
        resolve_swipe_with_pause(with_pause),
        high_resolution_swipe_fix);
}

bool asst::Controller::inject_input_event(InputEvent& event)
{
    // 与 click/swipe 一致必须经 scale proxy，否则任务层的基准坐标未乘分辨率倍率直发设备
    CHECK_EXIST(m_scale_proxy, false);
    return m_scale_proxy->inject_input_event(event);
}

bool asst::Controller::press_esc()
{
    LogTraceFunction;

    CHECK_EXIST(m_controller, false);
    return m_controller->press_esc();
}

asst::ControlFeat::Feat asst::Controller::support_features()
{
    static_assert(std::is_integral_v<ControlFeat::Feat>, "ControlFeat::Feat must be an integral bitmask type");
    static_assert(
        (ControlFeat::SWIPE_WITH_PAUSE & (ControlFeat::SWIPE_WITH_PAUSE - 1)) == 0,
        "ControlFeat::SWIPE_WITH_PAUSE must be a single bit flag");

    CHECK_EXIST(m_controller, ControlFeat::NONE);
    auto feat = m_controller->support_features();
    if (!m_swipe_with_pause) {
        feat &= ~ControlFeat::SWIPE_WITH_PAUSE;
    }
    return feat;
}

bool asst::Controller::connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    LogTraceFunction;

    clear_info();

    // attach_window 会将 m_controller_type 置为 Win32，连接前按 touch mode 重新派生
    set_touch_mode(m_touch_mode);

    {
        std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
        m_controller = create_controller(m_controller_type, m_platform_type);
    }
    if (!m_controller) {
        Log.error("connect failed");
        return false;
    }

    sync_params();

    if (!m_controller->connect(adb_path, address, config)) {
        Log.error("connect failed");
        std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
        m_controller = nullptr;
        return false;
    }

    m_uuid = m_controller->get_uuid();

#ifdef ASST_DEBUG
    if (config == "DEBUG") {
        return true;
    }
#endif

    // Android uses lazy loading; no need to check in advance
#ifndef __ANDROID__
    // try to find the fastest way
    if (!screencap()) {
        Log.error("Cannot find a proper way to screencap!");
        return false;
    }
#endif

    auto proxy_callback = [&](const json::object& details) {
        json::value connection_info = json::object {
            { "uuid", m_uuid },
            { "details",
              json::object {
                  { "adb", adb_path },
                  { "address", address },
                  { "config", config },
              } },
        } | details;
        callback(AsstMsg::ConnectionInfo, connection_info);
    };

    try {
        m_scale_proxy = std::make_shared<ControlScaleProxy>(m_controller, m_controller_type, proxy_callback);
    }
    catch (const std::exception& e) {
        Log.error("Cannot create controller proxy: {}", e.what());
        return false;
    }

    if (!m_scale_proxy) {
        Log.error("Cannot create controller proxy!");
        return false;
    }

    m_scale_size = m_scale_proxy->get_scale_size();

    return true;
}

#ifdef _WIN32
bool asst::Controller::attach_window(
    void* hwnd,
    Win32ScreencapMethod screencap_method,
    Win32InputMethod mouse_method,
    Win32InputMethod keyboard_method)
{
    LogTraceFunction;

    clear_info();

    auto win32_controller = std::make_shared<Win32Controller>(m_callback, m_inst);
    if (!win32_controller->attach(hwnd, screencap_method, mouse_method, keyboard_method)) {
        Log.error("attach_window failed");
        return false;
    }

    m_controller = win32_controller;
    m_controller_type = ControllerType::Win32;
    m_uuid = m_controller->get_uuid();

    // 连接时只需要获取画面，识别前的输入操作由任务截图执行。
    if (!screencap_for_preview()) {
        Log.error("Cannot screencap!");
        return false;
    }

    auto proxy_callback = [&](const json::object& details) {
        json::value connection_info = json::object {
            { "uuid", m_uuid },
            { "details",
              json::object {
                  { "hwnd", reinterpret_cast<uintptr_t>(hwnd) },
                  { "screencap_method", screencap_method },
                  { "mouse_method", mouse_method },
                  { "keyboard_method", keyboard_method },
              } },
        } | details;
        callback(AsstMsg::ConnectionInfo, connection_info);
    };

    try {
        m_scale_proxy = std::make_shared<ControlScaleProxy>(m_controller, m_controller_type, proxy_callback);
    }
    catch (const std::exception& e) {
        Log.error("Cannot create controller proxy: {}", e.what());
        return false;
    }

    if (!m_scale_proxy) {
        Log.error("Cannot create controller proxy!");
        return false;
    }

    m_scale_size = m_scale_proxy->get_scale_size();

    return true;
}

void asst::Controller::restore_window_position()
{
    if (auto* win32_controller = dynamic_cast<Win32Controller*>(m_controller.get()); win32_controller != nullptr) {
        win32_controller->restore_window_position();
    }
}
#endif

bool asst::Controller::inited() noexcept
{
    CHECK_EXIST(m_controller, false);
    return m_controller->inited();
}

void asst::Controller::set_touch_mode(const TouchMode& mode) noexcept
{
    m_touch_mode = mode;

    switch (mode) {
    case TouchMode::Adb:
        m_controller_type = ControllerType::Adb;
        break;
    case TouchMode::Minitouch:
        m_controller_type = ControllerType::Minitouch;
        break;
    case TouchMode::Maatouch:
        m_controller_type = ControllerType::Maatouch;
        break;
    case TouchMode::MacPlayTools:
        m_controller_type = ControllerType::MacPlayTools;
        break;
    case TouchMode::MaaFwAdb:
        m_controller_type = ControllerType::MaaFwAdb;
        break;
    case TouchMode::ModAvdExtras:
        m_controller_type = ControllerType::ModAvd;
        break;
#if ASST_WITH_EMULATOR_EXTRAS
    case TouchMode::MumuExtras:
        m_controller_type = ControllerType::Mumu;
        break;
#endif
#ifdef __ANDROID__
    case TouchMode::Android:
        m_controller_type = ControllerType::MaaFwAndroidNative;
        break;
#endif
    default:
        m_controller_type = ControllerType::Minitouch;
    }
}

void asst::Controller::set_swipe_with_pause(bool enable) noexcept
{
    m_swipe_with_pause = enable;
    sync_params();
}

void asst::Controller::set_adb_lite_enabled(bool enable) noexcept
{
    m_platform_type = enable ? PlatformType::AdbLite : PlatformType::Native;
}

void asst::Controller::set_kill_adb_on_exit(bool enable) noexcept
{
    m_kill_adb_on_exit = enable;
    sync_params();
}

void asst::Controller::set_client_type(const std::string& client_type) noexcept
{
    m_client_type = client_type;
}

const std::string& asst::Controller::get_client_type() const noexcept
{
    return m_client_type;
}

const std::string& asst::Controller::get_uuid() const
{
    return m_uuid;
}

cv::Mat asst::Controller::get_image(bool raw)
{
    if (get_scale_size() == std::pair(0, 0)) {
        Log.error("Unknown image size");
        return {};
    }

    // Some emulators occasionally fail to capture; keep retrying ordinary failures.
    static constexpr int MaxTryCount = 20;
    bool success = false;
    for (int i = 0; i < MaxTryCount && inited(); ++i) {
        if (need_exit()) {
            break;
        }
        const auto timeouts_before_capture = m_consecutive_screencap_timeouts.load();
        if (screencap()) {
            success = true;
            break;
        }
        const auto timeouts_after_capture = m_consecutive_screencap_timeouts.load();
        if (!need_exit() && timeouts_after_capture > timeouts_before_capture && timeouts_after_capture >= 3) {
            throw ScreencapTimeoutException();
        }
    }
    while (!success && !need_exit()) {
        const auto timeouts_before_capture = m_consecutive_screencap_timeouts.load();
        if (screencap(true)) {
            success = true;
            break;
        }
        const auto timeouts_after_capture = m_consecutive_screencap_timeouts.load();
        if (!need_exit() && timeouts_after_capture > timeouts_before_capture && timeouts_after_capture >= 3) {
            throw ScreencapTimeoutException();
        }
        Log.error(__FUNCTION__, "screencap failed!");
        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "ScreencapFailed" },
            { "why", "ScreencapFailed" },
            { "details", json::object {} },
        };
        callback(AsstMsg::ConnectionInfo, info);
        if (m_consecutive_screencap_timeouts == 0) {
            std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
            const cv::Size size(m_scale_size.first, m_scale_size.second);
            m_cache_image = cv::Mat(size, CV_8UC3);
        }
        break;
    }

    if (raw) {
        std::shared_lock<std::shared_mutex> image_lock(m_image_mutex);
        cv::Mat copy = m_cache_image.clone();
        return copy;
    }

    return get_resized_image_cache();
}

cv::Mat asst::Controller::get_image_cache() const
{
    return get_resized_image_cache();
}

cv::Mat asst::Controller::get_preview_image_cache() const
{
    return get_resized_image_cache(true);
}

asst::Controller::ScreencapStatus asst::Controller::screencap_managed(
    const std::shared_ptr<ControllerAPI>& controller,
    cv::Mat& image,
    bool allow_reconnect,
    bool preview,
    std::string& stage)
{
    constexpr auto timeout = std::chrono::seconds(20);
    if (!controller->supports_screencap_cancellation()) {
        const bool success = preview ? controller->screencap_for_preview(image, allow_reconnect)
                                     : controller->screencap(image, allow_reconnect);
        if (success) {
            m_consecutive_screencap_timeouts = 0;
            return ScreencapStatus::Success;
        }
        return ScreencapStatus::Failed;
    }

    std::mutex mutex;
    std::condition_variable condition;
    bool completed = false;
    bool timed_out = false;
    std::thread watchdog([&] {
        std::unique_lock lock(mutex);
        if (condition.wait_for(lock, timeout, [&] { return completed; })) {
            return;
        }
        timed_out = true;
        lock.unlock();
        controller->cancel_screencap();
    });

    bool success = false;
    std::exception_ptr capture_exception;
    try {
        success = preview ? controller->screencap_for_preview(image, allow_reconnect)
                          : controller->screencap(image, allow_reconnect);
    }
    catch (...) {
        capture_exception = std::current_exception();
    }
    {
        std::lock_guard lock(mutex);
        completed = true;
    }
    condition.notify_one();
    watchdog.join();
    if (need_exit()) {
        if (timed_out) {
            controller->reset_screencap();
        }
        return ScreencapStatus::Failed;
    }
    if (capture_exception && !timed_out) {
        std::rethrow_exception(capture_exception);
    }
    if (!timed_out) {
        if (success) {
            m_consecutive_screencap_timeouts = 0;
            return ScreencapStatus::Success;
        }
        return ScreencapStatus::Failed;
    }

    stage = controller->screencap_stage();
    const auto consecutive_timeouts = ++m_consecutive_screencap_timeouts;
    const bool reset = controller->reset_screencap();
    Log.error(
        "Screencap timeout",
        "backend",
        static_cast<int>(m_controller_type),
        "stage",
        stage,
        "timeout_ms",
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count(),
        "consecutive_timeouts",
        consecutive_timeouts,
        "reset",
        reset);
    return ScreencapStatus::Timeout;
}

bool asst::Controller::screencap(bool allow_reconnect)
{
    std::shared_ptr<ControllerAPI> controller;
    std::string stage;
    ScreencapStatus result;
    {
        std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
        controller = m_controller;
        if (!controller) {
            Log.error(__FUNCTION__, "controller is not initialized");
            return false;
        }
        result = screencap_managed(controller, m_cache_image, allow_reconnect, false, stage);
        if (result == ScreencapStatus::Success) {
            // 任务帧更新后，外部预览也应读取最新的画面。
            m_preview_image.release();
        }
        else if (result == ScreencapStatus::Timeout) {
            m_cache_image.release();
            m_preview_image.release();
        }
    }
    if (result == ScreencapStatus::Timeout) {
        const auto consecutive_timeouts = m_consecutive_screencap_timeouts.load();
        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "ScreencapTimeout" },
            { "why", "ScreencapTimeout" },
            { "details",
              json::object {
                  { "backend", static_cast<int>(m_controller_type) },
                  { "stage", stage },
                  { "timeout_ms", 20'000 },
                  { "consecutive_timeouts", consecutive_timeouts },
                  { "will_retry", consecutive_timeouts < 3 },
              } },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }
    return result == ScreencapStatus::Success;
}

bool asst::Controller::screencap_for_preview(bool allow_reconnect)
{
    std::shared_ptr<ControllerAPI> controller;
    cv::Mat image;
    std::string stage;
    ScreencapStatus result;
    {
        std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
        controller = m_controller;
        if (!controller) {
            Log.error(__FUNCTION__, "controller is not initialized");
            return false;
        }
        result = screencap_managed(controller, image, allow_reconnect, true, stage);
        if (result == ScreencapStatus::Success) {
            // 预览帧不能覆盖识别帧，也不能共享可能被截图后端原地改写的缓冲区。
            m_preview_image = std::move(image);
        }
        else if (result == ScreencapStatus::Timeout) {
            m_preview_image.release();
        }
    }
    if (result == ScreencapStatus::Timeout) {
        const auto consecutive_timeouts = m_consecutive_screencap_timeouts.load();
        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "ScreencapTimeout" },
            { "why", "ScreencapTimeout" },
            { "details",
              json::object {
                  { "backend", static_cast<int>(m_controller_type) },
                  { "stage", stage },
                  { "timeout_ms", 20'000 },
                  { "consecutive_timeouts", consecutive_timeouts },
                  { "will_retry", consecutive_timeouts < 3 },
              } },
        };
        callback(AsstMsg::ConnectionInfo, info);
    }
    return result == ScreencapStatus::Success;
}
