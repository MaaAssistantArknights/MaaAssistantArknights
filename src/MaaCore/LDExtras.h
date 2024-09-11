#pragma once

#include "Common/AsstConf.h"

#if ASST_WITH_EMULATOR_EXTRAS

#include <filesystem>
#include <optional>
#include <string>
#include "Utils/Platform/SafeWindows.h"

#include "LD/dnopengl/dnopengl.h"

#include "Utils/LibraryHolder.hpp"
#include "Utils/NoWarningCVMat.h"

namespace asst
{
class LDExtras : public LibraryHolder<LDExtras>
{
public:
    LDExtras() = default;

    virtual ~LDExtras();

    bool inited() const { return inited_; }

    bool init(HWND hwnd, unsigned int ld_inst_index, const std::filesystem::path& ld_path);
    bool reload();
    void uninit();

    std::optional<cv::Mat> screencap();

private:
    bool load_ld_library();
    bool connect_ld();
    bool init_screencap();
    void disconnect_ld();

private:
    HWND hwnd_ = nullptr;
    unsigned int ld_inst_index_ = 0;

    IScreenShotClass* screenshot_instance_ = nullptr;
    int display_width_ = 0;
    int display_height_ = 0;
    std::vector<unsigned char> display_buffer_;

    bool inited_ = false;

private:
    std::filesystem::path ld_path_;
    inline static const std::string kInitialGLFuncName = "initialGL";
    inline static const std::string kUninitialGLFuncName = "uninitialGL";
    inline static const std::string kReadPixelsFuncName = "readPixels";
    inline static const std::string kCreateScreenShotInstanceFuncName = "CreateScreenShotInstance";

private:
    std::function<decltype(initialGL)> initial_gl_func_;
    std::function<decltype(uninitialGL)> uninitial_gl_func_;
    std::function<decltype(readPixels)> read_pixels_func_;
    std::function<decltype(CreateScreenShotInstance)> create_screenshot_instance_func_;
};
}
#endif
