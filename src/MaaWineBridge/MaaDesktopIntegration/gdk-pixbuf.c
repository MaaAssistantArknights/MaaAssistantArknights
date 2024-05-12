#include <gdk-pixbuf/gdk-pixbuf.h>
#include "main_loop.h"

#include <windef.h>

typedef void(__stdcall* GdkPixbufDestroyNotifyMsAbi)(guchar* pixels, gpointer data);

struct PixBufDestroyCallbackState
{
    GdkPixbufDestroyNotifyMsAbi free_func;
    gpointer user_data;
};

static void pixbuf_destroy_callback(guchar* pixels, gpointer data)
{
    struct PixBufDestroyCallbackState* state = (struct PixBufDestroyCallbackState*)data;
    // not guaranteed to be called from the main loop
    invoke_in_main_loop_stdcall(state->free_func, 2, pixels, state->user_data);
    free(state);
}

GdkPixbuf* __cdecl maa_wine_bridge_gdk_pixbuf_new_from_data(
    const guchar* data,
    GdkColorspace colorspace,
    gboolean has_alpha,
    int bits_per_sample,
    int width,
    int height,
    int rowstride,
    GdkPixbufDestroyNotifyMsAbi destroy_fn,
    gpointer destroy_fn_data)
{
    struct PixBufDestroyCallbackState* state = malloc(sizeof(struct PixBufDestroyCallbackState));
    state->free_func = destroy_fn;
    state->user_data = destroy_fn_data;

    return gdk_pixbuf_new_from_data(
        data,
        colorspace,
        has_alpha,
        bits_per_sample,
        width,
        height,
        rowstride,
        pixbuf_destroy_callback,
        state);
}

