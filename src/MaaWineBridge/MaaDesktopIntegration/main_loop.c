#include <glib.h>
#include <stdint.h>
// clang-format off
#include <windef.h>
#include <winbase.h>
// clang-format on
#include "main_loop.h"

typedef struct _pending_call
{
    void* function;
    int arg_count;
    uintptr_t args[CALLBACK_FORWARDER_MAX_ARGS];
    uintptr_t result;
} pending_call;

typedef uintptr_t(__stdcall* Callback0)();
typedef uintptr_t(__stdcall* Callback1)(uintptr_t);
typedef uintptr_t(__stdcall* Callback2)(uintptr_t, uintptr_t);
typedef uintptr_t(__stdcall* Callback3)(uintptr_t, uintptr_t, uintptr_t);
typedef uintptr_t(__stdcall* Callback4)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);

static GMainLoop* main_loop;
static GMainContext* main_context;

static int WINAPI gmainloop_thread(void* arg)
{
    GMainLoop* loop = arg;
    g_main_loop_ref(loop);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    return 0;
}

int __cdecl maa_wine_bridge_glib_default_main_loop_start()
{
    if (main_loop != NULL) {
        return 0;
    }
    main_context = g_main_context_default();
    GMainLoop* loop = g_main_loop_new(main_context, FALSE);
    main_loop = loop;
    HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)gmainloop_thread, loop, 0, NULL);
    return 1;
}

int __cdecl maa_wine_bridge_glib_default_main_loop_stop()
{
    if (!main_loop) {
        return 0;
    }
    GMainLoop* loop = main_loop;
    main_loop = NULL;
    g_main_loop_quit(loop);
    g_main_loop_unref(loop);
    return 1;
}

static gboolean invoke_helper(gpointer lparam)
{
    pending_call* pending = (pending_call*)lparam;
    switch (pending->arg_count) {
    case 0:
        pending->result = ((Callback0)pending->function)();
        break;
    case 1:
        pending->result = ((Callback1)pending->function)(pending->args[0]);
        break;
    case 2:
        pending->result = ((Callback2)pending->function)(pending->args[0], pending->args[1]);
        break;
    case 3:
        pending->result =
            ((Callback3)pending->function)(pending->args[0], pending->args[1], pending->args[2]);
        break;
    case 4:
        pending->result = ((Callback4)pending->function)(
            pending->args[0],
            pending->args[1],
            pending->args[2],
            pending->args[3]);
        break;
    }
    free(pending);
    return FALSE;
}

void invoke_in_main_loop_stdcall(void* callback, int arg_count, ...)
{
    if (arg_count > CALLBACK_FORWARDER_MAX_ARGS) {
        abort();
    }
    pending_call* pending = malloc(sizeof(pending_call));
    pending->function = callback;
    pending->arg_count = arg_count;
    va_list args;
    va_start(args, arg_count);
    for (int i = 0; i < arg_count; i++) {
        pending->args[i] = va_arg(args, uintptr_t);
    }
    va_end(args);
    g_main_context_invoke(main_context, invoke_helper, pending);
}

void invoke_in_main_loop(GSourceFunc callback, void* arg)
{
    g_main_context_invoke(main_context, callback, arg);
}
