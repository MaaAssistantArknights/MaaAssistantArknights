#include "task_queue.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>

#define MAX_TASKS 16
#define MAX_WORKERS 1

struct _Task
{
    TaskFunc function;
    void* arg;
    pthread_mutex_t mutex;
    pthread_cond_t cond_done;
    int done;
    atomic_int ref_count;
};

struct _TaskQueue
{
    Task* tasks[MAX_TASKS];
    int head;
    int tail;
    int stop;
    pthread_mutex_t mutex;
    pthread_cond_t cond_not_empty;
    pthread_cond_t cond_not_full;
    TaskQueueWorkerFactory worker_factory;
    int worker_count;
    TaskQueueWorker* workers[MAX_WORKERS];
};

static Task* task_new(TaskFunc function, void* arg)
{
    Task* task = calloc(1, sizeof(Task));
    if (!task) {
        return NULL;
    }
    task->function = function;
    task->arg = arg;
    // 失败清理只能销毁已初始化的同步对象，否则是 UB。
    if (pthread_mutex_init(&task->mutex, NULL) != 0) {
        free(task);
        return NULL;
    }
    if (pthread_cond_init(&task->cond_done, NULL) != 0) {
        pthread_mutex_destroy(&task->mutex);
        free(task);
        return NULL;
    }
    atomic_init(&task->ref_count, 1);
    return task;
}

void task_addref(Task* task)
{
    atomic_fetch_add(&task->ref_count, 1);
}

void task_release(Task* task)
{
    if (atomic_fetch_sub(&task->ref_count, 1) == 1) {
        pthread_mutex_destroy(&task->mutex);
        pthread_cond_destroy(&task->cond_done);
        free(task);
    }
}

// 回调返回后，在锁内向等待方发布完成状态。
static void task_complete(Task* task)
{
    pthread_mutex_lock(&task->mutex);
    task->done = 1;
    pthread_cond_signal(&task->cond_done);
    pthread_mutex_unlock(&task->mutex);
}

void task_invoke(Task* task)
{
    if (task->function) {
        task->function(task->arg);
    }
    task_complete(task);
}

void task_wait(Task* task)
{
    pthread_mutex_lock(&task->mutex);
    while (!task->done) {
        pthread_cond_wait(&task->cond_done, &task->mutex);
    }
    pthread_mutex_unlock(&task->mutex);
}

TaskQueue* task_queue_new(TaskQueueWorkerFactory worker_factory)
{
    if (!worker_factory) {
        return NULL;
    }

    TaskQueue* queue = calloc(1, sizeof(TaskQueue));
    if (!queue) {
        return NULL;
    }
    queue->worker_factory = worker_factory;

    // worker 启动后立刻开始消费，同步对象必须先初始化。
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        goto fail_queue;
    }
    if (pthread_cond_init(&queue->cond_not_empty, NULL) != 0) {
        goto fail_mutex;
    }
    if (pthread_cond_init(&queue->cond_not_full, NULL) != 0) {
        goto fail_not_empty;
    }

    TaskQueueWorker* worker = worker_factory(queue);
    if (!worker) {
        goto fail_not_full;
    }
    queue->workers[queue->worker_count++] = worker;
    return queue;

fail_not_full:
    pthread_cond_destroy(&queue->cond_not_full);
fail_not_empty:
    pthread_cond_destroy(&queue->cond_not_empty);
fail_mutex:
    pthread_mutex_destroy(&queue->mutex);
fail_queue:
    free(queue);
    return NULL;
}

void task_queue_free(TaskQueue* queue)
{
    if (!queue) {
        return;
    }

    pthread_mutex_lock(&queue->mutex);
    queue->stop = 1;
    pthread_cond_broadcast(&queue->cond_not_empty);
    pthread_cond_broadcast(&queue->cond_not_full);
    pthread_mutex_unlock(&queue->mutex);

    // 先等待 worker 退出，再销毁其访问的队列同步对象。
    for (int i = 0; i < queue->worker_count; i++) {
        TaskQueueWorker* worker = queue->workers[i];
        if (!worker) {
            continue;
        }
        worker->join(worker);
        worker->release(worker);
        queue->workers[i] = NULL;
    }

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond_not_empty);
    pthread_cond_destroy(&queue->cond_not_full);
    free(queue);
}

Task* task_queue_push(TaskQueue* queue, TaskFunc function, void* arg)
{
    if (!queue) {
        return NULL;
    }

    pthread_mutex_lock(&queue->mutex);
    // TODO: add worker if queue is full
    while (!queue->stop && (queue->tail + 1) % MAX_TASKS == queue->head) {
        pthread_cond_wait(&queue->cond_not_full, &queue->mutex);
    }

    Task* task = NULL;
    if (!queue->stop && (task = task_new(function, arg)) != NULL) {
        // 队列与调用方各持一个引用；先 addref 再入队，避免 worker 先释放最后一个引用。
        task_addref(task);
        queue->tasks[queue->tail] = task;
        queue->tail = (queue->tail + 1) % MAX_TASKS;
        pthread_cond_signal(&queue->cond_not_empty);
    }
    pthread_mutex_unlock(&queue->mutex);
    return task;
}

Task* task_queue_pop(TaskQueue* queue)
{
    if (!queue) {
        return NULL;
    }

    pthread_mutex_lock(&queue->mutex);
    while (queue->head == queue->tail && !queue->stop) {
        pthread_cond_wait(&queue->cond_not_empty, &queue->mutex);
    }
    if (queue->head == queue->tail) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    Task* task = queue->tasks[queue->head];
    queue->head = (queue->head + 1) % MAX_TASKS;

    pthread_cond_signal(&queue->cond_not_full);
    pthread_mutex_unlock(&queue->mutex);

    return task;
}
