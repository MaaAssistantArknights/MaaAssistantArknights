

#if defined(_WIN32)

#include "WSAController.h"

#include <TlHelp32.h>
#if defined(ASST_DEBUG)
#pragma warning(disable : 5054)
#include <opencv2/opencv.hpp>
#endif

namespace asst
{
    static class VirtualMouse
    {
    public:
        ~VirtualMouse();
        bool inject();
        bool down();
        bool up();

    private:
        void restore();

        static constexpr size_t code_size = 32;
        static constexpr unsigned char func_code[1024] =
            "\x58\x55\x53\x56\x57\x52\x48\x81\xec\x80\x00\x00\x00\x48\x8d\xac\x24\x80\x00\x00\x00\x48\x89\x4d"
            "\xf4\x48\x89\x45\xf8\x83\x7d\xf4\x01\x75\x2c\x48\x8b\x45\xf8\x48\x83\x78\x60\x01\x75\x12\x48\x81"
            "\xc4\x80\x00\x00\x00\x5a\x5f\x5e\x5b\x5d\xb8\x01\x80\x00\x00\xc3\x48\x81\xc4\x80\x00\x00\x00\x5a"
            "\x5f\x5e\x5b\x5d\x31\xc0\xc3\x48\x8b\x45\xf8\x48\x8b\x40\x10\x48\x89\x45\xc8\x48\x8b\x45\xf8\x48"
            "\x83\xc0\x18\x48\x89\x45\xc0\x48\x8b\x45\xf8\x48\x8b\x00\x48\x89\x45\xb8\x48\x8b\x45\xf8\xff\x50"
            "\x08\x48\x8b\x55\xb8\x4c\x8b\x45\xc0\x48\x89\xc1\x48\x8b\x45\xc8\x41\xb9\x20\x00\x00\x00\x45\x31"
            "\xd2\x48\xc7\x44\x24\x20\x00\x00\x00\x00\xff\xd0\x48\x8b\x45\xf8\x48\x8b\x00\x8b\x4d\xf4\xff\xd0"
            "\x66\x89\x45\xf2\x48\x8b\x45\xf8\x48\x8b\x40\x10\x48\x89\x45\xe0\x48\x8b\x45\xf8\x48\x83\xc0\x38"
            "\x48\x89\x45\xd8\x48\x8b\x45\xf8\x48\x8b\x00\x48\x89\x45\xd0\x48\x8b\x45\xf8\xff\x50\x08\x48\x8b"
            "\x55\xd0\x4c\x8b\x45\xd8\x48\x89\xc1\x48\x8b\x45\xe0\x41\xb9\x20\x00\x00\x00\x45\x31\xd2\x48\xc7"
            "\x44\x24\x20\x00\x00\x00\x00\xff\xd0\x8b\x45\xf2\x48\x81\xc4\x80\x00\x00\x00\x5a\x5f\x5e\x5b\x5d"
            "\xc3";
        /*
void FunctionCodeCompiledFromBelow()
{
	__asm {
		pop rax
		push rbp
		push rbx
		push rsi
		push rdi
		push rdx
		sub rsp, 128
		lea rbp, 128[rsp]
	}
	RemoteParam* pRP;
	INT vKey;
	__asm {
		mov[vKey], rcx
		mov[pRP], rax
	}

	if (vKey == VK_LBUTTON) {
		if (pRP->lpFlag == 1ull) {
			__asm {
				add rsp, 128
				pop rdx
				pop rdi
				pop rsi
				pop rbx
				pop rbp
				mov eax, 8001h
				ret
			}
		}
		{
			__asm {
				add rsp, 128
				pop rdx
				pop rdi
				pop rsi
				pop rbx
				pop rbp
				xor eax, eax
				ret
			}
		}
	}
	((PFN_WRITEPROCESSMEMORY)pRP->lpWriteProcessMemory)(
		((PFN_GETCURRENTPROCESS)pRP->lpGetCurrentProcess)(),
		pRP->lpGetKeyState, pRP->szOldCode, code_size, NULL
		);
	SHORT get = ((PFN_GetKeyState)pRP->lpGetKeyState)(vKey);
	((PFN_WRITEPROCESSMEMORY)pRP->lpWriteProcessMemory)(
		((PFN_GETCURRENTPROCESS)pRP->lpGetCurrentProcess)(),
		pRP->lpGetKeyState, pRP->szNewCode, code_size, NULL
		);
	__asm {
		mov eax, [get]
		add rsp, 128
		pop rdx
		pop rdi
		pop rsi
		pop rbx
		pop rbp
		ret
	}
}
        */

        struct
        {
            LPVOID lpGetKeyState;
            LPVOID lpGetCurrentProcess;
            LPVOID lpWriteProcessMemory;
            unsigned char oldcode[code_size];
            unsigned char newcode[code_size];
            LPVOID func_addr;
            ULONGLONG flag;
        } m_remote;
        ULONGLONG m_flag_offset;
        unsigned char m_oldcode[code_size];
        unsigned char m_newcode[code_size] =
            "\x48\xb8\x00\x00\x00\x00\x00\x00\x00\x00\x50\x48\xb8\x00\x00\x00\x00\x00\x00\x00\x00\xff\xe0";

        typedef BOOL(__stdcall* PFN_WRITEPROCESSMEMORY)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
        typedef HANDLE(__stdcall* PFN_GETCURRENTPROCESS)(void);
        typedef SHORT(__stdcall* PFN_GetKeyState)(INT);

        HANDLE m_target_process = nullptr;
        HMODULE m_kernel32 = nullptr;
        HMODULE m_user32 = nullptr;

        LPVOID remote_func_addr = nullptr;
        LPVOID remote_para_addr = nullptr;
        LPVOID remote_flag_addr = nullptr;

        bool m_good = false;
    } s_vm;
}

asst::WSAController::WSAController(const AsstCallback& callback, Assistant* inst, PlatformType type)
    : InstHelper(inst), m_callback(callback), m_wgc(m_last_error), m_uuid("111111")
{
    LogTraceFunction;

    if (!winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported()) {
        json::value create_device_error = json::object {
            { "uuid", m_uuid },
            { "what", "NotSupported" },
            { "why", "Require higher windows version" },
            { "details",
              json::object {
                  { "Minium version supported: win10 1903", m_last_error },
              } },
        };
        this->callback(AsstMsg::InternalError, create_device_error);
        Log.error("platform not supported");
        throw std::runtime_error("platform not supported");
    }

    if (m_last_error != 0) {
        json::value create_device_error = json::object {
            { "uuid", m_uuid },
            { "what", "DeviceFailed" },
            { "why", "Cannot create D3D11 device." },
            { "details",
              json::object {
                  { "GetLastError() = ", m_last_error },
              } },
        };
        this->callback(AsstMsg::InternalError, create_device_error);
        Log.error("failed to createdevice");
        throw std::runtime_error("failed to createdevice");
    }

    m_platform_io = PlatformFactory::create_platform(inst, type);

    if (!m_platform_io) {
        Log.error("platform not supported");
        throw std::runtime_error("platform not supported");
    }

    m_inited = true;
}

asst::WSAController::~WSAController()
{
    LogTraceFunction;

    m_inited = false;
}

bool asst::WSAController::connect([[maybe_unused]] const std::string& adb_path,
                                  [[maybe_unused]] const std::string& address,
                                  [[maybe_unused]] const std::string& config)
{
    LogTraceFunction;

    bool result = m_wgc.prepare(m_resize_window, m_golden_border);

    if (!result) {
        json::value error = json::object { { "uuid", m_uuid },
                                           { "what", "ConnectionFailed" },
                                           { "detail", "cannot prepare for connection" } };
        callback(AsstMsg::ConnectionInfo, error);
        return false;
    }

    if (need_exit()) {
        return false;
    }

    if (!m_wgc.start_capture()) {

        json::value error = json::object { { "uuid", m_uuid },
                                           { "what", "ConnectionFailed" },
                                           { "detail", "capture failed" } };
        callback(AsstMsg::ConnectionInfo, error);
        return false;
    }

    m_touch.init(m_wgc.pub_wndwsa, m_wgc.pub_caption_height);

    m_screen_size = m_wgc.pub_size;

    return true;
}

void asst::WSAController::set_resize_window(bool enable) noexcept {
    m_resize_window = enable;
}

void asst::WSAController::set_golden_border(bool enable) noexcept {
    m_golden_border = enable;
}

void asst::WSAController::set_swipe_with_pause(bool enable) noexcept {
    m_use_swipe_with_pause = enable;
}

asst::WSAController::FrameBuffer::FrameBuffer(OUT DWORD& last_error)
     : m_frame_pool(nullptr), m_session(nullptr), m_item(nullptr)
 {
    LogTraceFunction;

    HRESULT result = S_OK;
    last_error = 0;

    result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, nullptr, 0,
                               D3D11_SDK_VERSION, m_native_device.put(), nullptr, m_context.put());
    if (FAILED(result)) {
        last_error = GetLastError();
        return;
    }
}

asst::WSAController::FrameBuffer::~FrameBuffer() {
    LogTraceFunction;

    end_capture();
    m_frame_pool = nullptr;
    m_item = nullptr;
    m_device = nullptr;
    m_staging_texture = nullptr;
    m_context = nullptr;
    m_native_device = nullptr;
}

bool asst::WSAController::FrameBuffer::prepare(bool resize_window, bool golden_border)
{
    LogTraceFunction;

    m_golden_border = golden_border;

    auto fill_error = [](std::string fn_name) {
        Log.error(std::format("prepare() FAILED  function name [{}], GetLastError() = {}", fn_name, GetLastError()));
    };

    pub_wndwsa = FindWindow(arknights_classname.c_str(), nullptr);
    if (pub_wndwsa == NULL) {
        fill_error("FindWindow");
        return false;
    }

    if (IsIconic(pub_wndwsa)) {
        Log.warn("WSA window is iconic. Try to show WSA window.");
        if (!ShowWindow(pub_wndwsa, SW_SHOW)) return false;
    }
    
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    auto wndStyle = GetWindowLong(pub_wndwsa, GWL_STYLE);
    auto wndExStyle = GetWindowLong(pub_wndwsa, GWL_EXSTYLE);
    auto dpi = GetDpiForWindow(pub_wndwsa);

    RECT wantedSize = { .left = 0, .top = 0, .right = WindowWidthDefault, .bottom = WindowHeightDefault };
    RECT wantedWindow = wantedSize, wantedClient = wantedSize;
    if (resize_window) {
        AdjustWindowRectExForDpi(&wantedWindow, wndStyle, false, wndExStyle, dpi);
        wantedWindow.right -= wantedWindow.left;
        wantedWindow.bottom -= wantedWindow.top;
        wantedWindow.top = wantedWindow.left = 0;
        SetWindowPos(pub_wndwsa, NULL, 0, 0, wantedWindow.right, wantedWindow.bottom, SWP_NOMOVE);

        GetClientRect(pub_wndwsa, &wantedClient);
        pub_caption_height = wantedClient.bottom - wantedClient.top - WindowHeightDefault;
    }
    else {
        GetClientRect(pub_wndwsa, &wantedClient);
        GetWindowRect(pub_wndwsa, &wantedWindow);
        wantedWindow.bottom -= wantedWindow.top;
        wantedWindow.right -= wantedWindow.left;
        wantedWindow.left = wantedWindow.top = 0;
        RECT testCaption = { 0, 0, 100, 100 };
        AdjustWindowRectExForDpi(&testCaption, wndStyle, false, wndExStyle, dpi);

        auto black = wantedWindow.right - wantedClient.right;
        pub_caption_height = testCaption.bottom - testCaption.top - 100 - black;
        wantedSize.right = wantedClient.right;
        wantedSize.bottom = wantedClient.bottom - pub_caption_height;
    }
    m_arknights_roi = cv::Rect { 0, pub_caption_height, wantedSize.right, wantedSize.bottom };

    HRESULT result = S_OK;
    auto factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem,
                                                 IGraphicsCaptureItemInterop>();
    result = factory->CreateForWindow(
        pub_wndwsa, winrt::guid_of<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>(), winrt::put_abi(m_item));
    if (FAILED(result)) {
        fill_error("IGraphicsCaptureItem::CreateForWindow");
        return false;
    }

    auto dxgiDevice = m_native_device.as<IDXGIDevice>();
    result = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(),
                                                  reinterpret_cast<IInspectable**>(winrt::put_abi(m_device)));
    if (FAILED(result)) {
        fill_error("CreateDirect3D11DeviceFromDXGIDevice");
        return false;
    }
    
    m_size = m_item.Size();
    pub_size.first = wantedSize.right;
    pub_size.second = wantedSize.bottom;
    m_pitch = m_size.Width * 3ull;

    m_cache.create(m_size.Height, m_size.Width, CV_8UC3);

    {
        D3D11_TEXTURE2D_DESC desc = { .Width = (UINT)m_size.Width,
                                      .Height = (UINT)m_size.Height,
                                      .MipLevels = 1,
                                      .ArraySize = 1,
                                      .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
                                      .SampleDesc = { .Count = 1, .Quality = 0 },
                                      .Usage = D3D11_USAGE_STAGING,
                                      .CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE,
                                      .MiscFlags = 0 };
        result = m_native_device->CreateTexture2D(&desc, nullptr, m_staging_texture.put());
    }
    if (FAILED(result)) {
        fill_error("ID3D11Device::CreateTexture2D");
        return false;
    }

    m_frame_pool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
        m_device, winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, 1, m_size);
    if (m_frame_pool == nullptr) {
        fill_error("winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded");
        return false;
    }

    return SUCCEEDED(result);
}

bool asst::WSAController::FrameBuffer::start_capture()
{
    LogTraceFunction;

    if (end_capture()) return false;

    if (pub_wndwsa != FindWindow(arknights_classname.c_str(), nullptr)) {
        m_frame_pool = nullptr;
        m_session = nullptr;
        return false;
    }

    m_frame_arrived = m_frame_pool.FrameArrived({ this, &FrameBuffer::process_frame });
    m_session = m_frame_pool.CreateCaptureSession(m_item);
    m_session.IsBorderRequired(m_golden_border);
    m_session.IsCursorCaptureEnabled(false);
    m_session.StartCapture();

    return true;
}

bool asst::WSAController::FrameBuffer::end_capture()
{
    LogTraceFunction;

    if (m_session) {
        m_locker.lock();
        m_session = nullptr;
        m_frame_pool.FrameArrived(m_frame_arrived);
        m_locker.unlock();
        return true;
    }

    return false;
}

bool asst::WSAController::FrameBuffer::restart_capture()
{
    LogTraceFunction;

    end_capture();

    m_frame_pool.Recreate(m_device, winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, 1,
                          m_size); // BGR
    pub_size = { m_size.Width, m_size.Height };

    start_capture();

    return true;
}

bool asst::WSAController::FrameBuffer::get(cv::Mat& payload)
{
    while (!m_recieved.load())
        ;

    m_locker.lock();
    if (m_cache.empty()) [[unlikely]] {
        m_locker.unlock();
        return false;
    }
    payload = m_cache(m_arknights_roi).clone();
    m_recieved.store(false);
    m_locker.unlock();

    return true;
}

bool asst::WSAController::FrameBuffer::peek(cv::Mat& payload)
{
    m_locker.lock();
    if (m_cache.empty()) [[unlikely]] {
        m_locker.unlock();
        return false;
    }
    payload = m_cache(m_arknights_roi).clone();
    m_locker.unlock();

    return true;
}

void asst::WSAController::FrameBuffer::process_frame(
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& framePool,
    [[maybe_unused]] winrt::Windows::Foundation::IInspectable const& object)
{
    m_locker.lock();
    if (!m_session) {
        m_locker.unlock();
        return;
    }

    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame frame = framePool.TryGetNextFrame();
    auto size = frame.ContentSize();

    if (size != m_size) {
        m_locker.unlock();
        frame = nullptr;
        m_size = size;
        restart_capture();
        return;
    }

    winrt::com_ptr<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess> dxgiAccess = {
        frame.Surface().as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>()
    };
    winrt::com_ptr<::ID3D11Resource> resource;
    winrt::check_hresult(dxgiAccess->GetInterface(__uuidof(resource), resource.put_void()));
    
    m_context->CopyResource(m_staging_texture.get(), resource.get());

    D3D11_MAPPED_SUBRESOURCE field;
    UINT subresource = D3D11CalcSubresource(0, 0, 0);
    m_context->Map(m_staging_texture.get(), subresource, D3D11_MAP_READ, 0, &field);

    uchar* data = m_cache.ptr();
    uchar* source = (uchar*)field.pData;
    for (size_t i = 0; i < m_size.Height; i++) {
        for (size_t j = 0; j < m_size.Width; j++) {
            data[i * m_pitch + j * 3] = source[i * field.RowPitch + j * 4];
            data[i * m_pitch + j * 3 + 1] = source[i * field.RowPitch + j * 4 + 1];
            data[i * m_pitch + j * 3 + 2] = source[i * field.RowPitch + j * 4 + 2];
        }
    }
    m_recieved.store(true);
    m_locker.unlock();
    m_context->Unmap(m_staging_texture.get(), subresource);
}

void asst::WSAController::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}

bool asst::WSAController::screencap(cv::Mat& image_payload, [[maybe_unused]] bool allow_reconnect)
{
    return m_wgc.get(image_payload);
}

bool asst::WSAController::start_game([[maybe_unused]] const std::string& client_type)
{
    return true; // 如果已经连接上了，那就不用打开游戏了，因为游戏已经打开了
}

#if defined(ASST_DEBUG)
static clock_t last_input = clock();
#endif

bool asst::WSAController::click(const Point& p)
{
#if defined(ASST_DEBUG)
    clock_t from_last = clock() - last_input;
    last_input = clock();
    Log.info(std::format("Clicked: [ {} , {} ]", p.x, p.y));
    cv::Mat img;
    m_wgc.peek(img);
    cv::circle(img, { p.x, p.y }, 25, { 0, 0, 255 }, 10);
    cv::putText(img, std::format("from_last: {}", from_last), { 0, 50 }, cv::FONT_HERSHEY_PLAIN, 2.,
                { 0, 0, 255 }, 3);
    cv::imshow("Input", img);
    cv::waitKey(1);
#endif
    m_touch.wait();
    return m_touch.click(p.x, p.y);
}

bool asst::WSAController::swipe(const Point& p1, const Point& p2, int duration, bool extra_swipe,
                                [[maybe_unused]] double slope_in, [[maybe_unused]] double slope_out,
                                bool with_pause)
{
#if defined(ASST_DEBUG)
    clock_t from_last = clock() - last_input;
    last_input = clock();
    Log.info(std::format("Swiped: [ {} , {} ] -> [ {} , {} ]", p1.x, p1.y, p2.x, p2.y),
             "Type=", extra_swipe ? "Precisely" : "Normal", ", WithPause=", with_pause);
    cv::Mat img;
    m_wgc.peek(img);
    cv::arrowedLine(img, { p1.x, p1.y }, { p2.x, p2.y }, { 0, 0, 255 }, 5);
    cv::putText(img,
                std::format("from_last: {}, dur: {}, type: {}, pause: {}", from_last, duration,
                            extra_swipe ? "Precisely" : "Normal", with_pause ? "true" : "false"),
                { 0, 50 }, cv::FONT_HERSHEY_PLAIN, 2., { 0, 0, 255 }, 3);
    cv::imshow("Input", img);
    cv::waitKey(1);
#endif
    m_touch.wait();
    with_pause = with_pause && m_use_swipe_with_pause;
    duration = (duration == 0 ? 400 : duration);
    return extra_swipe ? m_touch.swipe_precisely(p1.x, p1.y, p2.x, p2.y, duration, with_pause)
                       : m_touch.swipe(p1.x, p1.y, p2.x, p2.y, duration, with_pause);
}

bool asst::WSAController::press_esc()
{
#if defined(ASST_DEBUG)
    clock_t from_last = clock() - last_input;
    last_input = clock();
    Log.info("Pressed escape");
    cv::Mat img;
    m_wgc.peek(img);
    cv::putText(img, std::format("from_last: {}", from_last), { 0, 50 }, cv::FONT_HERSHEY_PLAIN,
                2., { 0, 0, 255 }, 3);
    cv::putText(img, "Pressed ESC!", { 300, 300 }, cv::FONT_HERSHEY_PLAIN,
                3., { 0, 0, 255 }, 5);
    cv::imshow("Input", img);
    cv::waitKey(1);
#endif
    m_touch.wait();
    m_touch.press(VK_ESCAPE);
    return true;
}

asst::WSAController::Toucher::~Toucher()
{
    m_running.store(false);
    if (m_thread.joinable()) m_thread.join();
}

bool asst::WSAController::Toucher::init(HWND hwnd, int caption_height)
{
    if (m_inited) return false;

    m_wnd = hwnd;
    m_caption_height = caption_height;

    m_running.store(true);
    m_thread = std::thread([this]() { this->run(); });

    std::mt19937_64 random_engine { std::random_device {}() };
    std::uniform_real_distribution<double> dist(-5, 5);
    for (int i = 0; i < 10; i++)
        m_rands.push_back(dist(random_engine));

    if (!s_vm.inject()) return false;
    m_inited = true;
    return true;
}

bool asst::WSAController::Toucher::click(int x, int y)
{
    if (!m_inited) return false;

    if (m_msgs.size() > max_queue_length) {
        return false;
    }

    m_msgs.push({ 0, x, y });

    return true;
}

bool asst::WSAController::Toucher::swipe(double sx, double sy, double ex, double ey, int dur, bool pause)
{
    if (!m_inited) return false;

    if (m_msgs.size() > max_queue_length) {
        return false;
    }

    constexpr int nslices = 32;
    auto dx = (ex - sx) / nslices, dy = (ey - sy) / nslices;
    auto dur_slice = dur / nslices;

    m_msgs.push({ 1, (int)sx, (int)sy });
    m_msgs.push({ 100, 0, 0 });
    UNREFERENCED_PARAMETER(pause);

    linear_interpolate(sx, sy, dx, dy, 0, dur_slice, nslices);

    m_msgs.push({ 2, (int)sx, (int)sy });

    return true;
}

bool asst::WSAController::Toucher::swipe_precisely(double sx, double sy, double ex, double ey, int dur, bool pause)
{
    if (!m_inited) return false;

    if (m_msgs.size() > max_queue_length) {
        return false;
    }

    constexpr int nslices = 16;
    constexpr int neslices = 32;
    constexpr int extra_slice_size = 3;
    constexpr double time_ratio = 3 / 8.;
    ex += extra_slice_size, ey += extra_slice_size;

    auto dx = (ex - sx) / nslices, dy = (ey - sy) / nslices;
    auto ux = std::abs(dx) / dx, uy = std::abs(dy) / dy;

    auto dur_slice = dur * time_ratio / nslices;

    m_msgs.push({ 1, (int)sx, (int)sy });
    m_msgs.push({ 100, 0, 0 });
    UNREFERENCED_PARAMETER(pause);

    auto time_stamp = linear_interpolate(sx, sy, dx, dy, 0, dur_slice, nslices);

    dx = -ux * extra_slice_size / neslices, dy = -uy * extra_slice_size / neslices;
    time_stamp = linear_interpolate(sx, sy, dx, dy, time_stamp, dur_slice, neslices);
    linear_interpolate(sx, sy, 0, 0, time_stamp, 0, 10);

    m_msgs.push({ 2, (int)sx, (int)sy });

    return true;
}

void asst::WSAController::Toucher::press(int key)
{
    m_msgs.push({ -1, key, 0 });
}

void asst::WSAController::Toucher::wait()
{
    while (!m_msgs.empty())
        ;
}

void asst::WSAController::Toucher::run()
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    using namespace std::chrono;
    time_point<high_resolution_clock, milliseconds> tic;
    time_point<high_resolution_clock, milliseconds> toc;

    while (m_running.load()) {
        if (m_msgs.empty()) {
            std::this_thread::yield();
            continue;
        }

        auto command = m_msgs.front();
        LRESULT res = 0;
        command.y += m_caption_height;
        LPARAM coord = MAKELPARAM(command.x, command.y);
        
        switch (command.type) {
        case -1:
            res = SendMessage(m_wnd, WM_KEYDOWN, command.x, 0);
            Sleep(80);
            res = SendMessage(m_wnd, WM_KEYUP, command.x, 0);
            Sleep(80);
            break;
        case 0:
            s_vm.down();
            res = SendMessage(m_wnd, WM_MOUSEHOVER, 0, coord);
            res = SendMessage(m_wnd, WM_MOUSEHOVER, MK_LBUTTON, coord);
            res = SendMessage(m_wnd, WM_KEYDOWN, VK_LBUTTON, coord);
            res = SendMessage(m_wnd, WM_MOUSEHOVER, MK_LBUTTON, coord);
            res = SendMessage(m_wnd, WM_LBUTTONDOWN, 0, coord);
            res = SendMessage(m_wnd, WM_MOUSEHOVER, MK_LBUTTON, coord);
            Sleep(80);
            s_vm.up();
            res = SendMessage(m_wnd, WM_MOUSEHOVER, 0, coord);
            res = SendMessage(m_wnd, WM_LBUTTONUP, 0, coord);
            res = SendMessage(m_wnd, WM_MOUSEHOVER, 0, coord);
            res = SendMessage(m_wnd, WM_KEYUP, VK_LBUTTON, coord);
            res = SendMessage(m_wnd, WM_MOUSEHOVER, 0, coord);
            res = SendMessage(m_wnd, WM_MOUSELEAVE, 0, 0);
            Sleep(80);
            break;
        case 100:
            tic = time_point_cast<milliseconds>(high_resolution_clock::now());
            res = TRUE;
            break;
        case 200:
            toc = time_point_cast<milliseconds>(high_resolution_clock::now());
            while (toc - tic <= milliseconds { command.x }) {
                toc = time_point_cast<milliseconds>(high_resolution_clock::now());
            }
            res = TRUE;
            break;
        case 1:
            s_vm.down();
            res = SendMessage(m_wnd, WM_MOUSEHOVER, 0, coord);
            res = SendMessage(m_wnd, WM_MOUSEHOVER, MK_LBUTTON, coord);
            res = SendMessage(m_wnd, WM_KEYDOWN, VK_LBUTTON, coord);
            res = SendMessage(m_wnd, WM_MOUSEHOVER, MK_LBUTTON, coord);
            res = SendMessage(m_wnd, WM_LBUTTONDOWN, 0, coord);
            res = SendMessage(m_wnd, WM_MOUSEHOVER, MK_LBUTTON, coord);
            break;
        case 2:
            s_vm.up();
            res = SendMessage(m_wnd, WM_MOUSEHOVER, 0, coord);
            res = SendMessage(m_wnd, WM_LBUTTONUP, 0, coord);
            res = SendMessage(m_wnd, WM_MOUSEHOVER, 0, coord);
            res = SendMessage(m_wnd, WM_KEYUP, VK_LBUTTON, coord);
            res = SendMessage(m_wnd, WM_MOUSEHOVER, 0, coord);
            res = SendMessage(m_wnd, WM_MOUSELEAVE, 0, 0);
            Sleep(80);
            break;
        case 3:
            res = SendMessage(m_wnd, WM_MOUSEHOVER, MK_LBUTTON, coord);
            res = SendMessage(m_wnd, WM_MOUSEMOVE, 0, coord);
            break;
        case 4:
            res = SendMessage(m_wnd, WM_MOUSEHOVER, MK_LBUTTON, coord);
            res = SendMessage(m_wnd, WM_MOUSEMOVE, 0, coord);
            res = SendMessage(m_wnd, WM_KEYDOWN, VK_ESCAPE, 0);
            res = SendMessage(m_wnd, WM_KEYUP, VK_ESCAPE, 0);
            break;
        }

        m_msgs.pop();
    }
}

double asst::WSAController::Toucher::linear_interpolate(double& sx, double& sy, double dx, double dy, double time_stamp,
                                                      double dur_slice, double n)
{
    for (int i = 1; i <= n; i++) {
        sx += dx, sy += dy;
        m_msgs.push({ 200, (int)time_stamp, 0 });
        m_msgs.push({ 3, (int)sx, (int)sy });
        time_stamp += dur_slice;
    }
    return time_stamp;
}

asst::VirtualMouse::~VirtualMouse() 
{
    if (m_good) {
        SIZE_T wrotten_bytes = 0;
        if (!WriteProcessMemory(m_target_process, (LPVOID)m_remote.lpGetKeyState, (LPVOID)m_remote.oldcode, code_size,
                                &wrotten_bytes)) {
            Log.error("Cannot write the old codes. GetLastError() = ", GetLastError());
        }
        if (wrotten_bytes != code_size) {
            Log.error("Wrong code size! GetLastError() = ", GetLastError());
        }
        restore();
    }
}

bool asst::VirtualMouse::inject()
{
    if (m_good) return true;

    {
        HANDLE token;
        LUID se_debugname;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
            return false;
        }
        if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &se_debugname)) {
            CloseHandle(token);
            return false;
        }
        TOKEN_PRIVILEGES tkp;
        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Luid = se_debugname;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;                     // 特权启用
        if (!AdjustTokenPrivileges(token, FALSE, &tkp, sizeof(tkp), NULL, NULL)) // 启用指定访问令牌的特权
        {
            CloseHandle(token);
            return false;
        }
    }

    DWORD process_id = 0;
    {
        HANDLE snap_shot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);
        if (!Process32First(snap_shot, &pe)) {
            Log.error("The frist entry of the process list has not been copyied to the buffer");
            return false;
        }
        while (Process32Next(snap_shot, &pe)) // 循环查找下一个进程
        {
            if (!lstrcmp(L"WsaClient.exe", pe.szExeFile)) // 找到
                process_id = pe.th32ProcessID;
        }
    }

    if (process_id == 0) {
        Log.error("Cannot find wsa process id.");
        return false;
    }

    m_target_process = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, process_id);
    if (m_target_process == NULL) {
        Log.error("OpenProcess Error! GetLastError() = ", GetLastError());
        return false;
    }

    size_t func_size = 1024ull;

    remote_func_addr =
        VirtualAllocEx(m_target_process, NULL, func_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (remote_func_addr == NULL) {
        Log.error("Failed to virtual allock!");
        restore();
        return false;
    }

    remote_para_addr =
        VirtualAllocEx(m_target_process, NULL, sizeof(m_remote), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (remote_para_addr == NULL) {
        Log.error("Failed to virtual allock!");
        restore();
        return false;
    }

    Log.info("function addr:", remote_func_addr, " parameter addr : ", remote_para_addr);
    m_kernel32 = LoadLibrary(L"kernel32.dll");
    if (m_kernel32 == 0) {
        Log.error("Failed to load kernel32.dll");
        restore();
        return false;
    }
    m_user32 = LoadLibrary(L"user32.dll");
    if (m_user32 == 0) {
        Log.error("Failed to load user32.dll");
        restore();
        return false;
    }

    m_remote.lpGetCurrentProcess = (LPVOID)GetProcAddress(m_kernel32, "GetCurrentProcess");
    m_remote.lpWriteProcessMemory = (LPVOID)GetProcAddress(m_kernel32, "WriteProcessMemory");
    m_remote.lpGetKeyState = (LPVOID)GetProcAddress(m_user32, "GetKeyState");

    *(ULONGLONG*)&(m_newcode[2]) = ((ULONGLONG)remote_para_addr);
    *(ULONGLONG*)&(m_newcode[13]) = ((ULONGLONG)remote_func_addr);

    memcpy(m_oldcode, m_remote.lpGetKeyState, code_size);
    {
        std::ostringstream str_codes;
        str_codes << std::hex;
        for (auto i : m_oldcode) {
            str_codes << "\\x" << (int)i;
        }
        Log.info("Old codes: ", str_codes.str());
    }
    memcpy((char*)m_remote.oldcode, (char*)m_oldcode, code_size);
    memcpy((char*)m_remote.newcode, (char*)m_newcode, code_size);
    m_remote.func_addr = remote_func_addr;
    m_remote.flag = 0;

    SIZE_T wrotten_bytes = 0;
    DWORD pretection;

    VirtualProtectEx(m_target_process, (LPVOID)remote_func_addr, code_size, PAGE_READWRITE, &pretection);
    if (!WriteProcessMemory(m_target_process, (LPVOID)remote_func_addr, (LPVOID)&func_code, func_size,
                            &wrotten_bytes)) {
        Log.error("Failed to write memory, category: 1! GetLastError() = ", GetLastError());
        restore();
        return false;
    }
    VirtualProtectEx(m_target_process, (LPVOID)remote_func_addr, code_size, PAGE_EXECUTE, &pretection);

    if (!WriteProcessMemory(m_target_process, (LPVOID)remote_para_addr, (LPVOID)&m_remote, sizeof(m_remote),
                            &wrotten_bytes)) {
        Log.error("Failed to write memory, category: 2! GetLastError() = ", GetLastError());
        restore();
        return false;
    }

    VirtualProtectEx(m_target_process, (LPVOID)m_remote.lpGetKeyState, code_size, PAGE_READWRITE, &pretection);
    if (!WriteProcessMemory(m_target_process, (LPVOID)m_remote.lpGetKeyState, (LPVOID)m_newcode, code_size,
                            &wrotten_bytes)) {
        Log.error("Failed to write memory, category: 3! GetLastError() = ", GetLastError());
        if (!WriteProcessMemory(m_target_process, (LPVOID)m_remote.lpGetKeyState, (LPVOID)m_remote.oldcode, code_size,
                                &wrotten_bytes)) {
            Log.error("Cannot write the old codes. GetLastError() = ", GetLastError());
        }
        if (wrotten_bytes != code_size) {
            Log.error("Wrong code size! GetLastError() = ", GetLastError());
        }
        restore();
        return false;
    }
    VirtualProtectEx(m_target_process, (LPVOID)m_remote.lpGetKeyState, code_size, PAGE_EXECUTE, &pretection);

    Log.info("injected!");
    
    m_flag_offset = (ULONGLONG)&m_remote.flag - (ULONGLONG)&m_remote;
    remote_flag_addr = (LPVOID)((ULONGLONG)remote_para_addr + m_flag_offset);

    m_good = true;
    return true;
}

bool asst::VirtualMouse::down()
{
    if (!m_good) return false;
    m_remote.flag = 1;
    SIZE_T wrotten_bytes = 0;
    if (!WriteProcessMemory(m_target_process, (LPVOID)remote_flag_addr, (LPVOID)&m_remote.flag, sizeof(ULONGLONG),
                            &wrotten_bytes)) {
        Log.error("Failed to write memory, category: 4! GetLastError() = ", GetLastError());
        restore();
        m_good = false;
        return false;
    }
    if (wrotten_bytes != sizeof(ULONGLONG)) {
        Log.error("Wrong code size! GetLastError() = ", GetLastError());
    }
    return true;
}

bool asst::VirtualMouse::up()
{
    if (!m_good) return false;
    m_remote.flag = 0;
    SIZE_T wrotten_bytes = 0;
    if (!WriteProcessMemory(m_target_process, (LPVOID)remote_flag_addr, (LPVOID)&m_remote.flag, sizeof(ULONGLONG),
                            &wrotten_bytes)) {
        Log.error("Failed to write memory, category: 4! GetLastError() = ", GetLastError());
        restore();
        m_good = false;
        return false;
    }
    if (wrotten_bytes != sizeof(ULONGLONG)) {
        Log.error("Wrong code size! GetLastError() = ", GetLastError());
    }
    return true;
}

void asst::VirtualMouse::restore()
{
    if (remote_func_addr) {
        VirtualFree(remote_func_addr, 0, MEM_RELEASE);
        remote_func_addr = nullptr;
    }
    if (remote_para_addr) {
        VirtualFree(remote_para_addr, 0, MEM_RELEASE);
        remote_para_addr = nullptr;
    }
    if (m_target_process) {
        CloseHandle(m_target_process);
        m_target_process = NULL;
    }
    if (m_kernel32) {
        FreeLibrary(m_kernel32);
        m_kernel32 = NULL;
    }
    if (m_user32) {
        FreeLibrary(m_user32);
        m_user32 = NULL;
    }
    m_good = false;
}

#endif // _WIN32
