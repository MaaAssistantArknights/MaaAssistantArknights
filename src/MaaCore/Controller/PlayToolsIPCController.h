#pragma once

#include "ControllerAPI.h"
#include "MinitouchController.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <memory>
#include <string>

#include "Common/AsstMsg.h"
#include "InstHelper.h"

#ifdef __APPLE__
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>

// ============================================================
// IPC 协议常量与数据结构（仅 macOS）
// ============================================================

// 命令类型（与 TCP 版 MaaTools 协议对齐：触摸由上层 Controller 负责插值/时序）
enum class IPCCommandType : uint8_t
{
    SCREENSHOT  = 0,
    TOUCH_BEGAN = 1,  // 对应 TCP TUCH/phase=0 (Began)
    TOUCH_MOVED = 2,  // 对应 TCP TUCH/phase=1 (Moved)
    TOUCH_ENDED = 3,  // 对应 TCP TUCH/phase=3 (Ended)
    GET_SIZE    = 4,
    GET_VERSION = 5,
    TERMINATE   = 6,
};

// 事件类型
enum class IPCEventType : uint8_t
{
    SCREENSHOT_READY = 0,
    ACK = 1,
    ERROR = 2,
    SIZE_INFO = 3,
    VERSION_INFO = 4,
};

// IPC 内存布局常量
static constexpr int IPC_HEADER_SIZE = 256;
static constexpr int IPC_CMD_PACKET_SIZE = 32;
static constexpr int IPC_EVENT_PACKET_SIZE = 32;
static constexpr int IPC_RING_SIZE = 2048;
static constexpr int IPC_CMD_RING_OFFSET = IPC_HEADER_SIZE;
static constexpr int IPC_CMD_RING_SIZE = IPC_RING_SIZE * IPC_CMD_PACKET_SIZE;
static constexpr int IPC_EVENT_RING_OFFSET = IPC_CMD_RING_OFFSET + IPC_CMD_RING_SIZE;
static constexpr int IPC_EVENT_RING_SIZE = IPC_RING_SIZE * IPC_EVENT_PACKET_SIZE;
static constexpr int IPC_TOTAL_SIZE = IPC_HEADER_SIZE + IPC_CMD_RING_SIZE + IPC_EVENT_RING_SIZE;

// 命令包结构 (32 bytes, Cache Line aligned)
struct __attribute__((packed, aligned(32))) IPCCommandPacket
{
    uint8_t type;           // 命令类型
    uint8_t reserved_1[3];  // 对齐到 4 字节
    uint32_t seq_id;        // 序列号
    int32_t x;              // X 坐标
    int32_t y;              // Y 坐标
    int32_t x2;             // X2 坐标（用于 SWIPE/DRAG）
    int32_t y2;             // Y2 坐标（用于 SWIPE/DRAG）
    uint32_t duration;      // 持续时间（毫秒）
    uint8_t reserved_2[4];  // 预留字节
};

static_assert(sizeof(IPCCommandPacket) == 32, "IPCCommandPacket must be 32 bytes");

// 事件包结构 (32 bytes)
// Python pack format: '<B3xIi' (前 12 字节有效，其余保留)
// SIZE_INFO: error_code = (width & 0xFFFF) | ((height & 0xFFFF) << 16)
// VERSION_INFO: error_code = version
struct __attribute__((packed, aligned(32))) IPCEventPacket
{
    uint8_t type;           // 事件类型
    uint8_t reserved_1[3];  // 对齐到 4 字节
    uint32_t req_seq_id;    // 请求序列号
    int32_t error_code;     // 错误码 / 附加数据（尺寸/版本编码在此）
    uint8_t reserved_2[20]; // 保留字段，补齐至 32 字节
};

static_assert(sizeof(IPCEventPacket) == 32, "IPCEventPacket must be 32 bytes");

// 解码 SIZE_INFO 的 error_code: 低16位 = 宽，高16位 = 高
inline std::pair<int, int> decode_screen_size(int32_t encoded)
{
    int w = static_cast<int>(static_cast<uint32_t>(encoded) & 0xFFFF);
    int h = static_cast<int>((static_cast<uint32_t>(encoded) >> 16) & 0xFFFF);
    return { w, h };
}

// IPC 头部结构（与 Python/Swift 保持一致）
// 内存布局（各字段 Cache Line 对齐，64 字节一行）：
//   offset   0: cmd_write_index  （C++ 写，Swift 读）
//   offset  64: cmd_read_index   （Swift 写，C++ 读）
//   offset 128: event_write_index（Swift 写，C++ 读）
//   offset 192: event_read_index （C++ 写，Swift 读）
struct __attribute__((packed, aligned(64))) IPCHeader
{
    uint32_t cmd_write_index;       // 命令写索引（偏移 0）
    uint32_t reserved_0[15];        // 对齐到 64 字节

    uint32_t cmd_read_index;        // 命令读索引（偏移 64）
    uint32_t reserved_1[15];

    uint32_t event_write_index;     // 事件写索引（偏移 128）
    uint32_t reserved_2[15];

    uint32_t event_read_index;      // 事件读索引（偏移 192）
    uint32_t reserved_3[15];
};

static_assert(sizeof(IPCHeader) == 256, "IPCHeader must be 256 bytes");

#endif // __APPLE__

namespace asst
{

class PlayToolsIPCController : public ControllerAPI, protected InstHelper
{
public:
    PlayToolsIPCController(const AsstCallback& callback, Assistant* inst, PlatformType type);
    PlayToolsIPCController(const PlayToolsIPCController&) = delete;
    PlayToolsIPCController(PlayToolsIPCController&&) = delete;
    virtual ~PlayToolsIPCController();

    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;
    virtual bool inited() const noexcept override;

    virtual const std::string& get_uuid() const override;

    virtual size_t get_pipe_data_size() const noexcept override;

    virtual size_t get_version() const noexcept override;

    virtual bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;

    virtual bool start_game(const std::string& client_type) override;
    virtual bool stop_game(const std::string& client_type) override;

    virtual bool click(const Point& p) override;

    virtual bool input(const std::string& text) override;

    virtual bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        bool extra_swipe = false,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    virtual bool inject_input_event([[maybe_unused]] const InputEvent& event) override { return false; }

    virtual bool press_esc() override;

    virtual ControlFeat::Feat support_features() const noexcept override { return ControlFeat::NONE; }

    virtual std::pair<int, int> get_screen_res() const noexcept override;

    PlayToolsIPCController& operator=(const PlayToolsIPCController&) = delete;
    PlayToolsIPCController& operator=(PlayToolsIPCController&&) = delete;

    virtual void back_to_home() noexcept override;

protected:
    AsstCallback m_callback;

    std::string m_ipc_address;              // IPC 连接地址（Bundle ID 或容器路径）
    std::pair<int, int> m_screen_size = { 0, 0 };

    std::deque<long long> m_screencap_cost; // 截图用时
    int m_screencap_times = 0;              // 截图次数
    bool m_screencap_cost_reported = false; // 是否已报告截图耗时

#ifdef __APPLE__
    // IPC 环形缓冲区相关
    int m_ipc_shm_fd = -1;                  // 共享内存文件描述符
    void* m_ipc_shm_ptr = nullptr;          // 共享内存指针
    size_t m_ipc_shm_size = 0;              // 共享内存大小
    IPCHeader* m_ipc_header = nullptr;      // 头部指针
    uint8_t* m_ipc_cmd_ring = nullptr;      // 命令环形缓冲区
    uint8_t* m_ipc_event_ring = nullptr;    // 事件环形缓冲区
    
    // 截图共享内存
    int m_screencap_shm_fd = -1;            // 截图共享内存文件描述符
    void* m_screencap_shm_ptr = nullptr;    // 截图共享内存指针
    size_t m_screencap_shm_size = 0;        // 截图共享内存大小
    
    // POSIX 信号量
    sem_t* m_cmd_sem = nullptr;             // 命令信号量（通知有新命令可读）
    sem_t* m_event_sem = nullptr;           // 事件信号量（通知有新事件可读）
    std::string m_cmd_sem_name;
    std::string m_event_sem_name;
    
    // 共享内存名称（用于 close() 时 shm_unlink）
    std::string m_ipc_shm_name;
    std::string m_screencap_shm_name;
    
    // 连接状态
    int m_unix_socket = -1;                 // Unix Socket 文件描述符
    uint32_t m_seq_id = 0;                  // 序列号计数器
#endif

    static constexpr int DefaultClickDelay = 50;
    static constexpr int DefaultSwipeDelay = 5;

    bool toucher_down(const Point& p, const int delay = DefaultClickDelay);
    bool toucher_move(const Point& p, const int delay = DefaultSwipeDelay);
    bool toucher_up(const Point& p, const int delay = DefaultClickDelay);
    void toucher_wait(const int delay);

private:
    static constexpr int MinimalVersion = 2;
    static constexpr uint32_t IPCMagic = 0x53435049; // 'IPCS'
    
#ifdef __APPLE__
    void close();
    bool open();
    bool find_socket_path(std::string& socket_path) const;
    bool unix_socket_connect(const std::string& socket_path,
                             const std::string& ipc_shm_name,
                             const std::string& cmd_sem_name_hint,
                             const std::string& event_sem_name_hint);
    bool init_shared_memory(const std::string& ipc_name);
    bool init_screencap_shared_memory(const std::string& screencap_name, size_t screencap_size);
    bool open_semaphores(const std::string& cmd_sem_name, const std::string& event_sem_name);
    bool check_version();
    bool fetch_screen_res();
    
    uint32_t send_command(IPCCommandType cmd_type, int32_t x = 0, int32_t y = 0, 
                         int32_t x2 = 0, int32_t y2 = 0, uint32_t duration = 0);
    bool wait_event(uint32_t expected_seq_id, int timeout_ms = 5000,
                    IPCEventPacket* out_pkt = nullptr);
    bool toucher_commit(IPCCommandType phase, const Point& p, const int delay);
#endif
    void recordScreencapCost(long long cost, bool success);
    bool is_socket_alive() const noexcept;
    bool reconnect();
};

} // namespace asst
