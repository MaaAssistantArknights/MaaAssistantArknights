#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <vector>
#include "MaaUtils/NoWarningCVMat.hpp"

namespace asst
{
// 与 MaaFramework 的 ControlUnitAPI 接口保持 ABI 兼容
// 虚函数表布局必须与 MaaFramework 完全一致
class MaaFwControlUnitAPI
{
public:
    virtual ~MaaFwControlUnitAPI() = default;

    virtual bool connect() = 0;
    virtual bool connected() const = 0;

    virtual bool request_uuid(std::string& uuid) = 0;
    virtual uint64_t get_features() const = 0;

    virtual bool start_app(const std::string& intent) = 0;
    virtual bool stop_app(const std::string& intent) = 0;

    virtual bool screencap(cv::Mat& image) = 0;

    virtual bool click(int x, int y) = 0;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) = 0;

    virtual bool touch_down(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_move(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_up(int contact) = 0;

    virtual bool click_key(int key) = 0;
    virtual bool input_text(const std::string& text) = 0;

    virtual bool key_down(int key) = 0;
    virtual bool key_up(int key) = 0;

    virtual bool scroll(int dx, int dy) = 0;
};

class MaaFwAdbControlUnitAPI : public MaaFwControlUnitAPI
{
public:
    virtual ~MaaFwAdbControlUnitAPI() = default;

    virtual bool find_device(std::vector<std::string>& devices) = 0;
    virtual bool
        shell(const std::string& cmd, std::string& output, std::chrono::milliseconds timeout = std::chrono::milliseconds(20000)) = 0;
};

// 与 MaaFramework 的 MaaControllerFeature 兼容的常量
namespace MaaFeature
{
constexpr uint64_t None = 0;
constexpr uint64_t UseMouseDownAndUpInsteadOfClick = 1ULL << 1;
constexpr uint64_t UseKeyboardDownAndUpInsteadOfClick = 1ULL << 2;
} // namespace MaaFeature
} // namespace asst
