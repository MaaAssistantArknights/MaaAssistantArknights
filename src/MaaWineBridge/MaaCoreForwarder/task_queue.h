#pragma once

typedef struct _Task Task;
typedef struct _TaskQueue TaskQueue;
typedef struct _TaskQueueWorker TaskQueueWorker;

struct _TaskQueueWorker
{
    TaskQueue* queue;
    void (*addref)(TaskQueueWorker*);
    void (*release)(TaskQueueWorker*);
    void (*join)(TaskQueueWorker*);
};

typedef void (*TaskFunc)(void*);
typedef TaskQueueWorker* (*TaskQueueWorkerFactory)(TaskQueue*);

TaskQueue* task_queue_new(TaskQueueWorkerFactory worker_factory);
void task_queue_free(TaskQueue* queue);
Task* task_queue_push(TaskQueue* queue, TaskFunc function, void* arg);
Task* task_queue_pop(TaskQueue* queue);

void thread_pool_worker(TaskQueue* queue);

void task_wait(Task* task);
void task_invoke(Task* task);
void task_addref(Task* task);
void task_release(Task* task);
