#include "task_queue_wine.h"
#include <stdlib.h>

// clang-format off
#include <windef.h>
#include <winbase.h>

// clang-format on

typedef struct _TaskQueueWorkerWine
{
    TaskQueueWorker base;
    HANDLE thread;
} TaskQueueWorkerWine;

// 队列是 worker 的唯一持有者：join 之后 release 一次，不需要引用计数。
static void task_queue_worker_wine_release(TaskQueueWorker* worker)
{
    TaskQueueWorkerWine* worker_wine = (TaskQueueWorkerWine*)worker;
    if (worker_wine->thread) {
        CloseHandle(worker_wine->thread);
    }
    free(worker_wine);
}

static void task_queue_worker_wine_join(TaskQueueWorker* worker)
{
    TaskQueueWorkerWine* worker_wine = (TaskQueueWorkerWine*)worker;
    if (!worker_wine->thread) {
        return;
    }
    // 创建方持有的线程句柄无需 worker 自行发布，可避免 pthread_t 的同步问题。
    const DWORD wait_result = WaitForSingleObject(worker_wine->thread, INFINITE);
    if (wait_result != WAIT_OBJECT_0) {
        // 等待失败后继续释放会导致 worker 访问已释放内存。
        abort();
    }
}

static DWORD WINAPI worker(void* arg)
{
    TaskQueueWorkerWine* worker = arg;
    TaskQueue* queue = worker->base.queue;
    while (1) {
        Task* task = task_queue_pop(queue);
        if (task == NULL) {
            break;
        }
        task_invoke(task);
        task_release(task);
    }
    return 0;
}

TaskQueueWorker* task_queue_worker_wine_new(TaskQueue* queue)
{
    TaskQueueWorkerWine* worker_wine = calloc(1, sizeof(TaskQueueWorkerWine));
    if (!worker_wine) {
        return NULL;
    }
    worker_wine->base.queue = queue;
    worker_wine->base.release = task_queue_worker_wine_release;
    worker_wine->base.join = task_queue_worker_wine_join;

    worker_wine->thread = CreateThread(NULL, 0, worker, worker_wine, 0, NULL);
    if (!worker_wine->thread) {
        free(worker_wine);
        return NULL;
    }
    return (TaskQueueWorker*)worker_wine;
}
