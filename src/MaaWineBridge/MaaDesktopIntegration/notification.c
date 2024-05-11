#include <libnotify/notify.h>

#include <windef.h>

#include "main_loop.h"

int __cdecl maa_wine_bridge_notify_init(const char* app_name)
{
    return notify_init(app_name);
}

NotifyNotification* __cdecl maa_wine_bridge_notify_notification_new(
    const char* summary,
    const char* body,
    const char* icon)
{
    return notify_notification_new(summary, body, icon);
}

typedef void(__cdecl* NotifyActionCallbackMsAbi)(
    NotifyNotification* notification,
    char* action,
    gpointer user_data);

typedef void(__cdecl* GFreeFuncMsAbi)(gpointer data);

struct NotifyActionCallbackState
{
    NotifyActionCallbackMsAbi x64abi_callback;
    gpointer user_data;
    GFreeFuncMsAbi free_func;
};

static void maa_wine_bridge_notify_action_callback(
    NotifyNotification* notification,
    char* action,
    gpointer user_data)
{
    struct NotifyActionCallbackState* state = (struct NotifyActionCallbackState*)user_data;
    // called from the main loop (wine-aware thread)
    state->x64abi_callback(notification, action, state->user_data);
}

static void unix_free_func(gpointer data)
{
    struct NotifyActionCallbackState* state = (struct NotifyActionCallbackState*)data;
    state->free_func(state->user_data);
    free(state);
}

void __cdecl maa_wine_bridge_notify_notification_add_action(
    NotifyNotification* notification,
    const char* action,
    const char* label,
    NotifyActionCallbackMsAbi callback,
    gpointer user_data,
    GFreeFuncMsAbi free_func)
{
    struct NotifyActionCallbackState* state = malloc(sizeof(struct NotifyActionCallbackState));
    state->x64abi_callback = callback;
    state->user_data = user_data;
    state->free_func = free_func;
    notify_notification_add_action(
        notification,
        action,
        label,
        maa_wine_bridge_notify_action_callback,
        state,
        unix_free_func);
}

void __cdecl maa_wine_bridge_notify_notification_clear_actions(NotifyNotification* notification)
{
    notify_notification_clear_actions(notification);
}

void __cdecl maa_wine_bridge_notify_notification_clear_hints(NotifyNotification* notification)
{
    notify_notification_clear_hints(notification);
}

void __cdecl maa_wine_bridge_notify_notification_close(
    NotifyNotification* notification,
    GError** error)
{
    notify_notification_close(notification, error);
}

const char* __cdecl maa_wine_bridge_notify_notification_get_activation_token(
    NotifyNotification* notification)
{
    return notify_notification_get_activation_token(notification);
}

gint __cdecl maa_wine_bridge_notify_notification_get_closed_reason(
    const NotifyNotification* notification)
{
    return notify_notification_get_closed_reason(notification);
}

void __cdecl maa_wine_bridge_notify_notification_set_app_name(
    NotifyNotification* notification,
    const char* app_name)
{
    notify_notification_set_app_name(notification, app_name);
}

void __cdecl maa_wine_bridge_notify_notification_set_category(
    NotifyNotification* notification,
    const char* category)
{
    notify_notification_set_category(notification, category);
}

void __cdecl maa_wine_bridge_notify_notification_set_hint(
    NotifyNotification* notification,
    const char* key,
    GVariant* value)
{
    notify_notification_set_hint(notification, key, value);
}

void __cdecl maa_wine_bridge_notify_notification_set_image_from_pixbuf(
    NotifyNotification* notification,
    GdkPixbuf* pixbuf)
{
    notify_notification_set_image_from_pixbuf(notification, pixbuf);
}

void __cdecl maa_wine_bridge_notify_notification_set_timeout(
    NotifyNotification* notification,
    gint timeout)
{
    notify_notification_set_timeout(notification, timeout);
}

void __cdecl maa_wine_bridge_notify_notification_set_urgency(
    NotifyNotification* notification,
    NotifyUrgency urgency)
{
    notify_notification_set_urgency(notification, urgency);
}

void __cdecl maa_wine_bridge_notify_notification_show(
    NotifyNotification* notification,
    GError** error)
{
    notify_notification_show(notification, error);
}

gboolean __cdecl maa_wine_bridge_notify_notification_update(
    NotifyNotification* notification,
    const char* summary,
    const char* body,
    const char* icon)
{
    return notify_notification_update(notification, summary, body, icon);
}

