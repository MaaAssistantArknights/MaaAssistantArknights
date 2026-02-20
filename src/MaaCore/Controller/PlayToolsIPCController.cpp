#include "PlayToolsIPCController.h"

#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <chrono>
#include <meojson/json.hpp>
#include <numeric>
#include <ranges>
#include <sstream>
#include <cstring>
#include <time.h>
#include <cerrno>

#ifdef __APPLE__

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>

#endif // __APPLE__

#include "Config/GeneralConfig.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "SwipeHelper.hpp"

using namespace std::chrono;

namespace asst
{

PlayToolsIPCController::PlayToolsIPCController(
    const AsstCallback& callback,
    Assistant* inst,
    PlatformType type [[maybe_unused]]) :
    InstHelper(inst),
    m_callback(callback)
{
    LogTraceFunction;
}

PlayToolsIPCController::~PlayToolsIPCController()
{
#ifdef __APPLE__
    close();
#endif
}

#ifdef __APPLE__

bool PlayToolsIPCController::connect(
    const std::string& adb_path [[maybe_unused]],
    const std::string& address,
    const std::string& config [[maybe_unused]])
{
    LogInfo << "Connecting to PlayTools via IPC:" << address;
    
    // 如果地址不变且已连接，直接返回（避免重复连接）
    if (m_ipc_address == address && inited() && is_socket_alive()) {
        LogInfo << "Already connected to the same address, reusing connection";
        return true;
    }
    
    // 如果地址改变，关闭旧连接
    if (m_ipc_address != address) {
        close();
        m_ipc_address = address;
    }

    return open();
}

bool PlayToolsIPCController::inited() const noexcept
{
    return m_ipc_shm_ptr != nullptr && m_ipc_header != nullptr && m_screen_size.first > 0;
}

const std::string& asst::PlayToolsIPCController::get_uuid() const
{
    const static std::string uuid("com.hypergryph.arknights.ipc");
    return uuid;
}

size_t PlayToolsIPCController::get_pipe_data_size() const noexcept
{
    return 0;
}

size_t PlayToolsIPCController::get_version() const noexcept
{
    return MinimalVersion;
}

bool PlayToolsIPCController::screencap(cv::Mat& image_payload, bool allow_reconnect)
{
    using namespace std::chrono;
    LogTraceFunction;

    auto start_time = high_resolution_clock::now();

    // 检查连接状态，必要时自动重连
    if (!inited() || !is_socket_alive()) {
        if (allow_reconnect) {
            LogInfo << "IPC connection lost before screencap, reconnecting...";
            if (!reconnect()) {
                LogError << "Reconnect failed";
                auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
                recordScreencapCost(duration.count(), false);
                return false;
            }
        } else {
            LogError << "IPC not initialized or connection lost";
            auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
            recordScreencapCost(duration.count(), false);
            return false;
        }
    }
    
    // 发送截图命令
    uint32_t seq_id = send_command(IPCCommandType::SCREENSHOT);
    
    // 等待事件响应；超时时若允许重连则重试一次
    if (!wait_event(seq_id, 5000)) {
        if (allow_reconnect) {
            LogWarn << "Screenshot timeout, attempting reconnect and retry...";
            if (!reconnect()) {
                LogError << "Reconnect failed after screenshot timeout";
                auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
                recordScreencapCost(duration.count(), false);
                return false;
            }
            seq_id = send_command(IPCCommandType::SCREENSHOT);
            if (!wait_event(seq_id, 5000)) {
                LogError << "Screenshot still timeout after reconnect";
                auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
                recordScreencapCost(duration.count(), false);
                return false;
            }
        } else {
            LogError << "Screenshot timeout";
            auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
            recordScreencapCost(duration.count(), false);
            return false;
        }
    }
    
    // 从截图共享内存读取数据
    if (!m_screencap_shm_ptr || m_screencap_shm_size == 0) {
        LogError << "Screenshot shared memory not initialized";
        auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
        recordScreencapCost(duration.count(), false);
        return false;
    }
    
    // 直接包装共享内存中的像素数据为 cv::Mat（零拷贝）
    int img_type = CV_8UC4; // BGRA 格式
    cv::Mat shared_image(m_screen_size.second, m_screen_size.first, img_type, 
                        m_screencap_shm_ptr);
    
    // 转换 BGRA -> BGR（OpenCV 默认格式）
    cv::cvtColor(shared_image, image_payload, cv::COLOR_BGRA2BGR);
    
    auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
    recordScreencapCost(duration.count(), true);
    
    return true;
}

bool PlayToolsIPCController::start_game(const std::string& client_type [[maybe_unused]])
{
    Log.info("Start game is not supported in IPC mode");
    return true;
}

bool PlayToolsIPCController::stop_game(const std::string& client_type [[maybe_unused]])
{
    LogInfo << "Sending TERMINATE command via IPC";
    if (!inited()) {
        LogWarn << "IPC not initialized, cannot send terminate";
        return false;
    }
    uint32_t seq_id = send_command(IPCCommandType::TERMINATE);
    return wait_event(seq_id, 5000);
}

bool PlayToolsIPCController::click(const Point& p)
{
    LogTrace << "PlayToolsIPC click:" << p;
    return toucher_down(p) && toucher_up(p);
}

bool PlayToolsIPCController::input([[maybe_unused]] const std::string& text)
{
    Log.info("InputText is not supported on iOS");
    return true;
}

bool PlayToolsIPCController::swipe(
    const Point& p1,
    const Point& p2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool with_pause [[maybe_unused]])
{
    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    const auto width = m_screen_size.first;
    const auto height = m_screen_size.second;

    if (x1 < 0 || x1 >= width || y1 < 0 || y1 >= height) {
        Log.warn("swipe point1 is out of range", x1, y1);
        x1 = std::clamp(x1, 0, width - 1);
        y1 = std::clamp(y1, 0, height - 1);
    }

    LogTrace << "PlayToolsIPC swipe" << p1 << p2 << duration << extra_swipe << slope_in << slope_out;

    toucher_down(p1);

    auto bounds_check = [width, height](int x, int y) {
        return x >= 0 && x <= width && y >= 0 && y <= height;
    };

    auto move_func = [this](int x, int y) {
        toucher_move({ x, y });
        return true;
    };

    auto progressive_move = [&](int _x1, int _y1, int _x2, int _y2, int _duration) {
        interpolate_swipe(
            _x1, _y1, _x2, _y2,
            _duration,
            DefaultSwipeDelay,
            slope_in,
            slope_out,
            move_func,
            bounds_check);
    };

    const auto& opt = Config.get_options();
    progressive_move(x1, y1, x2, y2, duration ? duration : opt.minitouch_swipe_default_duration);

    if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
        toucher_wait(opt.minitouch_swipe_extra_end_delay);
        progressive_move(x2, y2, x2, y2 - opt.minitouch_extra_swipe_dist,
                         opt.minitouch_extra_swipe_duration);
    }

    return toucher_up(p2);
}

bool PlayToolsIPCController::press_esc()
{
    Log.info("ESC is not supported on iOS");
    return false;
}

std::pair<int, int> PlayToolsIPCController::get_screen_res() const noexcept
{
    return m_screen_size;
}

void PlayToolsIPCController::back_to_home() noexcept
{
    Log.info("HOME is not supported on iOS");
}

void PlayToolsIPCController::toucher_wait(const int delay)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
}

bool PlayToolsIPCController::toucher_down(const Point& p, const int delay)
{
    return toucher_commit(IPCCommandType::TOUCH_BEGAN, p, delay);
}

bool PlayToolsIPCController::toucher_move(const Point& p, const int delay)
{
    return toucher_commit(IPCCommandType::TOUCH_MOVED, p, delay);
}

bool PlayToolsIPCController::toucher_up(const Point& p, const int delay)
{
    return toucher_commit(IPCCommandType::TOUCH_ENDED, p, delay);
}

bool PlayToolsIPCController::toucher_commit(IPCCommandType phase, const Point& p, const int delay)
{
    // 若未初始化或 socket 已断开，尝试重连
    if (!inited() || !is_socket_alive()) {
        LogInfo << "IPC connection not ready for touch, reconnecting...";
        if (!reconnect()) {
            LogWarn << "Reconnect failed, cannot send touch event";
            return false;
        }
    }
    uint32_t seq_id = send_command(phase, p.x, p.y);
    bool ret = wait_event(seq_id, 3000);
    if (!ret) {
        // 命令超时，尝试重连后重试一次
        LogWarn << "Touch event timeout, attempting reconnect and retry...";
        if (reconnect()) {
            seq_id = send_command(phase, p.x, p.y);
            ret = wait_event(seq_id, 3000);
        }
    }
    toucher_wait(delay);
    return ret;
}

void PlayToolsIPCController::close()
{
    m_screen_size = { 0, 0 };
    m_seq_id = 0;
    
    m_screencap_cost.clear();
    m_screencap_times = 0;
    m_screencap_cost_reported = false;

    if (m_unix_socket >= 0) {
        ::close(m_unix_socket);
        m_unix_socket = -1;
    }
    
    if (m_cmd_sem) {
        sem_close(m_cmd_sem);
        m_cmd_sem = nullptr;
    }
    
    if (m_event_sem) {
        sem_close(m_event_sem);
        m_event_sem = nullptr;
    }
    
    if (m_ipc_shm_ptr && m_ipc_shm_ptr != MAP_FAILED) {
        munmap(m_ipc_shm_ptr, m_ipc_shm_size);
        m_ipc_shm_ptr = nullptr;
    }
    
    if (m_ipc_shm_fd >= 0) {
        ::close(m_ipc_shm_fd);
        if (!m_ipc_shm_name.empty()) {
            shm_unlink(m_ipc_shm_name.c_str());
            m_ipc_shm_name.clear();
        }
        m_ipc_shm_fd = -1;
    }
    
    if (m_screencap_shm_ptr && m_screencap_shm_ptr != MAP_FAILED) {
        munmap(m_screencap_shm_ptr, m_screencap_shm_size);
        m_screencap_shm_ptr = nullptr;
    }
    
    if (m_screencap_shm_fd >= 0) {
        ::close(m_screencap_shm_fd);
        if (!m_screencap_shm_name.empty()) {
            shm_unlink(m_screencap_shm_name.c_str());
            m_screencap_shm_name.clear();
        }
        m_screencap_shm_fd = -1;
    }
    
    LogInfo << "IPC connection closed";
}

bool PlayToolsIPCController::open()
{
    // 检查完整的初始化状态（包括 socket 活跃性）
    // 即使 shared memory 仍然映射，如果 socket 断开也需要重新初始化
    if (inited()) {
        // 二次检查：如果 socket 已断开，需要重新初始化
        if (m_unix_socket < 0 || !is_socket_alive()) {
            LogWarn << "IPC state appears initialized but socket is not alive, force reconnecting...";
            close();
        } else {
            return true;  // Socket 活跃，直接返回
        }
    }

    LogInfo << "Opening IPC connection, address: [" << m_ipc_address << "]";
    
    std::string socket_path;
    if (!find_socket_path(socket_path)) {
        LogError << "Failed to find Unix Socket";
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    std::string timestamp_str = std::to_string(timestamp);
    
    // shm_open 名称必须以 '/' 开头
    m_ipc_shm_name = "/maa_ipc_" + timestamp_str;
    std::string cmd_sem_name = "/maa_cmd_" + timestamp_str;
    std::string event_sem_name = "/maa_evt_" + timestamp_str;
    
    LogInfo << "IPC shm name: " << m_ipc_shm_name;
    
    if (!init_shared_memory(m_ipc_shm_name)) {
        LogError << "Failed to initialize IPC shared memory";
        close();
        return false;
    }
    
    // unix_socket_connect 内部完成完整握手：
    //   JSON 握手 → 接收响应（信号量名、截图尺寸） → 创建截图 shm
    //   → SCM_RIGHTS 传递 fd → 接收 FD_ACK → 打开信号量
    if (!unix_socket_connect(socket_path, m_ipc_shm_name, cmd_sem_name, event_sem_name)) {
        LogError << "Failed to connect via Unix Socket";
        close();
        return false;
    }
    
    if (!check_version() || !fetch_screen_res()) {
        LogError << "Failed to check version or fetch screen resolution";
        close();
        return false;
    }
    
    LogInfo << "IPC connection established successfully";
    return true;
}

bool PlayToolsIPCController::find_socket_path(std::string& socket_path) const
{
    // Swift 侧已构建完整的 socket 路径（~/Library/Containers/bundle/Data/tmp/socket_name）
    // 这里只需验证路径存在并支持替代路径
    
    if (m_ipc_address.empty()) {
        LogError << "IPC address is empty, cannot find socket";
        return false;
    }
    
    LogInfo << "IPC Socket Path Debug:";
    LogInfo << "  Raw address: [" << m_ipc_address << "]";
    LogInfo << "  Length: " << m_ipc_address.length();
    LogInfo << "  Contains '/Library/Containers/': " << (m_ipc_address.find("/Library/Containers/") != std::string::npos ? "YES" : "NO");
    
    LogInfo << "Validating socket path: [" << m_ipc_address << "]";
    
    // 验证主路径
    if (access(m_ipc_address.c_str(), F_OK) == 0) {
        socket_path = m_ipc_address;
        LogInfo << "Socket path verified: " << socket_path;
        return true;
    }
    
    int err = errno;
    LogWarn << "Primary socket path not found: " << m_ipc_address << " (errno=" << err << ", " << strerror(err) << ")";
    
    // 尝试替代路径（某些版本的 PlayTools 可能在不同位置）
    // 将 /Data/tmp/ 替换为 /Data/Library/tmp/
    std::string alt_socket = m_ipc_address;
    size_t pos = alt_socket.find("/Data/tmp/");
    if (pos != std::string::npos) {
        alt_socket.replace(pos, 10, "/Data/Library/tmp/");
        
        LogInfo << "Trying alternative socket path: [" << alt_socket << "]";
        if (access(alt_socket.c_str(), F_OK) == 0) {
            socket_path = alt_socket;
            LogInfo << "Found socket at alternative path: " << socket_path;
            return true;
        }
        
        LogWarn << "Alternative path also not found: " << alt_socket;
    }
    
    LogError << "Failed to find socket at both primary and alternative paths";
    LogError << "Please ensure PlayTools is running with IPC server enabled";
    return false;
}

bool PlayToolsIPCController::init_shared_memory(const std::string& ipc_name)
{
    LogInfo << "Creating shared memory: [" << ipc_name << "]";
    
    // 尝试清理旧的 shmem（可能存在的话）
    shm_unlink(ipc_name.c_str());
    
    // 直接创建 shmem，不使用 O_EXCL，这样即使已存在也能打开
    // 权限设为 0666，允许其他进程（PlayTools）访问
    m_ipc_shm_fd = shm_open(ipc_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (m_ipc_shm_fd < 0) {
        int err = errno;
        LogError << "shm_open failed: errno=" << err << " (" << strerror(err) << ")";
        LogError << "Sandbox may restrict shared memory creation permission";
        return false;
    }
    
    LogInfo << "Shared memory opened successfully (fd=" << m_ipc_shm_fd << ")";
    
    // 设置 shmem 大小
    if (ftruncate(m_ipc_shm_fd, IPC_TOTAL_SIZE) != 0) {
        int err = errno;
        LogError << "ftruncate failed: errno=" << err << " (" << strerror(err) << ")";
        ::close(m_ipc_shm_fd);
        m_ipc_shm_fd = -1;
        shm_unlink(ipc_name.c_str());
        return false;
    }
    
    LogInfo << "Shared memory size set to " << IPC_TOTAL_SIZE << " bytes";
    
    m_ipc_shm_ptr = mmap(nullptr, IPC_TOTAL_SIZE, PROT_READ | PROT_WRITE, 
                         MAP_SHARED, m_ipc_shm_fd, 0);
    if (m_ipc_shm_ptr == MAP_FAILED) {
        int err = errno;
        LogError << "Failed to mmap shared memory: errno=" << err << " (" << strerror(err) << ")";
        ::close(m_ipc_shm_fd);
        m_ipc_shm_fd = -1;
        shm_unlink(ipc_name.c_str());
        return false;
    }
    
    m_ipc_shm_size = IPC_TOTAL_SIZE;
    m_ipc_header = reinterpret_cast<IPCHeader*>(m_ipc_shm_ptr);
    m_ipc_cmd_ring = reinterpret_cast<uint8_t*>(m_ipc_shm_ptr) + IPC_CMD_RING_OFFSET;
    m_ipc_event_ring = reinterpret_cast<uint8_t*>(m_ipc_shm_ptr) + IPC_EVENT_RING_OFFSET;
    
    m_ipc_header->cmd_write_index = 0;
    m_ipc_header->cmd_read_index = 0;
    m_ipc_header->event_write_index = 0;
    m_ipc_header->event_read_index = 0;
    
    LogInfo << "IPC shared memory created: " << IPC_TOTAL_SIZE << " bytes";
    return true;
}

bool PlayToolsIPCController::unix_socket_connect(
    const std::string& socket_path,
    const std::string& ipc_shm_name,
    const std::string& cmd_sem_name_hint,
    const std::string& event_sem_name_hint)
{
    LogInfo << "Connecting to Unix Socket: " << socket_path;
    
    // 握手重试逻辑：服务端可能因为刚刚清闭连接而暂时无法接收新握手
    const int max_handshake_retries = 3;
    const int handshake_retry_delay_ms = 100;
    
    for (int retry = 0; retry < max_handshake_retries; ++retry) {
        m_unix_socket = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_unix_socket < 0) {
            int err = errno;
            LogError << "Failed to create socket: errno=" << err << " (" << strerror(err) << ")";
            return false;
        }
        
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);
        
        if (::connect(m_unix_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            int err = errno;
            LogError << "Failed to connect to Unix Socket: errno=" << err << " (" << strerror(err) << ")";
            LogError << "Make sure PlayTools application is running and IPC server is enabled";
            ::close(m_unix_socket);
            m_unix_socket = -1;
            return false;
        }
        
        LogInfo << "Unix Socket connected";
        
        // 步骤 1：发送握手 JSON（与 Python 客户端协议一致）
        json::value config = json::object {
            { "shm_name",       ipc_shm_name },
            { "cmd_sem_name",   cmd_sem_name_hint },
            { "event_sem_name", event_sem_name_hint }
        };
        std::string config_json = config.to_string() + "\n";
        
        if (send(m_unix_socket, config_json.c_str(), config_json.size(), 0) < 0) {
            int err = errno;
            LogError << "Failed to send handshake JSON: errno=" << err << " (" << strerror(err) << ")";
            ::close(m_unix_socket);
            m_unix_socket = -1;
            return false;
        }
        LogInfo << "Sent handshake (" << config_json.size() << " bytes)";
        
        // 步骤 2：接收 Swift 側响应（含信号量名、截图大小、屏幕分辨率）
        std::string response_line;
        char ch = 0;
        int recv_timeout_count = 0;
        bool handshake_responded = false;
        
        for (int i = 0; i < 8192; ++i) {
            ssize_t n = recv(m_unix_socket, &ch, 1, 0);
            if (n < 0) {
                int err = errno;
                if (err == EAGAIN || err == EWOULDBLOCK) {
                    // 暂时无数据，继续等待（但避免无限等待）
                    if (++recv_timeout_count > 100) {
                        LogWarn << "Timeout waiting for handshake response, retry #" << (retry + 1);
                        ::close(m_unix_socket);
                        m_unix_socket = -1;
                        if (retry < max_handshake_retries - 1) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(handshake_retry_delay_ms));
                        }
                        handshake_responded = false;
                        break;
                    }
                    usleep(10000);  // 等待 10ms 后重试
                    --i;  // 不计入迭代计数
                    continue;
                }
                LogError << "recv() error while reading response: errno=" << err << " (" << strerror(err) << ")";
                ::close(m_unix_socket);
                m_unix_socket = -1;
                return false;
            }
            if (n == 0) {
                // 连接被对端关闭
                if (response_line.empty()) {
                    LogWarn << "PlayTools server closed connection immediately after receiving handshake (retry #" << (retry + 1) << ")";
                    LogInfo << "Possible causes:";
                    LogInfo << "  1. PlayTools server is still processing the previous connection close";
                    LogInfo << "  2. Shared memory name already in use";
                    LogInfo << "  3. Server is rejecting the request";
                } else {
                    LogWarn << "Connection closed by PlayTools while reading handshake response (received " 
                             << response_line.size() << " bytes)";
                }
                ::close(m_unix_socket);
                m_unix_socket = -1;
                if (retry < max_handshake_retries - 1) {
                    LogInfo << "Retrying handshake in " << handshake_retry_delay_ms << "ms...";
                    std::this_thread::sleep_for(std::chrono::milliseconds(handshake_retry_delay_ms));
                    handshake_responded = false;
                    break;  // 重试外层循环
                } else {
                    LogError << "All handshake retries exhausted";
                    return false;
                }
            }
            if (ch == '\n') {
                handshake_responded = true;
                break;
            }
            response_line += ch;
        }
        
        if (!handshake_responded) {
            continue;  // 重试外层循环
        }
        
        LogInfo << "Handshake response received: " << response_line;
        
        auto response_opt = json::parse(response_line);
        if (!response_opt) {
            LogError << "Failed to parse response JSON";
            ::close(m_unix_socket);
            m_unix_socket = -1;
            return false;
        }
        auto& resp = *response_opt;
        
        if (resp.get("status", std::string("")) != "ok") {
            LogError << "Handshake rejected: " << resp.get("message", std::string("unknown"));
            ::close(m_unix_socket);
            m_unix_socket = -1;
            return false;
        }
        
        std::string actual_cmd_sem = resp.get("cmd_sem_name",   std::string(""));
        std::string actual_evt_sem = resp.get("event_sem_name", std::string(""));
        int screencap_size         = resp.get("screencap_size", 0);
        int handshake_width        = resp.get("width",  0);
        int handshake_height       = resp.get("height", 0);
        
        if (actual_cmd_sem.empty() || actual_evt_sem.empty() || screencap_size <= 0) {
            LogError << "Incomplete handshake response: cmd_sem=[" << actual_cmd_sem
                     << "] evt_sem=[" << actual_evt_sem
                     << "] screencap_size=" << screencap_size;
            ::close(m_unix_socket);
            m_unix_socket = -1;
            return false;
        }
        LogInfo << "Handshake OK: cmd_sem=" << actual_cmd_sem
                << " evt_sem=" << actual_evt_sem
                << " screencap=" << screencap_size << " bytes ("
                << handshake_width << "x" << handshake_height << ")";
        
        // 步骤 3：创建截图专用共享内存
        auto ts2 = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        m_screencap_shm_name = "/maa_screencap_" + std::to_string(ts2);
        
        if (!init_screencap_shared_memory(m_screencap_shm_name, static_cast<size_t>(screencap_size))) {
            LogError << "Failed to create screencap shared memory";
            ::close(m_unix_socket);
            m_unix_socket = -1;
            return false;
        }
        
        // 步骤 4：通过 SCM_RIGHTS 传递两个 fd（IPC 环形缓冲区 + 截图缓冲区）
        int fds[2] = { m_ipc_shm_fd, m_screencap_shm_fd };
        char iov_data[] = "FD_TRANSFER";
        struct iovec iov {};
        iov.iov_base = iov_data;
        iov.iov_len  = sizeof(iov_data) - 1;
        
        alignas(struct cmsghdr) char cmsg_buf[CMSG_SPACE(sizeof(fds))] = {};
        struct msghdr msg {};
        msg.msg_iov        = &iov;
        msg.msg_iovlen     = 1;
        msg.msg_control    = cmsg_buf;
        msg.msg_controllen = sizeof(cmsg_buf);
        
        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type  = SCM_RIGHTS;
        cmsg->cmsg_len   = CMSG_LEN(sizeof(fds));
        memcpy(CMSG_DATA(cmsg), fds, sizeof(fds));
        
        if (sendmsg(m_unix_socket, &msg, 0) < 0) {
            int err = errno;
            LogError << "sendmsg(SCM_RIGHTS) failed: errno=" << err << " (" << strerror(err) << ")";
            ::close(m_unix_socket);
            m_unix_socket = -1;
            return false;
        }
        LogInfo << "Sent fds via SCM_RIGHTS: ipc_fd=" << fds[0] << ", screencap_fd=" << fds[1];
        
        // 步骤 5：接收 FD_ACK 确认
        char ack_buf[16] = {};
        ssize_t ack_n = recv(m_unix_socket, ack_buf, sizeof(ack_buf) - 1, 0);
        if (ack_n < 0) {
            int err = errno;
            LogError << "recv() error while waiting for FD_ACK: errno=" << err << " (" << strerror(err) << ")";
            ::close(m_unix_socket);
            m_unix_socket = -1;
            return false;
        }
        if (ack_n < 6 || strncmp(ack_buf, "FD_ACK", 6) != 0) {
            LogError << "Did not receive FD_ACK (got " << ack_n << " bytes: [" << std::string(ack_buf, ack_n) << "])";
            ::close(m_unix_socket);
            m_unix_socket = -1;
            return false;
        }
        LogInfo << "Received FD_ACK";
        
        // 步骤 6：打开 Swift 端已创建的信号量
        if (!open_semaphores(actual_cmd_sem, actual_evt_sem)) {
            LogError << "Failed to open semaphores";
            ::close(m_unix_socket);
            m_unix_socket = -1;
            return false;
        }
        
        // 握手已知屏幕尺寸，直接设置
        if (handshake_width > 0 && handshake_height > 0) {
            m_screen_size = { handshake_width, handshake_height };
            LogInfo << "Screen size from handshake: " << handshake_width << "x" << handshake_height;
        }
        
        return true;  // 握手成功，退出重试循环
    }
    
    LogError << "Failed to complete handshake after " << max_handshake_retries << " retries";
    return false;
}

bool PlayToolsIPCController::init_screencap_shared_memory(const std::string& screencap_name, 
                                                          size_t screencap_size)
{
    shm_unlink(screencap_name.c_str());
    
    m_screencap_shm_fd = shm_open(screencap_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (m_screencap_shm_fd < 0) {
        int err = errno;
        LogError << "Failed to create screencap shared memory: errno=" << err << " (" << strerror(err) << ")";
        return false;
    }
    
    if (ftruncate(m_screencap_shm_fd, screencap_size) != 0) {
        int err = errno;
        LogError << "Failed to set screencap shared memory size: errno=" << err << " (" << strerror(err) << ")";
        ::close(m_screencap_shm_fd);
        m_screencap_shm_fd = -1;
        shm_unlink(screencap_name.c_str());
        return false;
    }
    
    m_screencap_shm_ptr = mmap(nullptr, screencap_size, PROT_READ | PROT_WRITE,
                               MAP_SHARED, m_screencap_shm_fd, 0);
    if (m_screencap_shm_ptr == MAP_FAILED) {
        int err = errno;
        LogError << "Failed to mmap screencap shared memory: errno=" << err << " (" << strerror(err) << ")";
        ::close(m_screencap_shm_fd);
        m_screencap_shm_fd = -1;
        shm_unlink(screencap_name.c_str());
        return false;
    }
    
    m_screencap_shm_size = screencap_size;
    LogInfo << "Screencap shared memory created: " << screencap_size << " bytes";
    return true;
}

bool PlayToolsIPCController::open_semaphores(const std::string& cmd_sem_name, 
                                            const std::string& event_sem_name)
{
    LogInfo << "Opening semaphores: cmd=" << cmd_sem_name << ", event=" << event_sem_name;
    
    m_cmd_sem = sem_open(cmd_sem_name.c_str(), 0, 0666, 0);
    if (m_cmd_sem == SEM_FAILED) {
        int err = errno;
        LogError << "Failed to open command semaphore: errno=" << err << " (" << strerror(err) << ")";
        LogError << "Semaphore name: " << cmd_sem_name;
        return false;
    }
    
    m_event_sem = sem_open(event_sem_name.c_str(), 0, 0666, 0);
    if (m_event_sem == SEM_FAILED) {
        int err = errno;
        LogError << "Failed to open event semaphore: errno=" << err << " (" << strerror(err) << ")";
        LogError << "Semaphore name: " << event_sem_name;
        sem_close(m_cmd_sem);
        m_cmd_sem = nullptr;
        return false;
    }
    
    LogInfo << "Semaphores opened successfully";
    return true;
}

bool PlayToolsIPCController::check_version()
{
    LogInfo << "Checking version";
    uint32_t seq_id = send_command(IPCCommandType::GET_VERSION);
    
    if (!wait_event(seq_id, 2000)) {
        LogError << "Version check timeout";
        return false;
    }
    
    LogInfo << "Version check successful";
    return true;
}

bool PlayToolsIPCController::fetch_screen_res()
{
    LogInfo << "Fetching screen resolution";
    uint32_t seq_id = send_command(IPCCommandType::GET_SIZE);
    
    IPCEventPacket evt{};
    if (!wait_event(seq_id, 2000, &evt)) {
        LogError << "Screen resolution fetch timeout";
        return false;
    }
    
    // SIZE_INFO 事件：error_code 低16位 = width，高16位 = height
    if (static_cast<IPCEventType>(evt.type) != IPCEventType::SIZE_INFO) {
        LogError << "Unexpected event type in GET_SIZE response: "
                 << static_cast<int>(evt.type);
        return false;
    }
    auto [w, h] = decode_screen_size(evt.error_code);
    if (w == 0 || h == 0) {
        LogError << "SIZE_INFO event returned invalid resolution: "
                 << w << "x" << h << " (encoded=" << evt.error_code << ")";
        return false;
    }

    m_screen_size.first  = w;
    m_screen_size.second = h;
    LogInfo << "Screen resolution updated from SIZE_INFO: "
            << m_screen_size.first << "x" << m_screen_size.second;
    return true;
}

uint32_t PlayToolsIPCController::send_command(IPCCommandType cmd_type, int32_t x, int32_t y,
                                             int32_t x2, int32_t y2, uint32_t duration)
{
    if (!inited()) {
        LogError << "IPC not initialized";
        return 0;
    }
    
    m_seq_id++;
    
    uint32_t write_idx = m_ipc_header->cmd_write_index;
    uint32_t index = write_idx % IPC_RING_SIZE;
    
    size_t offset = IPC_CMD_RING_OFFSET + index * IPC_CMD_PACKET_SIZE;
    IPCCommandPacket* cmd_pkt = reinterpret_cast<IPCCommandPacket*>(
        reinterpret_cast<uint8_t*>(m_ipc_shm_ptr) + offset);
    
    cmd_pkt->type = static_cast<uint8_t>(cmd_type);
    cmd_pkt->seq_id = m_seq_id;
    cmd_pkt->x = x;
    cmd_pkt->y = y;
    cmd_pkt->x2 = x2;
    cmd_pkt->y2 = y2;
    cmd_pkt->duration = duration;
    
    m_ipc_header->cmd_write_index = (write_idx + 1) % (IPC_RING_SIZE * 2);
    
    if (sem_post(m_cmd_sem) < 0) {
        LogWarn << "Failed to post command semaphore: errno=" << errno;
    }
    
    return m_seq_id;
}

bool PlayToolsIPCController::wait_event(uint32_t expected_seq_id [[maybe_unused]], int timeout_ms,
                                         IPCEventPacket* out_pkt)
{
    auto start = std::chrono::high_resolution_clock::now();
    
    // macOS 不支持 sem_timedwait，使用轮询方式
    const int poll_interval_us = 1000; // 1ms
    while (true) {
        int ret = sem_trywait(m_event_sem);
        if (ret == 0) {
            break; // 成功获取信号
        }
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();
        if (elapsed >= timeout_ms) {
            LogWarn << "Event wait timeout after " << elapsed << "ms";
            return false;
        }
        
        usleep(poll_interval_us);
    }
    
    uint32_t read_idx = m_ipc_header->event_read_index;
    uint32_t write_idx = m_ipc_header->event_write_index;
    
    if (read_idx == write_idx) {
        LogWarn << "Event queue is empty";
        return false;
    }
    
    uint32_t index = read_idx % IPC_RING_SIZE;
    size_t offset = IPC_EVENT_RING_OFFSET + index * IPC_EVENT_PACKET_SIZE;
    
    // 在推进读索引前，将事件数据拷贝给调用者
    IPCEventPacket* evt_pkt = reinterpret_cast<IPCEventPacket*>(
        reinterpret_cast<uint8_t*>(m_ipc_shm_ptr) + offset);
    if (out_pkt) {
        *out_pkt = *evt_pkt;
    }
    
    m_ipc_header->event_read_index = (read_idx + 1) % (IPC_RING_SIZE * 2);
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    LogTrace << "Event received in " << elapsed << "ms";
    
    return true;
}

bool PlayToolsIPCController::is_socket_alive() const noexcept
{
#ifdef __APPLE__
    if (m_unix_socket < 0) return false;
    char buf;
    ssize_t n = recv(m_unix_socket, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
    if (n == 0) {
        // 对端已优雅关闭
        return false;
    }
    if (n < 0) {
        // EAGAIN/EWOULDBLOCK 表示无数据但连接活跃
        return (errno == EAGAIN || errno == EWOULDBLOCK);
    }
    // 有未读数据（正常情况不应发生，但连接仍活跃）
    return true;
#else
    return false;
#endif
}

bool PlayToolsIPCController::reconnect()
{
    LogInfo << "Reconnecting IPC (close + open)...";
    close();
    if (!open()) {
        LogError << "IPC reconnect failed";
        return false;
    }
    LogInfo << "IPC reconnect successful";
    return true;
}

void PlayToolsIPCController::recordScreencapCost(long long cost, bool success)
{
    m_screencap_cost.emplace_back(success ? cost : -1);
    ++m_screencap_times;

    if (m_screencap_cost.size() > 30) {
        m_screencap_cost.pop_front();
    }

    if (m_screencap_times > 9 && !m_screencap_cost_reported) {
        m_screencap_cost_reported = true;

        auto filtered_cost = m_screencap_cost | std::views::filter([](auto num) { return num > 0; });
        if (filtered_cost.empty()) {
            return;
        }

        auto filtered_count = m_screencap_cost.size() - std::ranges::count(m_screencap_cost, -1);
        auto [screencap_cost_min, screencap_cost_max] = std::ranges::minmax(filtered_cost);

        json::value info = json::object {
            { "uuid", get_uuid() },
            { "what", "ScreencapCost" },
            { "details",
              json::object {
                  { "min", screencap_cost_min },
                  { "max", screencap_cost_max },
                  { "avg",
                    filtered_count > 0
                        ? std::accumulate(filtered_cost.begin(), filtered_cost.end(), 0ll) / filtered_count
                        : -1 },
              } },
        };

        if (m_screencap_cost.size() > filtered_count) {
            info["details"]["fault_times"] = m_screencap_cost.size() - filtered_count;
        }

        m_callback(AsstMsg::ConnectionInfo, info, inst());
    }
}

#else // Non-Apple platform stub implementations

bool PlayToolsIPCController::connect(
    const std::string& adb_path [[maybe_unused]],
    const std::string& address [[maybe_unused]],
    const std::string& config [[maybe_unused]])
{
    LogInfo << "PlayToolsIPC is only supported on macOS";
    return false;
}

bool PlayToolsIPCController::inited() const noexcept
{
    return false;
}

const std::string& PlayToolsIPCController::get_uuid() const
{
    static const std::string uuid("com.hypergryph.arknights.ipc.unsupported");
    return uuid;
}

size_t PlayToolsIPCController::get_pipe_data_size() const noexcept
{
    return 0;
}

size_t PlayToolsIPCController::get_version() const noexcept
{
    return 0;
}

bool PlayToolsIPCController::screencap(cv::Mat& image_payload [[maybe_unused]], bool allow_reconnect [[maybe_unused]])
{
    return false;
}

bool PlayToolsIPCController::start_game(const std::string& client_type [[maybe_unused]])
{
    return false;
}

bool PlayToolsIPCController::stop_game(const std::string& client_type [[maybe_unused]])
{
    return false;
}

bool PlayToolsIPCController::click(const Point& p [[maybe_unused]])
{
    return false;
}

bool PlayToolsIPCController::input(const std::string& text [[maybe_unused]])
{
    return false;
}

bool PlayToolsIPCController::swipe(
    const Point& p1 [[maybe_unused]],
    const Point& p2 [[maybe_unused]],
    int duration [[maybe_unused]],
    bool extra_swipe [[maybe_unused]],
    double slope_in [[maybe_unused]],
    double slope_out [[maybe_unused]],
    bool with_pause [[maybe_unused]])
{
    return false;
}

bool PlayToolsIPCController::press_esc()
{
    return false;
}

std::pair<int, int> PlayToolsIPCController::get_screen_res() const noexcept
{
    return { 0, 0 };
}

void PlayToolsIPCController::back_to_home() noexcept
{
}

#endif // __APPLE__

} // namespace asst
