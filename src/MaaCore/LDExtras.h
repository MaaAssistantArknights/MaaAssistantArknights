#pragma once

#include "Common/AsstConf.h"

#if ASST_WITH_EMULATOR_EXTRAS

#include "Utils/Platform/SafeWindows.h"
#include <filesystem>
#include <optional>
#include <string>

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

    bool init(
        const std::filesystem::path& ld_path,
        unsigned int ld_inst_index,
        unsigned int ld_pid,
        int width,
        int height);
    bool reload();
    void uninit();

    std::optional<cv::Mat> screencap() const;

private:
    bool load_ld_library();
    bool connect_ld();
    bool init_screencap();
    void disconnect_ld();

private:
    unsigned int ld_inst_index_ = 0;
    unsigned int ld_pid_ = 0;

    IScreenShotClass* screenshot_instance_ = nullptr;
    int display_width_ = 0;
    int display_height_ = 0;

    bool inited_ = false;

private:
    std::filesystem::path ld_path_;
    inline static const std::string kCreateScreenShotInstanceFuncName = "CreateScreenShotInstance";

private:
    std::function<decltype(CreateScreenShotInstance)> create_screenshot_instance_func_;
};
}
#endif
