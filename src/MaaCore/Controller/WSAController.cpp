
#include "WSAController.h"

#if defined(_WIN32)

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
    if (m_staging_texture) m_staging_texture->Release();
    if (m_context) m_context->Release();
    if (m_native_device) m_native_device->Release();
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
        Log.trace("WSA window is iconic. Try to show WSA window.");
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

        pub_caption_height = testCaption.bottom - testCaption.top - 100;
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

    m_front.create(m_size.Height, m_size.Width, CV_8UC3);
    m_back.create(m_size.Height, m_size.Width, CV_8UC3);

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

bool asst::WSAController::FrameBuffer::get_once(cv::Mat& payload)
{
    m_locker.lock();
    if (m_front.empty()) {
        m_locker.unlock();
        m_need_update.store(true);
        return false;
    }
    payload = m_front(m_arknights_roi).clone();
    m_locker.unlock();
    m_need_update.store(true);
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

    uchar* data = m_back.ptr();
    uchar* source = (uchar*)field.pData;
    for (size_t i = 0; i < m_size.Height; i++) {
        for (size_t j = 0; j < m_size.Width; j++) {
            data[i * m_pitch + j * 3] = source[i * field.RowPitch + j * 4];
            data[i * m_pitch + j * 3 + 1] = source[i * field.RowPitch + j * 4 + 1];
            data[i * m_pitch + j * 3 + 2] = source[i * field.RowPitch + j * 4 + 2];
        }
    }
    if (m_need_update.load()) {
        std::swap(m_front, m_back);
        m_need_update.store(false);
    }
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
    return m_wgc.get_once(image_payload);
}

bool asst::WSAController::start_game([[maybe_unused]] const std::string& client_type)
{
    return true; // 如果已经连接上了，那就不用打开游戏了，因为游戏已经打开了
}

bool asst::WSAController::click(const Point& p)
{
    return  m_touch.click(p.x, p.y);
}

bool asst::WSAController::swipe(const Point& p1, const Point& p2, int duration, bool extra_swipe,
                                [[maybe_unused]] double slope_in, [[maybe_unused]] double slope_out,
                                bool with_pause)
{
    if (extra_swipe) {
        return m_touch.swipe_precisely(p1.x, p1.y, p2.x, p2.y, duration, with_pause);
    }
    else {
        return m_touch.swipe(p1.x, p1.y, p2.x, p2.y, duration);
    }
}

bool asst::WSAController::press_esc()
{
    m_touch.press(VK_ESCAPE);
    return true;
}

asst::WSAController::Toucher::~Toucher()
{
    m_running.store(false);
    if (m_thread.joinable()) m_thread.join();
}

void asst::WSAController::Toucher::init(HWND hwnd, int caption_height)
{
    if (m_inited) return;

    m_wnd = hwnd;
    m_caption_height = caption_height;

    m_running.store(true);
    m_thread = std::thread([this]() { this->run(); });

    std::mt19937_64 random_engine { std::random_device {}() };
    std::uniform_real_distribution<double> dist(-5, 5);
    for (int i = 0; i < 10; i++)
        random_end.push_back(dist(random_engine));

    m_inited = true;
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

bool asst::WSAController::Toucher::swipe(double sx, double sy, double ex, double ey, int dur)
{
    if (!m_inited) return false;

    if (dur < 40) {
        return false;
    }

    if (m_msgs.size() > max_queue_length) {
        return false;
    }

    constexpr int nslices = 6;
    auto dx = (ex - sx) / nslices, dy = (ey - sy) / nslices;
    auto dur_slice = dur / nslices;

    m_msgs.push({ 100, 0, 0 });
    m_msgs.push({ 1, (int)sx, (int)sy });
    linear_interpolate(sx, sy, dx, dy, 0, dur_slice, nslices);

    m_msgs.push({ 2, (int)sx, (int)sy });

    return true;
}

bool asst::WSAController::Toucher::swipe_precisely(double sx, double sy, double ex, double ey, int dur, bool pause)
{
    if (!m_inited) return false;

    if (dur < 40) {
        return false;
    }

    if (m_msgs.size() > max_queue_length) {
        return false;
    }

    constexpr int nslices = 6;
    constexpr int neslices = 6;
    constexpr int extra_slice_size = 3;
    ex += extra_slice_size, ey += extra_slice_size;
    constexpr double slide_time_ratio = 3. / 5.;

    auto dx = (ex - sx) / nslices, dy = (ey - sy) / nslices;
    auto ux = std::abs(dx) / dx, uy = std::abs(dy) / dy;

    auto dur_slice = dur * slide_time_ratio / nslices;

    m_msgs.push({ 100, 0, 0 });
    m_msgs.push({ 1, (int)sx, (int)sy });
    if (pause) press(VK_ESCAPE);
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
            Sleep(50);
            continue;
        }

        auto command = m_msgs.front();
        LRESULT res = 0;
        command.y += m_caption_height;
        LPARAM coord = MAKELPARAM(command.x, command.y);

        switch (command.type) {
        case -1:
            res = SendMessage(m_wnd, WM_KEYDOWN, command.x, 0);
            Sleep(20);
            res = SendMessage(m_wnd, WM_KEYUP, command.x, 0);
            Sleep(20);
            break;
        case 0:
            res = SendMessage(m_wnd, WM_LBUTTONDOWN, 0, coord);
            Sleep(20);
            res &= SendMessage(m_wnd, WM_LBUTTONUP, 0, coord);
            Sleep(20);
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
            res = SendMessage(m_wnd, WM_LBUTTONDOWN, 0, coord);
            break;
        case 2:
            res = SendMessage(m_wnd, WM_LBUTTONUP, 0, coord);
            break;
        case 3:
            res = SendMessage(m_wnd, WM_MOUSEMOVE, 0, coord);
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

#endif // _WIN32
