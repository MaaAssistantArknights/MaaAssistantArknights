#include "MacNativeController.h"
#include "MaaUtils/NoWarningCV.hpp"

#if defined(__APPLE__) && ASST_WITH_MAC_NATIVE

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include <dlfcn.h>
#include <mach/mach_time.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "SwipeHelper.hpp"
#include "Utils/Logger.hpp"
#ifdef ASST_DEBUG
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Platform.hpp"
#include "Utils/WorkingDir.hpp"
#endif

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
    size_t requested_width = 0;
    size_t requested_height = 0;
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
    const auto handler = ^(SCShareableContent* content, NSError* error) {
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
            configuration.ignoreShadowsSingleWindow = YES;
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
            constexpr double MaxCaptureWidth = 1920.0;
            const double capture_scale = std::min(point_pixel_scale, MaxCaptureWidth / capture_rect.size.width);
            result->requested_width = static_cast<size_t>(std::lround(capture_rect.size.width * capture_scale));
            result->requested_height = static_cast<size_t>(std::lround(capture_rect.size.height * capture_scale));
            configuration.width = result->requested_width;
            configuration.height = result->requested_height;
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
                                          }
                                          dispatch_semaphore_signal(result->semaphore);
                                      }];
        } else {
            result->error = "MacNative capture requires macOS 14 or later.";
            dispatch_semaphore_signal(result->semaphore);
        }
        [configuration release];
        [filter release];
    };
    [SCShareableContent getShareableContentExcludingDesktopWindows:YES
                                               onScreenWindowsOnly:YES
                                                 completionHandler:handler];
}

double row_chromatic_fraction(const cv::Mat& image, int y)
{
    const int step = std::max(1, image.cols / 640);
    size_t colored = 0;
    size_t sampled = 0;
    for (int x = 0; x < image.cols; x += step) {
        const auto& pixel = image.at<cv::Vec3b>(y, x);
        const auto max_channel = std::max({ pixel[0], pixel[1], pixel[2] });
        const auto min_channel = std::min({ pixel[0], pixel[1], pixel[2] });
        if (max_channel - min_channel > 14) {
            ++colored;
        }
        ++sampled;
    }
    return sampled == 0 ? 0.0 : static_cast<double>(colored) / sampled;
}

int detect_title_bar_height(const cv::Mat& image)
{
    if (image.empty() || image.rows < 30) {
        return 0;
    }

    constexpr double HeaderChromaLimit = 0.06;
    constexpr double ContentChromaLimit = 0.12;
    const int probe_rows = std::min(6, image.rows);
    for (int y = 0; y < probe_rows; ++y) {
        if (row_chromatic_fraction(image, y) > HeaderChromaLimit) {
            return 0;
        }
    }

    const int max_header_height = std::min(image.rows / 6, 120);
    for (int y = probe_rows; y + 2 < max_header_height; ++y) {
        if (row_chromatic_fraction(image, y) > ContentChromaLimit
            && row_chromatic_fraction(image, y + 1) > ContentChromaLimit
            && row_chromatic_fraction(image, y + 2) > ContentChromaLimit) {
            return y;
        }
    }
    return 0;
}

cv::Rect detect_game_content(const cv::Mat& image)
{
    // The capture preserves the window aspect ratio, so the 16:9 game fills
    // the width below the title bar. Dark game content must not alter bounds.
    const int top = image.rows - static_cast<int>(std::lround(image.cols * 9.0 / 16.0));
    if (top > 0 && top <= std::min(image.rows / 6, 120)) {
        return { 0, top, image.cols, image.rows - top };
    }
    const int title_bar_height = detect_title_bar_height(image);
    return { 0, title_bar_height, image.cols, image.rows - title_bar_height };
}

#ifdef ASST_DEBUG
void prune_mac_native_images(const std::filesystem::path& directory)
{
    constexpr size_t MaxRawImages = 4000;
    static std::mutex prune_mutex;
    std::scoped_lock lock(prune_mutex);
    using ImageFile = std::pair<std::filesystem::file_time_type, std::filesystem::path>;
    std::vector<ImageFile> raw_images;
    std::error_code error;
    std::filesystem::directory_iterator iter(directory, std::filesystem::directory_options::skip_permission_denied, error);
    if (error) {
        LogWarn << "Cannot inspect MacNative screenshot directory" << directory << error.message();
        return;
    }
    for (const std::filesystem::directory_iterator end; iter != end; iter.increment(error)) {
        if (error) {
            LogWarn << "Cannot iterate MacNative screenshots" << directory << error.message();
            break;
        }
        const auto& path = iter->path();
        const auto name = path.filename().string();
        if ((path.extension() != ".png" && path.extension() != ".jpg")
            || !iter->is_regular_file(error)) {
            error.clear();
            continue;
        }
        const auto time = std::filesystem::last_write_time(path, error);
        if (error) {
            LogWarn << "Cannot inspect MacNative screenshot" << path << error.message();
            error.clear();
            continue;
        }
        raw_images.emplace_back(time, path);
    }
    std::sort(raw_images.begin(), raw_images.end(), [](const ImageFile& lhs, const ImageFile& rhs) {
        return lhs.first == rhs.first ? lhs.second < rhs.second : lhs.first < rhs.first;
    });
    const size_t excess = raw_images.size() > MaxRawImages ? raw_images.size() - MaxRawImages : 0;
    for (size_t i = 0; i < excess; ++i) {
        std::filesystem::remove(raw_images[i].second, error);
        if (error) {
            LogWarn << "Cannot remove old MacNative screenshot" << raw_images[i].second << error.message();
            error.clear();
        }
    }
}
#endif

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
        // ScreenCaptureKit's image already has the top-left orientation expected by the controller.
        CGContextDrawImage(context, CGRectMake(0, 0, width, height), result->image);
        CGContextRelease(context);
        cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);
        frame = result->frame;

#ifdef ASST_DEBUG
        static std::atomic_uint64_t capture_sequence = 0;
        const auto capture_id = capture_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
        const auto debug_dir = asst::utils::path("debug") / asst::utils::path("MacNative");
        const auto capture_suffix = std::to_string(capture_id);
        if (!asst::utils::save_debug_image(bgr, debug_dir, false, "", "window_" + capture_suffix, "jpg", { cv::IMWRITE_JPEG_QUALITY, 65 })) {
            LogWarn << "Failed to save the MacNative window screenshot" << capture_id;
        }
        prune_mac_native_images(asst::UserDir.get() / debug_dir);
#endif

        const cv::Rect content = detect_game_content(bgr);
        if (content.width <= 0 || content.height <= 0) {
            LogError << "MacNative could not determine the game content bounds.";
            return false;
        }

        const double source_scale_x = frame.scale_x;
        const double source_scale_y = frame.scale_y;
        frame.capture_offset_x += content.x / source_scale_x;
        frame.capture_offset_y += content.y / source_scale_y;
        frame.capture_width = content.width / source_scale_x;
        frame.capture_height = content.height / source_scale_y;
        constexpr int NormalizedWidth = 1280;
        constexpr int NormalizedHeight = 720;
        frame.pixel_width = NormalizedWidth;
        frame.pixel_height = NormalizedHeight;
        frame.scale_x = frame.pixel_width / frame.capture_width;
        frame.scale_y = frame.pixel_height / frame.capture_height;

        cv::Mat normalized;
        cv::resize(
            bgr(content),
            normalized,
            cv::Size(frame.pixel_width, frame.pixel_height),
            0.0,
            0.0,
            cv::INTER_AREA);
        bgr = std::move(normalized);
#ifdef ASST_DEBUG
        LogDebug << "MacNative capture" << capture_id
                 << "window" << frame.window_id << "origin" << frame.x << frame.y
                 << "window size" << frame.width << frame.height
                 << "requested size" << result->requested_width << result->requested_height
                 << "raw size" << width << height
                 << "content rect" << content.x << content.y << content.width << content.height
                 << "capture offset" << frame.capture_offset_x << frame.capture_offset_y
                 << "capture size" << frame.capture_width << frame.capture_height;
#endif
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
    CGEventRef get() const { return m_event; }

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
    // Keep edge coordinates inside the content; a point exactly on the window
    // border can be ignored by the native game even though it is in the image.
    constexpr double BorderInset = 2.0;
    return CGPointMake(
        frame.capture_offset_x + std::clamp(point.x / frame.scale_x, BorderInset, frame.capture_width - BorderInset),
        frame.capture_offset_y + std::clamp(point.y / frame.scale_y, BorderInset, frame.capture_height - BorderInset));
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
    constexpr int EdgeInset = 10;
    const asst::Point inset_point {
        std::clamp(point.x, EdgeInset, std::max(EdgeInset, frame.pixel_width - EdgeInset - 1)),
        std::clamp(point.y, EdgeInset, std::max(EdgeInset, frame.pixel_height - EdgeInset - 1)),
    };
    const auto local = local_point(frame, inset_point);
    const auto screen = screen_point(frame, local);
    LogDebug << "MacNative click mapping"
             << "image point" << point.x << point.y
             << "inset image point" << inset_point.x << inset_point.y
             << "window point" << local.x << local.y
             << "screen point" << screen.x << screen.y
             << "window origin" << frame.x << frame.y
             << "capture offset" << frame.capture_offset_x << frame.capture_offset_y
             << "capture size" << frame.capture_width << frame.capture_height
             << "scale" << frame.scale_x << frame.scale_y;
    if (!post_event(make_scroll(1, 0, kCGScrollPhaseBegan), frame, local, screen) || !post_event(make_gesture(false, kCGScrollPhaseBegan, 0, 0), frame, local, screen) || !post_event(make_gesture(true, kCGScrollPhaseBegan, 1, 0), frame, local, screen)) {
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    return post_event(make_scroll(0, 0, kCGScrollPhaseEnded), frame, local, screen) && post_event(make_gesture(false, kCGScrollPhaseEnded, 0, 0), frame, local, screen) && post_event(make_gesture(true, kCGScrollPhaseEnded, 0, 0), frame, local, screen);
}

bool post_swipe(
    const NativeFrame& frame,
    const asst::Point& from,
    const asst::Point& to,
    int duration_ms,
    double slope_in,
    double slope_out)
{
    if (!AXIsProcessTrusted()) {
        LogError << "Accessibility permission is required for MacNative input.";
        return false;
    }
    const auto steps = std::clamp(duration_ms / 10, 1, 80);
    auto previous_local = local_point(frame, from);
    const auto start_screen = screen_point(frame, previous_local);
    if (!post_event(make_scroll(0, 0, kCGScrollPhaseBegan), frame, previous_local, start_screen)
        || !post_event(make_gesture(false, kCGScrollPhaseBegan, 0, 0), frame, previous_local, start_screen)
        || !post_event(make_gesture(true, kCGScrollPhaseBegan, 0, 0), frame, previous_local, start_screen)) {
        return false;
    }
    const auto start = std::chrono::steady_clock::now();
    double previous_progress = 0.0;

    for (int i = 1; i <= steps; ++i) {
        const auto deadline = start + std::chrono::nanoseconds(static_cast<int64_t>(duration_ms) * 1'000'000 * i / steps);
        std::this_thread::sleep_until(deadline);
        const double progress = std::max(
            previous_progress,
            std::clamp(asst::cubic_spline(slope_in, slope_out, static_cast<double>(i) / steps), 0.0, 1.0));
        const asst::Point pixel_point {
            static_cast<int>(std::lround(from.x + (to.x - from.x) * progress)),
            static_cast<int>(std::lround(from.y + (to.y - from.y) * progress)),
        };
        const auto local = local_point(frame, pixel_point);
        const auto screen = screen_point(frame, local);
        const auto step_x = local.x - previous_local.x;
        const auto step_y = local.y - previous_local.y;
        if (!post_event(make_scroll(
                            static_cast<int32_t>(std::lround(step_x)),
                            static_cast<int32_t>(std::lround(step_y)), kCGScrollPhaseChanged),
                frame, local, screen)
            || !post_event(make_gesture(false, kCGScrollPhaseChanged, 0, 0), frame, local, screen)
            || !post_event(make_gesture(true, kCGScrollPhaseChanged, step_x, step_y), frame, local, screen)) {
            return false;
        }
        previous_local = local;
        previous_progress = progress;
    }

    const auto local = local_point(frame, to);
    const auto screen = screen_point(frame, local);
    return post_event(make_scroll(0, 0, kCGScrollPhaseEnded), frame, local, screen) && post_event(make_gesture(false, kCGScrollPhaseEnded, 0, 0), frame, local, screen) && post_event(make_gesture(true, kCGScrollPhaseEnded, 0, 0), frame, local, screen);
}

bool same_window_geometry(const NativeFrame& lhs, const NativeFrame& rhs)
{
    constexpr double Tolerance = 0.01;
    return lhs.pid == rhs.pid && lhs.window_id == rhs.window_id && std::abs(lhs.width - rhs.width) < Tolerance
        && std::abs(lhs.height - rhs.height) < Tolerance;
}
} // namespace MacNativeDetail

using MacNativeDetail::capture_frame;
using MacNativeDetail::NativeFrame;
using MacNativeDetail::same_window_geometry;

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
        if (!same_window_geometry(frame, next)) {
            LogWarn << "MacNative target window changed; a fresh screenshot is required before input.";
            return false;
        }
        // Use the crop that produced the recognized image for input mapping.
        // Only the window's current screen origin needs to be refreshed.
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
    for (int attempt = 0; attempt < 20; ++attempt) {
        if (m_impl->refresh_frame()) {
            return true;
        }
        if (attempt < 19) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    return false;
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
    if (m_impl->initialized && !same_window_geometry(m_impl->frame, frame)) {
        LogInfo << "MacNative target window geometry changed; the new frame is now active.";
    }
    m_impl->frame = frame;
    m_impl->initialized = true;
    return true;
}

bool asst::MacNativeController::start_game(const std::string& client_type [[maybe_unused]])
{
    constexpr auto GamePath = "/Applications/Arknights.app";
    NSBundle* bundle = [NSBundle bundleWithPath:@"/Applications/Arknights.app"];
    if (!bundle) {
        LogError << "MacNative cannot start the game: expected application at" << GamePath;
        return false;
    }

    NSString* bundle_id = bundle.bundleIdentifier;
    if (!bundle_id || m_impl->bundle_id != bundle_id.UTF8String) {
        LogError << "MacNative game bundle identifier does not match the connected application at" << GamePath;
        return false;
    }

    struct LaunchResult {
        dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
        bool started = false;
        std::string error;

        ~LaunchResult() { dispatch_release(semaphore); }
    };
    const auto result = std::make_shared<LaunchResult>();
    [[NSWorkspace sharedWorkspace] openApplicationAtURL:[NSURL fileURLWithPath:@"/Applications/Arknights.app"]
                                          configuration:[NSWorkspaceOpenConfiguration configuration]
                                      completionHandler:^(NSRunningApplication* app, NSError* error) {
                                          result->started = app != nil;
                                          if (error) {
                                              result->error = error.localizedDescription.UTF8String;
                                          }
                                          dispatch_semaphore_signal(result->semaphore);
                                      }];
    if (dispatch_semaphore_wait(result->semaphore, dispatch_time(DISPATCH_TIME_NOW, 30 * NSEC_PER_SEC)) != 0) {
        LogError << "MacNative timed out while starting the game at" << GamePath;
        return false;
    }
    if (!result->started) {
        LogError << "MacNative could not start the game at" << GamePath << result->error;
        return false;
    }
    return true;
}

bool asst::MacNativeController::stop_game(const std::string& client_type [[maybe_unused]])
{
    NSString* bundle_id = [NSString stringWithUTF8String:m_impl->bundle_id.c_str()];
    NSArray<NSRunningApplication*>* apps = [NSRunningApplication runningApplicationsWithBundleIdentifier:bundle_id];
    if (apps.count == 0) {
        LogInfo << "MacNative game is already stopped" << m_impl->bundle_id;
        return true;
    }
    bool stopped = true;
    for (NSRunningApplication* app in apps) {
        const pid_t pid = app.processIdentifier;
        if (!AXIsProcessTrusted()) {
            LogError << "Accessibility permission is required to quit the MacNative game.";
            stopped = false;
            continue;
        }

        constexpr CGKeyCode QuitKey = 0x0C; // Q on the macOS virtual keyboard.
        MacNativeDetail::EventOwner key_down(CGEventCreateKeyboardEvent(nullptr, QuitKey, true));
        MacNativeDetail::EventOwner key_up(CGEventCreateKeyboardEvent(nullptr, QuitKey, false));
        if (!key_down.get() || !key_up.get()) {
            LogError << "MacNative could not create a quit shortcut for the game process" << pid;
            stopped = false;
            continue;
        }
        CGEventSetFlags(key_down.get(), kCGEventFlagMaskCommand);
        CGEventSetFlags(key_up.get(), kCGEventFlagMaskCommand);
        CGEventPostToPid(pid, key_down.get());
        CGEventPostToPid(pid, key_up.get());

        for (int attempt = 0; attempt < 20 && !app.isTerminated; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        if (!app.isTerminated) {
            LogError << "MacNative game did not quit after Command-Q" << pid;
            stopped = false;
        }
    }
    return stopped;
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
    if (extra_swipe != SwipeExtraDirection::None || with_pause) {
        LogWarn << "MacNative ignores extra movement and pause parameters.";
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
    const bool curved_swipe = slope_in != 1 || slope_out != 1;
    // A 200 ms wheel gesture leaves the native operator list coasting across
    // multiple pages. Give curved swipes enough time to decelerate in place.
    const int duration_ms = curved_swipe ? std::max(duration, 650) : (duration > 0 ? duration : 500);
    // Native gesture scrolling travels farther than the same task swipe on
    // touch controllers. Scale the gesture displacement without changing the
    // shared task coordinates or the starting point.
    constexpr double SwipeDistanceScale = 0.8;
    const Point native_to {
        static_cast<int>(std::lround(from.x + (to.x - from.x) * SwipeDistanceScale)),
        static_cast<int>(std::lround(from.y + (to.y - from.y) * SwipeDistanceScale)),
    };
    const auto local_from = MacNativeDetail::local_point(m_impl->frame, from);
    const auto local_to = MacNativeDetail::local_point(m_impl->frame, native_to);
    LogDebug << "MacNative swipe mapping"
             << "image from" << from.x << from.y
             << "requested image to" << to.x << to.y
             << "native image to" << native_to.x << native_to.y
             << "window from" << local_from.x << local_from.y
             << "window to" << local_to.x << local_to.y
             << "requested duration" << duration
             << "duration" << duration_ms
             << "slope" << slope_in << slope_out;
    return post_swipe(m_impl->frame, from, native_to, std::max(duration_ms, 8), slope_in, slope_out);
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
