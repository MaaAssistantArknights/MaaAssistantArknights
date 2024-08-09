#include <glib.h>
#include <glib-object.h>

#include "main_loop.h"

#include <windef.h>

void __cdecl maa_wine_bridge_g_object_unref(void* object)
{
    g_object_unref(G_OBJECT(object));
}

typedef void(__cdecl* GClosureNotifyMsAbi)(gpointer data, GClosure* closure);

typedef void(__cdecl* GCallbackMsAbi)(gpointer data, gpointer data2);

typedef struct _shim_g_signal_callback_state
{
    GCallbackMsAbi c_handler;
    gpointer data;
    GClosureNotifyMsAbi destroy_data;
} shim_g_signal_callback_state;

static void shim_g_signal_callback(gpointer sender, gpointer arg)
{
    shim_g_signal_callback_state* state = (shim_g_signal_callback_state*)arg;
    // called from the main loop (wine-aware thread)
    state->c_handler(sender, state->data);
}

static void shim_g_signal_callback_destroy(gpointer data, GClosure* closure)
{
    shim_g_signal_callback_state* state = (shim_g_signal_callback_state*)data;
    // called from the main loop (wine-aware thread)
    state->destroy_data(state->data, closure);
    free(state);
}

gulong __cdecl maa_wine_bridge_g_signal_connect_data(
    void* instance,
    const char* detailed_signal,
    GCallbackMsAbi c_handler,
    gpointer data,
    GClosureNotifyMsAbi destroy_data,
    GConnectFlags connect_flags)
{
    shim_g_signal_callback_state* state = malloc(sizeof(shim_g_signal_callback_state));
    state->c_handler = c_handler;
    state->data = data;
    state->destroy_data = destroy_data;
    return g_signal_connect_data(
        instance,
        detailed_signal,
        G_CALLBACK(shim_g_signal_callback),
        state,
        shim_g_signal_callback_destroy,
        connect_flags);
}
