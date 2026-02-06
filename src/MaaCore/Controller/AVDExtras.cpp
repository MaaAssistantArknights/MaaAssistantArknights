#include "AVDExtras.h"

#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include <cerrno>
#include <sstream>

#ifdef _WIN32
#include "MaaUtils/SafeWindows.hpp"
#else
#include <sys/mman.h>
#include <sys/stat.h>
#endif

namespace asst
{
AVDExtras::~AVDExtras()
{
    uninit();
}

bool AVDExtras::init(const std::string& webrtc_start_cmd, const std::string& webrtc_stop_cmd)
{
    m_webrtc_start_cmd = webrtc_start_cmd;
    m_webrtc_stop_cmd = webrtc_stop_cmd;
    m_inited = connect_avd() && init_screencap();
    return m_inited;
}

bool AVDExtras::reload()
{
    disconnect_avd();
    m_inited = connect_avd() && init_screencap();
    LogInfo << "Reload AVDExtras: " << VAR(m_inited);
    return m_inited;
}

void AVDExtras::uninit()
{
    if (m_inited) {
        m_inited = false;
        disconnect_avd();
    }
}

std::optional<cv::Mat> AVDExtras::screencap()
{
    if (m_shm == nullptr) {
        LogError << "AVDExtras m_shm is null";
        return std::nullopt;
    }

    VideoInfo* video_info = static_cast<VideoInfo*>(m_shm);
    uint8_t* frame_data = static_cast<uint8_t*>(m_shm) + sizeof(VideoInfo);
    cv::Mat bgra(video_info->height, video_info->width, CV_8UC4, frame_data);
    cv::Mat bgr;
    cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);

    return bgr;
}

bool AVDExtras::connect_avd()
{
    LogInfo << "Starting AVD shared memory: " << VAR(m_webrtc_start_cmd);
    std::string output = platform::call_command(m_webrtc_start_cmd);
    LogInfo << "Starting AVD shared memory: " << VAR(output);

    std::istringstream iss(output);
    iss >> m_shm_name;

    std::string status;
    iss >> status;
    if (status != "OK") {
        LogError << "Expected OK but got " << VAR(status);
        return false;
    }

#ifdef _WIN32
    // https://android.googlesource.com/platform/external/qemu/+/7d785f50047e1de08952811aff552ccbfa2449c4/android/emu/base/memory/src/android/base/memory/SharedMemory_win32.cpp#37
    m_shm_name = "SHM_" + m_shm_name;
#endif
    LogInfo << "AVD shared memory name: " << VAR(m_shm_name);
    return true;
}

bool AVDExtras::init_screencap()
{
    LogInfo << VAR(m_shm_name);

#ifdef _WIN32
    std::wstring wname(m_shm_name.begin(), m_shm_name.end());
    m_shm_handle = OpenFileMappingW(FILE_MAP_READ, false, wname.c_str());
    if (m_shm_handle == nullptr) {
        LogError << "AVDExtras failed to create file mapping: " << VAR(GetLastError());
        return false;
    }

    m_shm = MapViewOfFile(m_shm_handle, FILE_MAP_READ, 0, 0, 0);
    if (m_shm == nullptr) {
        LogError << "AVDExtras failed to map view of file: " << VAR(GetLastError());
        CloseHandle(m_shm_handle);
        m_shm_handle = nullptr;
        return false;
    }
#else
    m_shm_fd = shm_open(m_shm_name.c_str(), O_RDONLY, 0);
    if (m_shm_fd == -1) {
        LogError << "AVDExtras failed to open shared memory: " << VAR(errno);
        return false;
    }

    struct stat st;
    if (fstat(m_shm_fd, &st) == -1) {
        LogError << "AVDExtras failed to stat shared memory: " << VAR(errno);
        close(m_shm_fd);
        m_shm_fd = -1;
        return false;
    }
    m_shm_size = st.st_size;
    LogInfo << "AVD shared memory size: " << VAR(m_shm_size);

    m_shm = mmap(nullptr, m_shm_size, PROT_READ, MAP_SHARED, m_shm_fd, 0);
    if (m_shm == MAP_FAILED) {
        LogError << "AVDExtras failed to mmap shared memory: " << VAR(errno);
        m_shm = nullptr;
        return false;
    }
#endif

    VideoInfo* video_info = static_cast<VideoInfo*>(m_shm);
    LogInfo << "VideoInfo: " << VAR(video_info->width) << VAR(video_info->height) << VAR(video_info->fps)
            << VAR(video_info->frameNumber) << VAR(video_info->tsUs);

    return true;
}

void AVDExtras::disconnect_avd()
{
    LogInfo << VAR(m_shm_name);

#ifdef _WIN32
    if (m_shm != nullptr) {
        UnmapViewOfFile(m_shm);
        m_shm = nullptr;
    }

    if (m_shm_handle != nullptr) {
        CloseHandle(m_shm_handle);
        m_shm_handle = nullptr;
    }
#else
    if (m_shm != nullptr && m_shm != MAP_FAILED) {
        munmap(m_shm, m_shm_size);
        m_shm = nullptr;
    }

    if (m_shm_fd != -1) {
        close(m_shm_fd);
        m_shm_fd = -1;
    }
#endif

    LogInfo << "Stopping AVD shared memory: " << VAR(m_webrtc_stop_cmd);
    std::string output = platform::call_command(m_webrtc_stop_cmd);
    LogInfo << "Stopping AVD shared memory: " << VAR(output);
}
}
