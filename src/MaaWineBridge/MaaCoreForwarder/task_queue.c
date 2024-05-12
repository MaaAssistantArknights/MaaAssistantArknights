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
    task->function = function;
    task->arg = arg;
    pthread_mutex_init(&task->mutex, NULL);
    pthread_cond_init(&task->cond_done, NULL);
    task->done = 0;
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

void task_invoke(Task* task)
{
    pthread_mutex_lock(&task->mutex);
    task->function(task->arg);
    task->done = 1;
    pthread_cond_signal(&task->cond_done);
    pthread_mutex_unlock(&task->mutex);
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
    TaskQueue* queue = calloc(1, sizeof(TaskQueue));
    queue->head = 0;
    queue->tail = 0;
    queue->stop = 0;
    queue->worker_factory = worker_factory;
    TaskQueueWorker* worker = worker_factory(queue);
    if (!worker) {
        free(queue);
        return NULL;
    }
    queue->workers[queue->worker_count++] = worker;

    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond_not_empty, NULL);
    pthread_cond_init(&queue->cond_not_full, NULL);
    return queue;
}

void task_queue_free(TaskQueue* queue)
{
    pthread_mutex_lock(&queue->mutex);
    queue->stop = 1;
    pthread_cond_broadcast(&queue->cond_not_empty);
    pthread_mutex_unlock(&queue->mutex);

    for (int i = 0; i < queue->worker_count; i++) {
        TaskQueueWorker* worker = queue->workers[i];
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
    if (queue->stop) {
        return NULL;
    }
    pthread_mutex_lock(&queue->mutex);
    // TODO: add worker if queue is full
    while ((queue->tail + 1) % MAX_TASKS == queue->head) {
        pthread_cond_wait(&queue->cond_not_full, &queue->mutex);
    }
    Task* task = task_new(function, arg);

    queue->tasks[queue->tail] = task;
    queue->tail = (queue->tail + 1) % MAX_TASKS;

    pthread_cond_signal(&queue->cond_not_empty);
    pthread_mutex_unlock(&queue->mutex);
    task_addref(task);
    // 1 reference for the queue, 1 for the caller
    return task;
}

Task* task_queue_pop(TaskQueue* queue)
{
    if (queue->stop) {
        return NULL;
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->head == queue->tail && !queue->stop) {
        pthread_cond_wait(&queue->cond_not_empty, &queue->mutex);
    }

    if (queue->stop) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    Task* task = queue->tasks[queue->head];
    queue->head = (queue->head + 1) % MAX_TASKS;

    pthread_cond_signal(&queue->cond_not_full);
    pthread_mutex_unlock(&queue->mutex);

    return task;
}
