#pragma once

#ifdef __ANDROID__

#include <string>
#include <dlfcn.h>
#include <cstdint>

namespace asst
{

// Frame info structure - for reading screen frames (32 bytes)
typedef struct {
    uint32_t width;          // Width - 4 bytes
    uint32_t height;         // Height - 4 bytes
    uint32_t stride;         // Row stride - 4 bytes
    uint32_t length;         // Data length - 4 bytes
    void *data;              // Pixel data pointer - 8 bytes
    void *frame_ref;         // FrameBuffer pointer for unlocking - 8 bytes
} FrameInfo;

typedef enum {
    IMAGE_FORMAT_UNKNOWN = 0,
    IMAGE_FORMAT_RGBA_8888 = 2
} ImageFormat;

typedef enum {
    START_GAME = 1,
    STOP_GAME = 2,
    INPUT = 4,
    TOUCH_DOWN = 6,
    TOUCH_MOVE = 7,
    TOUCH_UP = 8,
    KEY_DOWN = 9,
    KEY_UP = 10
} MethodType;

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    const char *package_name;   // 应用包名
    int force_stop;             // 是否强制停止 (0=false, 1=true)
} StartGameArgs;

typedef struct {
    const char *client_type;
} StopGameArgs;

typedef struct {
    const char *text;
} InputArgs;

typedef struct {
    Position p;
} TouchArgs;

typedef struct {
    int key_code;
} KeyArgs;

typedef union {
    StartGameArgs start_game;
    StopGameArgs stop_game;
    InputArgs input;
    TouchArgs touch;
    KeyArgs key;
} ArgUnion;

typedef struct {
    int display_id;
    MethodType method;
    ArgUnion args;
} MethodParam;

class AndroidExternalLib
{
public:
    static AndroidExternalLib& instance();

    static bool load(const std::string& library_path);

    AndroidExternalLib(const AndroidExternalLib&) = delete;
    AndroidExternalLib& operator=(const AndroidExternalLib&) = delete;

    ~AndroidExternalLib();


    FrameInfo GetLockedPixels(void) const;


    int UnlockPixels(FrameInfo info) const;

    void* AttachThread() const;

    int DetachThread(void* env) const;

    int DispatchInputMessage(MethodParam param) const;

    bool is_loaded() const;

private:
    AndroidExternalLib();

    bool load_library(const std::string& library_path);
    bool resolve_symbols();
    void clear_symbols();

    void* m_handle = nullptr;
    std::string m_library_path;
    bool m_loaded = false;

    typedef FrameInfo (*GetLockedPixels_t)(void);
    typedef int (*UnlockPixels_t)(FrameInfo);
    typedef void* (*AttachThread_t)(void);
    typedef int (*DetachThread_t)(void*);
    typedef int (*DispatchInputMessage_t)(MethodParam);

    struct FunctionPointers {
        GetLockedPixels_t get_locked_pixels = nullptr;
        UnlockPixels_t unlock_pixels = nullptr;
        AttachThread_t attach_thread = nullptr;
        DetachThread_t detach_thread = nullptr;
        DispatchInputMessage_t dispatch_input_message = nullptr;
    } m_funcs;

    static AndroidExternalLib* s_instance;
};

} // namespace asst

#endif // __ANDROID__