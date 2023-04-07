#pragma once

#include "AsstCallerTypes.h"
#include "AsstInterface.h"
#include "AsstPort.h"

ASST_INTERFACE(CustomImage,
          int32_t width;         // 图像宽度W
          int32_t height;        // 图像高度
          int32_t channels;      // 图像通道数
          int32_t pixel_format;  // 像素格式，例如 CV_8UC3 表示每个像素有三个 8 位无符号整数通道
          int32_t channel_order; // 像素通道顺序，例如 BGR、RGB 等
          uint8_t * data;        // 指向图像数据的指针
);

enum EventType : int
{
    TOUCH_DOWN,
    TOUCH_UP,
    TOUCH_MOVE,
    TOUCH_RESET,
    KEY_DOWN,
    KEY_UP,
    WAIT_MS,
    COMMIT,
    UNKNOWN = INT_MAX
};

struct AsstInputEvent
{
    int32_t type;
    int64_t pointer_id;
    int32_t x;
    int32_t y;
    int32_t keycode;
    int64_t milis;
};

ASST_INTERFACE(AsstCustomController,
          AsstBool (*connect)(void* self, const char* adb_path, const char* address, const char* config); // 连接设备
          AsstBool(*inited)(void* self);                                // 是否已经初始化
          AsstBool(*set_swipe_with_pause)(void* self, AsstBool enable); // 设置是否在滑动时暂停
          AsstBool(*get_uuid)(void* self, char* buff, uint64_t len);    // 获取设备的 UUID
          AsstBool(*screencap)(void* self, CustomImage** image, AsstBool allow_reconnect); // 截屏
          AsstBool(*start_game)(void* self, const char* client_type);                      // 启动游戏
          AsstBool(*stop_game)(void* self);                                                // 停止游戏
          AsstBool(*click)(void* self, int32_t x, int32_t y);                              // 点击
          AsstBool(*swipe)(void* self, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration,
                           AsstBool extra_swipe, double slope_in, double slope_out, AsstBool with_pause); // 滑动
          AsstBool(*inject_input_event)(void* self, const AsstInputEvent* event); // 注入输入事件
          AsstBool(*press_esc)(void* self);                                       // 按下 ESC 键
          AsstControlFeat(*support_features)(void* self);                         // 支持的功能
          AsstBool(*get_screen_res)(void* self, int32_t* width, int32_t* height); // 获取屏幕分辨率
);
