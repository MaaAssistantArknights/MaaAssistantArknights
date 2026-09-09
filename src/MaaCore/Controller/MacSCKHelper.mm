#include "MacSCKHelper.h"

#ifdef ASST_WITH_MAC_SCK

#import <Accelerate/Accelerate.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include "Utils/Logger.hpp"

@interface MacSCKOutput : NSObject <SCStreamDelegate, SCStreamOutput>

@property (nonatomic, readonly) CVImageBufferRef buffer;

@property (atomic, assign) BOOL running;

@end

@implementation MacSCKOutput {
    std::atomic_bool _initialized;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _initialized.store(false, std::memory_order_relaxed);
    }
    return self;
}

- (void)stream:(SCStream*)stream didStopWithError:(NSError*)error
{
    if (error) {
        Log.error(__FUNCTION__, "| Stream stopped with error:", error.localizedDescription.UTF8String);
    } else {
        Log.trace(__FUNCTION__, "| Stream stopped without error");
    }
    self.running = NO;
    _initialized.store(true, std::memory_order_release);
    _initialized.notify_all();
}

- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(SCStreamOutputType)type
{
    if (type != SCStreamOutputTypeScreen || !CMSampleBufferIsValid(sampleBuffer)) {
        return;
    }

    auto attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, NO);
    if (!attachments || CFArrayGetCount(attachments) == 0) {
        return;
    }
    bool complete = false;
    if (auto frameInfo = CFArrayGetValueAtIndex(attachments, 0)) {
        if (auto value = CFDictionaryGetValue((CFDictionaryRef)frameInfo, SCStreamFrameInfoStatus)) {
            SCFrameStatus status;
            if (CFNumberGetValue((CFNumberRef)value, kCFNumberNSIntegerType, &status)) {
                switch (status) {
                case SCFrameStatusComplete:
                case SCFrameStatusStarted:
                    complete = true;
                    break;
                default:
                    break;
                }
            }
        }
    }
    if (!complete) {
        return;
    }

    const auto newBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);

    if (newBuffer && _buffer != newBuffer) {
        CFRetain(newBuffer);
        if (_buffer) {
            CFRelease(_buffer);
        }
        _buffer = newBuffer;

        if (!_initialized.load(std::memory_order::relaxed)) {
            _initialized.store(true, std::memory_order_release);
            _initialized.notify_all();
        }
    }
}

- (void)waitForInitialized
{
    _initialized.wait(false, std::memory_order_acquire);
}

- (void)dealloc
{
    if (_buffer) {
        CFRelease(_buffer);
        _buffer = NULL;
    }
    [super dealloc];
}

@end

bool asst::MacSCKHelper::hasPermission()
{
    return CGPreflightScreenCaptureAccess();
}

bool asst::MacSCKHelper::requestPermission()
{
    return CGRequestScreenCaptureAccess();
}

struct asst::MacSCKHelper::Impl {
    ~Impl();

    SCStream* m_stream = nil;
    MacSCKOutput* m_output = nil;

    dispatch_queue_t m_queue = nil;

    bool init(std::string_view bundle_id, std::string_view port, std::pair<int, int> size, std::array<int16_t, 8> rect);
    bool capture(std::vector<uint8_t>& bgrData) const;
};

asst::MacSCKHelper::Impl::~Impl()
{
    const auto stream = m_stream;
    const auto output = m_output;
    const auto queue = m_queue;

    m_stream = nil;
    m_output = nil;
    m_queue = nil;

    const auto cleanup = ^{
        [stream release];
        [output release];
        if (queue) {
            dispatch_release(queue);
        }
    };

    if (stream) {
        [stream stopCaptureWithCompletionHandler:^(NSError* _Nullable error) {
            if (error) {
                Log.error("Error stopping capture:", error.localizedDescription.UTF8String);
            }
            cleanup();
        }];
    } else {
        cleanup();
    }
}

bool asst::MacSCKHelper::Impl::init(std::string_view bundle_id, std::string_view port, std::pair<int, int> size, std::array<int16_t, 8> rect)
{
    auto sem = dispatch_semaphore_create(0);
    __block bool result = false;

    const auto handler = ^(SCShareableContent* _Nullable content, NSError* _Nullable error) {
        if (error) {
            Log.error("Cannot get shareable content:", error.localizedDescription.UTF8String);
            dispatch_semaphore_signal(sem);
            return;
        }

        SCWindow* targetWindow = nil;
        for (SCWindow* window in content.windows) {
            auto bundleID = window.owningApplication.bundleIdentifier.UTF8String;
            if (bundleID && bundleID != bundle_id) {
                continue;
            }
            auto title = window.title.UTF8String;
            if (title && std::string_view(title).find(port) != std::string_view::npos) {
                targetWindow = window;
                break;
            }
        }
        if (!targetWindow) {
            Log.error("No window found with bundle ID:", bundle_id, ", port:", port);
            dispatch_semaphore_signal(sem);
            return;
        }

        const auto filter = [[SCContentFilter alloc] initWithDesktopIndependentWindow:targetWindow];

        auto config = [[SCStreamConfiguration alloc] init];
        config.width = size.first;
        config.height = size.second;
        config.pixelFormat = kCVPixelFormatType_32BGRA;
        config.colorSpaceName = kCGColorSpaceSRGB;
        config.showsCursor = NO;

        const auto window_height = rect[3];
        const auto content_width = rect[6];
        const auto content_height = rect[7];

        const auto titlebar_height = window_height - content_height;
        if (titlebar_height > 0) {
            Log.trace("Titlebar logical height:", titlebar_height);
            config.sourceRect = CGRectMake(0, titlebar_height, content_width, content_height);
        }

        m_output = [[MacSCKOutput alloc] init];
        m_stream = [[SCStream alloc] initWithFilter:filter configuration:config delegate:m_output];

        [config release];
        [filter release];

        m_queue = dispatch_queue_create("plus.maa.MacSCKOutput", NULL);
        [m_stream addStreamOutput:m_output
                             type:SCStreamOutputTypeScreen
               sampleHandlerQueue:m_queue
                            error:&error];

        if (error) {
            Log.error("Cannot add stream output:", error.localizedDescription.UTF8String);
            dispatch_semaphore_signal(sem);
            return;
        }

        [m_stream startCaptureWithCompletionHandler:^(NSError* _Nullable error) {
            if (error) {
                Log.error("Cannot start capture:", error.localizedDescription.UTF8String);
                dispatch_semaphore_signal(sem);
                return;
            }

            Log.trace("Started capture for window:", targetWindow.title.UTF8String);
            m_output.running = YES;
            result = true;
            dispatch_semaphore_signal(sem);
        }];
    };

    [SCShareableContent getShareableContentExcludingDesktopWindows:YES
                                               onScreenWindowsOnly:YES
                                                 completionHandler:handler];

    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    dispatch_release(sem);
    return result;
}

struct BufferGuard {
    CVImageBufferRef buffer;
    bool locked = false;

    explicit BufferGuard(CVImageBufferRef buf) noexcept
        : buffer(buf)
    {
    }

    BufferGuard(const BufferGuard&) = delete;
    BufferGuard& operator=(const BufferGuard&) = delete;

    ~BufferGuard()
    {
        if (locked) {
            CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
        }
        if (buffer) {
            CFRelease(buffer);
        }
    }
};

bool asst::MacSCKHelper::Impl::capture(std::vector<uint8_t>& bgrData) const
{
    if (!m_queue || !m_output) {
        Log.error("Stream output is not initialized");
        return false;
    }

    [m_output waitForInitialized];

    __block CVImageBufferRef buffer = nullptr;
    dispatch_sync(m_queue, ^{
        if (!m_output.running) {
            Log.error("Stream is not running");
            return;
        }
        if (m_output.buffer) {
            CFRetain(m_output.buffer);
            buffer = m_output.buffer;
        }
    });

    if (!buffer) {
        Log.error("No image buffer available");
        return false;
    }

    BufferGuard guard { buffer };

    long ret = CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    if (ret != kCVReturnSuccess) [[unlikely]] {
        Log.error("Failed to lock pixel buffer:", ret);
        return false;
    }

    guard.locked = true;

    const auto width = CVPixelBufferGetWidth(buffer);
    const auto height = CVPixelBufferGetHeight(buffer);

    const auto dstRowBytes = width * 3;
    bgrData.resize(height * dstRowBytes);

    vImage_Buffer srcBuffer;
    srcBuffer.data = CVPixelBufferGetBaseAddress(buffer);
    srcBuffer.height = height;
    srcBuffer.width = width;
    srcBuffer.rowBytes = CVPixelBufferGetBytesPerRow(buffer);

    vImage_Buffer dstBuffer;
    dstBuffer.data = bgrData.data();
    dstBuffer.height = height;
    dstBuffer.width = width;
    dstBuffer.rowBytes = dstRowBytes;

    ret = vImageConvert_RGBA8888toRGB888(&srcBuffer, &dstBuffer, kvImageNoFlags);
    if (ret != kvImageNoError) [[unlikely]] {
        Log.error("Failed to convert buffer channels:", ret);
        return false;
    }

    return true;
}

asst::MacSCKHelper::MacSCKHelper()
    : pImpl(std::make_unique<Impl>())
{
}

asst::MacSCKHelper::~MacSCKHelper() = default;

bool asst::MacSCKHelper::init(std::string_view bundle_id, std::string_view port, std::pair<int, int> size, std::array<int16_t, 8> rect)
{
    LogTraceFunction;
    return pImpl->init(bundle_id, port, size, rect);
}

bool asst::MacSCKHelper::capture(std::vector<uint8_t>& bgrData) const
{
    LogTraceFunction;
    return pImpl->capture(bgrData);
}

#endif // ASST_WITH_MAC_SCK
