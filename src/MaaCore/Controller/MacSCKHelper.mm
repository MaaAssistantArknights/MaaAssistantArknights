#include "MacSCKHelper.h"

#ifdef ASST_WITH_MAC_SCK

#import <Accelerate/Accelerate.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include "Utils/Logger.hpp"

#include <mutex>

@interface MacSCKOutput : NSObject <SCStreamDelegate, SCStreamOutput>

@property (nonatomic, readonly) CVImageBufferRef buffer;

@property (atomic, assign) BOOL running;

- (void)waitForInitialized;
- (void)cancelWait;

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

- (void)cancelWait
{
    self.running = NO;
    _initialized.store(true, std::memory_order_release);
    _initialized.notify_all();
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

struct asst::MacSCKHelper::Impl : std::enable_shared_from_this<Impl> {
    ~Impl();

    SCStream* m_stream = nil;
    MacSCKOutput* m_output = nil;
    std::mutex m_output_mutex;
    std::mutex m_init_mutex;
    dispatch_semaphore_t m_init_semaphore = nil;
    std::atomic_bool m_init_cancelled = false;

    dispatch_queue_t m_queue = nil;

    void signal_init() noexcept;
    bool init(std::string_view bundle_id, std::string_view port, std::pair<int, int> size, std::array<int16_t, 8> rect);
    bool capture(std::vector<uint8_t>& bgrData) const;
    void cancel_capture() noexcept;
};

void asst::MacSCKHelper::Impl::signal_init() noexcept
{
    std::lock_guard lock(m_init_mutex);
    if (m_init_semaphore) {
        dispatch_semaphore_signal(m_init_semaphore);
    }
}

asst::MacSCKHelper::Impl::~Impl()
{
    const auto stream = m_stream;
    const auto queue = m_queue;
    MacSCKOutput* output = nil;
    {
        std::lock_guard lock(m_output_mutex);
        output = m_output;
        m_output = nil;
    }

    m_stream = nil;
    m_queue = nil;
    if (m_init_semaphore) {
        dispatch_release(m_init_semaphore);
        m_init_semaphore = nil;
    }

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
    {
        std::lock_guard lock(m_init_mutex);
        m_init_semaphore = sem;
    }
    const auto self = shared_from_this();
    __block bool result = false;

    if (m_init_cancelled.load(std::memory_order_acquire)) {
        signal_init();
        return false;
    }

    const auto handler = ^(SCShareableContent* _Nullable content, NSError* _Nullable error) {
        if (self->m_init_cancelled.load(std::memory_order_acquire)) {
            self->signal_init();
            return;
        }
        if (error) {
            Log.error("Cannot get shareable content:", error.localizedDescription.UTF8String);
            self->signal_init();
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
            self->signal_init();
            return;
        }

        const auto filter = [[SCContentFilter alloc] initWithDesktopIndependentWindow:targetWindow];

        auto config = [[SCStreamConfiguration alloc] init];
        config.width = size.first;
        config.height = size.second;
        config.scalesToFit = YES;
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

        {
            std::lock_guard lock(self->m_output_mutex);
            self->m_output = [[MacSCKOutput alloc] init];
        }
        self->m_stream = [[SCStream alloc] initWithFilter:filter configuration:config delegate:self->m_output];

        [config release];
        [filter release];

        self->m_queue = dispatch_queue_create("plus.maa.MacSCKOutput", NULL);
        [self->m_stream addStreamOutput:self->m_output
                                   type:SCStreamOutputTypeScreen
                     sampleHandlerQueue:self->m_queue
                                  error:&error];

        if (error) {
            Log.error("Cannot add stream output:", error.localizedDescription.UTF8String);
            self->signal_init();
            return;
        }

        [self->m_stream startCaptureWithCompletionHandler:^(NSError* _Nullable error) {
            if (self->m_init_cancelled.load(std::memory_order_acquire)) {
                self->signal_init();
                return;
            }
            if (error) {
                Log.error("Cannot start capture:", error.localizedDescription.UTF8String);
                self->signal_init();
                return;
            }

            Log.trace("Started capture for window:", targetWindow.title.UTF8String);
            self->m_output.running = YES;
            result = true;
            self->signal_init();
        }];
    };

    [SCShareableContent getShareableContentExcludingDesktopWindows:YES
                                               onScreenWindowsOnly:YES
                                                 completionHandler:handler];

    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    return result && !m_init_cancelled.load(std::memory_order_acquire);
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

void asst::MacSCKHelper::Impl::cancel_capture() noexcept
{
    m_init_cancelled.store(true, std::memory_order_release);
    signal_init();

    MacSCKOutput* output = nil;
    {
        std::lock_guard lock(m_output_mutex);
        output = [m_output retain];
    }
    if (output) {
        [output cancelWait];
        [output release];
    }
}

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
    : pImpl(std::make_shared<Impl>())
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

void asst::MacSCKHelper::cancel_capture() noexcept
{
    if (pImpl) {
        pImpl->cancel_capture();
    }
}

void asst::MacSCKHelper::reset()
{
    pImpl = std::make_shared<Impl>();
}

#endif // ASST_WITH_MAC_SCK
