// LinuxWindowController 测试驱动
// 用法: LinuxWindowControllerTest <resource_dir> <window_name>
// 流程: 加载资源 -> 按窗口标题绑定 -> 截图 -> 点击指定坐标 -> 再截图
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AsstCaller.h"

static void on_msg(AsstMsgId msg, const char* details_json, void* custom_arg)
{
    printf("[msg %d] %s\n", (int)msg, details_json ? details_json : "");
    fflush(stdout);
}

static int save_png(const char* path, AsstHandle handle)
{
    // 与 GUI 一致：分配固定大小缓冲区，用 AsstGetNullSize() 判断失败
    unsigned char* buf = (unsigned char*)malloc(1280 * 720 * 4);
    if (!buf) {
        return -1;
    }
    AsstSize got = AsstGetImage(handle, buf, 1280 * 720 * 4);
    if (got == 0 || got == AsstGetNullSize()) {
        printf("get_image failed (got=%llu)\n", (unsigned long long)got);
        free(buf);
        return -1;
    }
    printf("image bytes: %llu\n", (unsigned long long)got);
    FILE* f = fopen(path, "wb");
    if (f) {
        fwrite(buf, 1, got, f);
        fclose(f);
        printf("saved: %s\n", path);
    }
    free(buf);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <resource_dir> <window_name> [click_x click_y]\n", argv[0]);
        return 2;
    }

    const char* resource_dir = argv[1];
    const char* window_name = argv[2];

    printf("MaaCore version: %s\n", AsstGetVersion());

    if (!AsstLoadResource(resource_dir)) {
        fprintf(stderr, "AsstLoadResource(%s) failed\n", resource_dir);
        return 1;
    }

    AsstHandle handle = AsstCreateEx(on_msg, NULL);
    if (handle == NULL) {
        fprintf(stderr, "AsstCreateEx failed\n");
        return 1;
    }

    printf("attaching to window \"%s\" (focus_for_keys=off)...\n", window_name);
    AsstAsyncCallId id = AsstAsyncAttachWindowByName(handle, window_name, 0 /* focus_for_keys */, 1 /* block */);
    printf("attach call id=%d\n", (int)id);

    if (!AsstConnected(handle)) {
        fprintf(stderr, "NOT connected after attach\n");
        AsstDestroy(handle);
        return 1;
    }
    printf("CONNECTED.\n");

    char uuid[128] = { 0 };
    AsstGetUUID(handle, uuid, sizeof(uuid));
    printf("uuid: %s\n", uuid);

    AsstAsyncScreencap(handle, 1);
    if (save_png("/home/poland/maa-work/shots/ctrl_before.png", handle) != 0) {
        fprintf(stderr, "screencap failed\n");
        AsstDestroy(handle);
        return 1;
    }

    if (argc >= 5) {
        int x = atoi(argv[3]);
        int y = atoi(argv[4]);
        printf("clicking (%d, %d) [1280x720 space]...\n", x, y);
        AsstAsyncCallId click_id = AsstAsyncClick(handle, x, y, 1 /* block */);
        printf("click call id=%d\n", (int)click_id);
        AsstAsyncScreencap(handle, 1);
        save_png("/home/poland/maa-work/shots/ctrl_after.png", handle);
    }

    AsstDestroy(handle);
    printf("done.\n");
    return 0;
}
