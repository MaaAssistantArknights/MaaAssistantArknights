#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "MaaUtils/NoWarningCVMat.hpp"

namespace asst
{
class AVDExtras
{
public:
    AVDExtras() = default;

    virtual ~AVDExtras();

    bool inited() const { return m_inited; }

    bool init(const std::string& webrtc_start_cmd, const std::string& webrtc_stop_cmd);
    bool reload();
    void uninit();

    std::optional<cv::Mat> screencap();

private:
    bool connect_avd();
    bool init_screencap();
    void disconnect_avd();

private:
    std::string m_webrtc_start_cmd;
    std::string m_webrtc_stop_cmd;

    bool m_inited = false;

    std::string m_shm_name;
#ifdef _WIN32
    void* m_shm_handle = nullptr;
#else
    int m_shm_fd = -1;
    size_t m_shm_size = 0;
#endif
    // https://android.googlesource.com/platform/external/qemu/+/7d785f50047e1de08952811aff552ccbfa2449c4/android/android-ui/modules/aemu-recording/src/android/recording/video/VideoFrameSharer.h#53
    struct VideoInfo
    {
        uint32_t width;
        uint32_t height;
        uint32_t fps;  // target framerate (always 60)
        uint32_t frameNumber;
        uint64_t tsUs; // timestamp when this frame was received
    };

    void* m_shm = nullptr;
};
}
