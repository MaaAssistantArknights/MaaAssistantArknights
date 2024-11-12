#include "task_queue_wine.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>

// clang-format off
#include <windef.h>
#include <winbase.h>

// clang-format on

typedef struct _TaskQueueWorkerWine
{
    TaskQueueWorker base;
    atomic_int ref_count;
    pthread_t posix_thread;
} TaskQueueWorkerWine;

static void task_queue_worker_wine_addref(TaskQueueWorker* worker)
{
    TaskQueueWorkerWine* worker_wine = (TaskQueueWorkerWine*)worker;
    atomic_fetch_add(&worker_wine->ref_count, 1);
}

static void task_queue_worker_wine_release(TaskQueueWorker* worker)
{
    TaskQueueWorkerWine* worker_wine = (TaskQueueWorkerWine*)worker;
    if (atomic_fetch_sub(&worker_wine->ref_count, 1) == 1) {
        free(worker_wine);
    }
}

static void task_queue_worker_wine_join(TaskQueueWorker* worker)
{
    TaskQueueWorkerWine* worker_wine = (TaskQueueWorkerWine*)worker;
    pthread_join(worker_wine->posix_thread, NULL);
}

static DWORD WINAPI worker(void* arg)
{
    TaskQueueWorkerWine* worker = arg;
    worker->posix_thread = pthread_self();
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
    TaskQueueWorkerWine* worker_wine = malloc(sizeof(TaskQueueWorkerWine));
    worker_wine->base.queue = queue;
    worker_wine->base.addref = task_queue_worker_wine_addref;
    worker_wine->base.release = task_queue_worker_wine_release;
    worker_wine->base.join = task_queue_worker_wine_join;
    atomic_init(&worker_wine->ref_count, 1);
    HANDLE thr = CreateThread(NULL, 0, worker, worker_wine, 0, NULL);
    if (thr == NULL) {
        free(worker_wine);
        return NULL;
    }
    CloseHandle(thr);
    return (TaskQueueWorker*)worker_wine;
}
