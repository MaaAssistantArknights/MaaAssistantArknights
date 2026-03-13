#include "InterfaceTask.h"

#include "Controller/Controller.h"

bool asst::InterfaceTask::run()
{
    // PC 连接模式（Win32）下将光标移到被控窗口左下角外侧，避免悬停效果干扰画面。
    // 其他控制器的默认实现为空操作，此处无需平台判断。
    if (ctrler()) {
        ctrler()->move_cursor_out_of_view();
    }

    if (PackageTask::run()) {
        return true;
    }

    save_img(utils::path("debug") / utils::path("interface"));
    return false;
}
