#include "MacNativeController.h"
#include "MaaUtils/NoWarningCV.hpp"

#if defined(__APPLE__) && ASST_WITH_MAC_NATIVE

#import <ApplicationServices/ApplicationServices.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include <dlfcn.h>
#include <mach/mach_time.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "Utils/Logger.hpp"

namespace MacNativeDetail {
struct NativeFrame {
    pid_t pid = 0;
    CGWindowID window_id = 0;
    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;
    double capture_offset_x = 0;
    double capture_offset_y = 0;
    double capture_width = 0;
    double capture_height = 0;
    int pixel_width = 0;
    int pixel_height = 0;
    double scale_x = 0;
    double scale_y = 0;
};

struct CaptureResult {
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    SCWindow* window = nil;
    CGImageRef image = nullptr;
    NativeFrame frame;
    std::string error;

    ~CaptureResult()
    {
        if (window) {
            [window release];
        }
        if (image) {
            CGImageRelease(image);
        }
        dispatch_release(semaphore);
    }
};

void capture_window(
    std::string bundle_id,
    const std::shared_ptr<CaptureResult>& result)
{
    [SCShareableContent getShareableContentExcludingDesktopWindows:YES
                                               onScreenWindowsOnly:YES
                                                 completionHandler:^(SCShareableContent* content, NSError* error) {
                                                     if (error || !content) {
                                                         result->error = error.localizedDescription.UTF8String
                                                             ? error.localizedDescription.UTF8String
                                                             : "ScreenCaptureKit could not enumerate windows.";
                                                         dispatch_semaphore_signal(result->semaphore);
                                                         return;
                                                     }

                                                     SCWindow* selected = nil;
                                                     size_t matches = 0;
                                                     for (SCWindow* window in content.windows) {
                                                         NSString* owner = window.owningApplication.bundleIdentifier;
                                                         if (!window.isOnScreen || !owner || bundle_id != owner.UTF8String) {
                                                             continue;
                                                         }
                                                         const CGRect frame = window.frame;
                                                         if (frame.size.width < 600 || frame.size.height < 300) {
                                                             continue;
                                                         }
                                                         selected = window;
                                                         ++matches;
                                                     }
                                                     if (matches != 1) {
                                                         result->error = matches == 0
                                                             ? "No visible game window matches the configured bundle identifier."
                                                             : "More than one visible game window matches the configured bundle identifier.";
                                                         dispatch_semaphore_signal(result->semaphore);
                                                         return;
                                                     }

                                                     result->window = [selected retain];
                                                     const CGRect frame = selected.frame;
                                                     result->frame.pid = selected.owningApplication.processID;
                                                     result->frame.window_id = selected.windowID;
                                                     result->frame.x = frame.origin.x;
                                                     result->frame.y = frame.origin.y;
                                                     result->frame.width = frame.size.width;
                                                     result->frame.height = frame.size.height;

                                                     SCContentFilter* filter = [[SCContentFilter alloc] initWithDesktopIndependentWindow:selected];
                                                     SCStreamConfiguration* configuration = [[SCStreamConfiguration alloc] init];
                                                     configuration.showsCursor = NO;
                                                     if (@available(macOS 14.0, *)) {
                                                         const CGRect capture_rect = filter.contentRect;
                                                         const double point_pixel_scale = filter.pointPixelScale;
                                                         if (capture_rect.size.width <= 0 || capture_rect.size.height <= 0 || point_pixel_scale <= 0) {
                                                             result->error = "ScreenCaptureKit returned invalid capture geometry.";
                                                             dispatch_semaphore_signal(result->semaphore);
                                                             [configuration release];
                                                             [filter release];
                                                             return;
                                                         }
                                                         result->frame.capture_offset_x = capture_rect.origin.x - frame.origin.x;
                                                         result->frame.capture_offset_y = capture_rect.origin.y - frame.origin.y;
                                                         result->frame.capture_width = capture_rect.size.width;
                                                         result->frame.capture_height = capture_rect.size.height;
                                                         [SCScreenshotManager captureImageWithFilter:filter
                                                                                       configuration:configuration
                                                                                   completionHandler:^(CGImageRef image, NSError* capture_error) {
                                                                                       if (capture_error || !image) {
                                                                                           result->error = capture_error.localizedDescription.UTF8String
                                                                                               ? capture_error.localizedDescription.UTF8String
                                                                                               : "ScreenCaptureKit returned an empty window image.";
                                                                                       } else {
                                                                                           result->image = CGImageRetain(image);
                                                                                           result->frame.pixel_width = static_cast<int>(CGImageGetWidth(image));
                                                                                           result->frame.pixel_height = static_cast<int>(CGImageGetHeight(image));
                                                                                           result->frame.scale_x = result->frame.pixel_width / result->frame.capture_width;
                                                                                           result->frame.scale_y = result->frame.pixel_height / result->frame.capture_height;
                                                                                           const double scale_tolerance = 0.05;
                                                                                           if (std::abs(result->frame.scale_x - point_pixel_scale) > scale_tolerance || std::abs(result->frame.scale_y - point_pixel_scale) > scale_tolerance) {
                                                                                               result->error = "Captured pixels do not match ScreenCaptureKit's window geometry.";
                                                                                           }
                                                                                       }
                                                                                       dispatch_semaphore_signal(result->semaphore);
                                                                                   }];
                                                     } else {
                                                         result->error = "MacNative capture requires macOS 14 or later.";
                                                         dispatch_semaphore_signal(result->semaphore);
                                                     }
                                                     [configuration release];
                                                     [filter release];
                                                 }];
}

bool capture_frame(std::string_view bundle_id, cv::Mat& bgr, NativeFrame& frame)
{
    if (!CGPreflightScreenCaptureAccess()) {
        LogError << "Screen Recording permission is required for MacNative capture.";
        return false;
    }
    if (@available(macOS 14.0, *)) {
        auto result = std::make_shared<CaptureResult>();
        capture_window(std::string(bundle_id), result);
        const auto wait_result = dispatch_semaphore_wait(
            result->semaphore,
            dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));
        if (wait_result != 0) {
            LogError << "Timed out while capturing the MacNative target window.";
            return false;
        }
        if (!result->error.empty() || !result->image) {
            LogError << "MacNative capture failed: " << result->error;
            return false;
        }

        const int width = result->frame.pixel_width;
        const int height = result->frame.pixel_height;
        cv::Mat bgra(height, width, CV_8UC4);
        auto color_space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        if (!color_space) {
            LogError << "Cannot create the MacNative image color space.";
            return false;
        }
        auto context = CGBitmapContextCreate(
            bgra.data,
            static_cast<size_t>(width),
            static_cast<size_t>(height),
            8,
            bgra.step,
            color_space,
            static_cast<CGBitmapInfo>(kCGBitmapByteOrder32Little) | static_cast<CGBitmapInfo>(kCGImageAlphaPremultipliedFirst));
        if (color_space) {
            CGColorSpaceRelease(color_space);
        }
        if (!context) {
            LogError << "Cannot allocate the MacNative image conversion context.";
            return false;
        }
        CGContextTranslateCTM(context, 0, height);
        CGContextScaleCTM(context, 1, -1);
        CGContextDrawImage(context, CGRectMake(0, 0, width, height), result->image);
        CGContextRelease(context);
        cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);
        frame = result->frame;
        return true;
    }
    LogError << "MacNative capture requires macOS 14 or later.";
    return false;
}

class EventOwner {
public:
    explicit EventOwner(CGEventRef event = nullptr)
        : m_event(event)
    {
    }
    ~EventOwner()
    {
        if (m_event)
            CFRelease(m_event);
    }
    EventOwner(const EventOwner&) = delete;
    EventOwner& operator=(const EventOwner&) = delete;

private:
    CGEventRef m_event = nullptr;
};

struct GestureRoute {
    using PostToPid = void (*)(pid_t, CGEventRef);
    using SetWindowLocation = void (*)(CGEventRef, CGPoint);

    void* framework = nullptr;
    PostToPid post = nullptr;
    SetWindowLocation set_window_location = nullptr;
};

GestureRoute& gesture_route()
{
    static GestureRoute route;
    static std::once_flag initialized;
    std::call_once(initialized, [] {
        route.framework = dlopen(
            "/System/Library/PrivateFrameworks/SkyLight.framework/SkyLight",
            RTLD_LAZY | RTLD_LOCAL);
        if (!route.framework) {
            return;
        }
        route.post = reinterpret_cast<GestureRoute::PostToPid>(
            dlsym(route.framework, "SLEventPostToPid"));
        route.set_window_location = reinterpret_cast<GestureRoute::SetWindowLocation>(
            dlsym(route.framework, "SLEventSetWindowLocation"));
        if (!route.set_window_location) {
            route.set_window_location = reinterpret_cast<GestureRoute::SetWindowLocation>(
                dlsym(RTLD_DEFAULT, "CGEventSetWindowLocation"));
        }
    });
    return route;
}

// The private event layout follows the verified sequence in trycua's
// src/macos_gesture.hpp and may change with macOS.
constexpr CGEventType GestureType = static_cast<CGEventType>(29);
constexpr CGEventField GestureSubtype = static_cast<CGEventField>(110);
constexpr CGEventField GesturePhase = static_cast<CGEventField>(132);
constexpr CGEventField GestureTranslationFlag = static_cast<CGEventField>(135);
constexpr CGEventField GestureDeltaX = static_cast<CGEventField>(118);
constexpr CGEventField GestureDeltaY = static_cast<CGEventField>(119);
constexpr CGEventField TargetWindow = static_cast<CGEventField>(51);
constexpr int64_t SyntheticTag = 0x4D41414745535452LL;

CGEventRef make_scroll(int32_t dx, int32_t dy, CGScrollPhase phase)
{
    auto event = CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitPixel, 2, dy, dx);
    if (event) {
        CGEventSetIntegerValueField(event, kCGScrollWheelEventIsContinuous, 1);
        CGEventSetIntegerValueField(event, kCGScrollWheelEventScrollPhase, phase);
        CGEventSetIntegerValueField(event, kCGScrollWheelEventMomentumPhase, 0);
    }
    return event;
}

CGEventRef make_gesture(bool translation, CGScrollPhase phase, double dx, double dy)
{
    auto event = CGEventCreate(nullptr);
    if (!event) {
        return nullptr;
    }
    CGEventSetType(event, GestureType);
    if (translation) {
        CGEventSetIntegerValueField(event, GestureSubtype, 6);
        CGEventSetIntegerValueField(event, GesturePhase, phase);
        CGEventSetIntegerValueField(event, GestureTranslationFlag, 1);
        CGEventSetDoubleValueField(event, GestureDeltaX, dx);
        CGEventSetDoubleValueField(event, GestureDeltaY, dy);
    }
    return event;
}

uint64_t event_timestamp_ns()
{
    mach_timebase_info_data_t info { };
    mach_timebase_info(&info);
    return static_cast<uint64_t>(
        static_cast<__uint128_t>(mach_absolute_time()) * info.numer / info.denom);
}

bool post_event(CGEventRef event, const NativeFrame& frame, CGPoint local, CGPoint screen)
{
    EventOwner owner(event);
    auto& route = gesture_route();
    if (!event || !route.post || !route.set_window_location) {
        LogError << "SkyLight gesture delivery is unavailable on this macOS system.";
        return false;
    }
    CGEventSetLocation(event, screen);
    route.set_window_location(event, local);
    CGEventSetIntegerValueField(event, kCGEventTargetUnixProcessID, frame.pid);
    CGEventSetIntegerValueField(event, kCGEventSourceUnixProcessID, frame.pid);
    CGEventSetIntegerValueField(event, TargetWindow, frame.window_id);
    CGEventSetIntegerValueField(event, kCGMouseEventWindowUnderMousePointer, frame.window_id);
    CGEventSetIntegerValueField(
        event, kCGMouseEventWindowUnderMousePointerThatCanHandleThisEvent, frame.window_id);
    CGEventSetIntegerValueField(event, kCGEventSourceUserData, SyntheticTag);
    CGEventSetTimestamp(event, event_timestamp_ns());
    route.post(frame.pid, event);
    return true;
}

CGPoint local_point(const NativeFrame& frame, const asst::Point& point)
{
    // ScreenCaptureKit reports the selected capture rectangle relative to the
    // window frame, so title-bar and crop offsets need no fixed-size guess.
    return CGPointMake(
        frame.capture_offset_x + point.x / frame.scale_x,
        frame.capture_offset_y + point.y / frame.scale_y);
}

CGPoint screen_point(const NativeFrame& frame, CGPoint local)
{
    return CGPointMake(frame.x + local.x, frame.y + local.y);
}

bool post_tap(const NativeFrame& frame, const asst::Point& point)
{
    if (!AXIsProcessTrusted()) {
        LogError << "Accessibility permission is required for MacNative input.";
        return false;
    }
    const auto local = local_point(frame, point);
    const auto screen = screen_point(frame, local);
    if (!post_event(make_scroll(1, 0, kCGScrollPhaseBegan), frame, local, screen) || !post_event(make_gesture(false, kCGScrollPhaseBegan, 0, 0), frame, local, screen) || !post_event(make_gesture(true, kCGScrollPhaseBegan, 1, 0), frame, local, screen)) {
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    return post_event(make_scroll(0, 0, kCGScrollPhaseEnded), frame, local, screen) && post_event(make_gesture(false, kCGScrollPhaseEnded, 0, 0), frame, local, screen) && post_event(make_gesture(true, kCGScrollPhaseEnded, 0, 0), frame, local, screen);
}

bool post_swipe(const NativeFrame& frame, const asst::Point& from, const asst::Point& to, int duration_ms)
{
    if (!AXIsProcessTrusted()) {
        LogError << "Accessibility permission is required for MacNative input.";
        return false;
    }
    const auto delta_x = (to.x - from.x) / frame.scale_x;
    const auto delta_y = (to.y - from.y) / frame.scale_y;
    const auto steps = std::clamp(duration_ms / 8, 1, 33);
    const auto start = std::chrono::steady_clock::now();

    for (int i = 1; i <= steps; ++i) {
        const auto deadline = start + std::chrono::nanoseconds(static_cast<int64_t>(duration_ms) * 1'000'000 * i / steps);
        std::this_thread::sleep_until(deadline);
        const double progress = static_cast<double>(i) / steps;
        const asst::Point pixel_point {
            static_cast<int>(std::lround(from.x + (to.x - from.x) * progress)),
            static_cast<int>(std::lround(from.y + (to.y - from.y) * progress)),
        };
        const auto local = local_point(frame, pixel_point);
        const auto screen = screen_point(frame, local);
        const auto phase = i == 1 ? kCGScrollPhaseBegan : kCGScrollPhaseChanged;
        if (!post_event(make_scroll(
                            static_cast<int32_t>(std::lround(delta_x / steps)),
                            static_cast<int32_t>(std::lround(delta_y / steps)), phase),
                frame, local, screen)
            || !post_event(make_gesture(false, phase, 0, 0), frame, local, screen) || !post_event(make_gesture(true, phase, delta_x / steps, delta_y / steps), frame, local, screen)) {
            return false;
        }
    }

    const auto local = local_point(frame, to);
    const auto screen = screen_point(frame, local);
    return post_event(make_scroll(0, 0, kCGScrollPhaseEnded), frame, local, screen) && post_event(make_gesture(false, kCGScrollPhaseEnded, 0, 0), frame, local, screen) && post_event(make_gesture(true, kCGScrollPhaseEnded, 0, 0), frame, local, screen);
}

bool same_capture_geometry(const NativeFrame& lhs, const NativeFrame& rhs)
{
    constexpr double Tolerance = 0.01;
    return lhs.pid == rhs.pid && lhs.window_id == rhs.window_id && lhs.pixel_width == rhs.pixel_width && lhs.pixel_height == rhs.pixel_height && std::abs(lhs.width - rhs.width) < Tolerance && std::abs(lhs.height - rhs.height) < Tolerance && std::abs(lhs.capture_offset_x - rhs.capture_offset_x) < Tolerance && std::abs(lhs.capture_offset_y - rhs.capture_offset_y) < Tolerance && std::abs(lhs.capture_width - rhs.capture_width) < Tolerance && std::abs(lhs.capture_height - rhs.capture_height) < Tolerance && std::abs(lhs.scale_x - rhs.scale_x) < Tolerance && std::abs(lhs.scale_y - rhs.scale_y) < Tolerance;
}
} // namespace MacNativeDetail

using MacNativeDetail::capture_frame;
using MacNativeDetail::NativeFrame;
using MacNativeDetail::same_capture_geometry;

struct asst::MacNativeController::Impl {
    mutable std::mutex mutex;
    std::string bundle_id;
    NativeFrame frame;
    bool initialized = false;

    bool refresh_frame()
    {
        cv::Mat current;
        NativeFrame next;
        if (!capture_frame(bundle_id, current, next)) {
            return false;
        }
        frame = next;
        initialized = true;
        return true;
    }

    bool validate_frame_for_input()
    {
        if (!initialized) {
            return false;
        }
        cv::Mat current;
        NativeFrame next;
        if (!capture_frame(bundle_id, current, next)) {
            return false;
        }
        if (!same_capture_geometry(frame, next)) {
            LogWarn << "MacNative window geometry changed; a fresh screenshot is required before input.";
            return false;
        }
        // A pure move keeps the screenshot coordinates valid; update only the
        // current screen origin used for background event delivery.
        frame.x = next.x;
        frame.y = next.y;
        return true;
    }
};

asst::MacNativeController::MacNativeController()
    : m_impl(std::make_unique<Impl>())
{
}

asst::MacNativeController::~MacNativeController() = default;

bool asst::MacNativeController::connect(
    const std::string& adb_path [[maybe_unused]], const std::string& address, const std::string& config)
{
    std::scoped_lock lock(m_impl->mutex);
    if (config != "MacNative" || address.empty()) {
        LogError << "MacNative requires a bundle identifier and the MacNative profile.";
        return false;
    }
    m_impl->bundle_id = address;
    m_impl->initialized = false;
    return m_impl->refresh_frame();
}

bool asst::MacNativeController::inited() const noexcept
{
    std::scoped_lock lock(m_impl->mutex);
    return m_impl->initialized;
}

const std::string& asst::MacNativeController::get_uuid() const
{
    return m_impl->bundle_id;
}

size_t asst::MacNativeController::get_pipe_data_size() const noexcept { return 0; }
size_t asst::MacNativeController::get_version() const noexcept { return 0; }

bool asst::MacNativeController::screencap(cv::Mat& image_payload, bool allow_reconnect [[maybe_unused]])
{
    std::scoped_lock lock(m_impl->mutex);
    NativeFrame frame;
    if (!capture_frame(m_impl->bundle_id, image_payload, frame)) {
        return false;
    }
    if (m_impl->initialized && !same_capture_geometry(m_impl->frame, frame)) {
        LogInfo << "MacNative target window geometry changed; the new frame is now active.";
    }
    m_impl->frame = frame;
    m_impl->initialized = true;
    return true;
}

bool asst::MacNativeController::start_game(const std::string& client_type [[maybe_unused]])
{
    LogWarn << "Starting the game is not supported by MacNative.";
    return false;
}

bool asst::MacNativeController::stop_game(const std::string& client_type [[maybe_unused]])
{
    LogWarn << "Stopping the game is not supported by MacNative.";
    return false;
}

bool asst::MacNativeController::click(const Point& point)
{
    std::scoped_lock lock(m_impl->mutex);
    if (!m_impl->validate_frame_for_input()) {
        return false;
    }
    if (point.x < 0 || point.y < 0 || point.x >= m_impl->frame.pixel_width || point.y >= m_impl->frame.pixel_height) {
        LogError << "MacNative click coordinates are outside the captured window.";
        return false;
    }
    return post_tap(m_impl->frame, point);
}

bool asst::MacNativeController::input(const std::string& text [[maybe_unused]])
{
    LogWarn << "Text input is not supported by MacNative.";
    return false;
}

bool asst::MacNativeController::swipe(
    const Point& from,
    const Point& to,
    int duration,
    SwipeExtraDirection extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause)
{
    std::scoped_lock lock(m_impl->mutex);
    if (extra_swipe != SwipeExtraDirection::None || slope_in != 1 || slope_out != 1 || with_pause) {
        LogWarn << "MacNative supports only a straight swipe without pause or extra movement.";
        return false;
    }
    if (!m_impl->validate_frame_for_input()) {
        return false;
    }
    const auto in_bounds = [&](const Point& point) {
        return point.x >= 0 && point.y >= 0 && point.x < m_impl->frame.pixel_width && point.y < m_impl->frame.pixel_height;
    };
    if (!in_bounds(from) || !in_bounds(to)) {
        LogError << "MacNative swipe coordinates are outside the captured window.";
        return false;
    }
    const int duration_ms = duration > 0 ? duration : 500;
    return post_swipe(m_impl->frame, from, to, std::max(duration_ms, 8));
}

bool asst::MacNativeController::inject_input_event(const InputEvent& event [[maybe_unused]])
{
    LogWarn << "Low-level touch injection is not supported by MacNative.";
    return false;
}

bool asst::MacNativeController::press_esc()
{
    LogWarn << "Keyboard input is not supported by MacNative.";
    return false;
}

asst::ControlFeat::Feat asst::MacNativeController::support_features() const noexcept
{
    return ControlFeat::NONE;
}

std::pair<int, int> asst::MacNativeController::get_screen_res() const noexcept
{
    std::scoped_lock lock(m_impl->mutex);
    return { m_impl->frame.pixel_width, m_impl->frame.pixel_height };
}

void asst::MacNativeController::back_to_home() noexcept
{
    LogWarn << "Returning to the home screen is not supported by MacNative.";
}

#endif // defined(__APPLE__) && ASST_WITH_MAC_NATIVE
