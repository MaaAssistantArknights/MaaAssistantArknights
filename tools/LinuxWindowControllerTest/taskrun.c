// taskrun.c — 通过 LinuxWindowController 运行一个 MAA 任务的验证驱动
// 用法: taskrun <resource_dir> <window_name> <task_type> <task_params_json> [run_seconds]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "AsstCaller.h"

static void on_msg(AsstMsgId msg, const char* details_json, void* custom_arg)
{
    printf("[msg %d] %s\n", (int)msg, details_json ? details_json : "");
    fflush(stdout);
}

int main(int argc, char** argv)
{
    if (argc < 5) {
        fprintf(stderr, "usage: %s <resource_dir> <window_name> <task_type> <task_params_json> [run_seconds]\n", argv[0]);
        return 2;
    }

    const char* resource_dir = argv[1];
    const char* window_name = argv[2];
    const char* task_type = argv[3];
    const char* task_params = argv[4];
    int run_seconds = argc >= 6 ? atoi(argv[5]) : 90;

    printf("MaaCore version: %s\n", AsstGetVersion());

    if (!AsstLoadResource(resource_dir)) {
        fprintf(stderr, "AsstLoadResource(%s) failed\n", resource_dir);
        return 1;
    }

    // 海外客户端资源包（若存在）
    char global_res[1024];
    snprintf(global_res, sizeof(global_res), "%s/resource/global/YoStarEN", resource_dir);
    if (access(global_res, F_OK) == 0) {
        printf("loading overseas resource: %s\n", global_res);
        AsstLoadResource(global_res);
    }

    AsstHandle handle = AsstCreateEx(on_msg, NULL);
    if (handle == NULL) {
        fprintf(stderr, "AsstCreateEx failed\n");
        return 1;
    }

    printf("attaching to window \"%s\"...\n", window_name);
    AsstAsyncAttachWindowByName(handle, window_name, 0, 1);
    if (!AsstConnected(handle)) {
        fprintf(stderr, "NOT connected after attach\n");
        AsstDestroy(handle);
        return 1;
    }
    printf("CONNECTED.\n");

    printf("appending task: %s %s\n", task_type, task_params);
    AsstTaskId task_id = AsstAppendTask(handle, task_type, task_params);
    printf("task id: %d\n", (int)task_id);
    if (task_id == 0) {
        fprintf(stderr, "append_task failed\n");
        AsstDestroy(handle);
        return 1;
    }

    printf("starting...\n");
    AsstStart(handle);

    for (int i = 0; i < run_seconds; i++) {
        sleep(1);
        if (!AsstRunning(handle)) {
            printf("task finished after %d s\n", i);
            break;
        }
    }

    AsstStop(handle);
    AsstDestroy(handle);
    printf("done.\n");
    return 0;
}
