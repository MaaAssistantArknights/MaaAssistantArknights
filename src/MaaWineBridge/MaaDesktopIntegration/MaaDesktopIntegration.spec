@ cdecl g_object_unref(ptr) maa_wine_bridge_g_object_unref
@ cdecl g_signal_connect_data(ptr str ptr ptr ptr long) maa_wine_bridge_g_signal_connect_data

@ cdecl notify_init(str) maa_wine_bridge_notify_init
@ cdecl notify_notification_new(str str str) maa_wine_bridge_notify_notification_new
@ cdecl notify_notification_add_action(ptr str str ptr ptr ptr) maa_wine_bridge_notify_notification_add_action
@ cdecl notify_notification_clear_actions(ptr) maa_wine_bridge_notify_notification_clear_actions
@ cdecl notify_notification_clear_hints(ptr) maa_wine_bridge_notify_notification_clear_hints
@ cdecl notify_notification_close(ptr ptr) maa_wine_bridge_notify_notification_close
@ cdecl notify_notification_get_activation_token(ptr) maa_wine_bridge_notify_notification_get_activation_token
@ cdecl notify_notification_get_closed_reason(ptr) maa_wine_bridge_notify_notification_get_closed_reason
@ cdecl notify_notification_set_app_name(ptr str) maa_wine_bridge_notify_notification_set_app_name
@ cdecl notify_notification_set_category(ptr str) maa_wine_bridge_notify_notification_set_category
@ cdecl notify_notification_set_hint(ptr str ptr) maa_wine_bridge_notify_notification_set_hint
@ cdecl notify_notification_set_image_from_pixbuf(ptr ptr) maa_wine_bridge_notify_notification_set_image_from_pixbuf
@ cdecl notify_notification_set_timeout(ptr long) maa_wine_bridge_notify_notification_set_timeout
@ cdecl notify_notification_set_urgency(ptr long) maa_wine_bridge_notify_notification_set_urgency
@ cdecl notify_notification_show(ptr ptr) maa_wine_bridge_notify_notification_show
@ cdecl notify_notification_update(ptr str str str) maa_wine_bridge_notify_notification_update

@ cdecl gdk_pixbuf_new_from_data(ptr long long long long long long ptr ptr) maa_wine_bridge_gdk_pixbuf_new_from_data

@ stdcall glib_default_main_loop_start() maa_wine_bridge_glib_default_main_loop_start
@ stdcall glib_default_main_loop_stop() maa_wine_bridge_glib_default_main_loop_stop

@ cdecl FcInitLoadConfigAndFonts() MaaFcInitLoadConfigAndFonts
@ cdecl FcNameParse(str) MaaFcNameParse
@ cdecl FcConfigSubstitute(ptr ptr long) MaaFcConfigSubstitute
@ cdecl FcDefaultSubstitute(ptr) MaaFcDefaultSubstitute
@ cdecl FcFontSort(ptr ptr long ptr ptr) MaaFcFontSort
@ cdecl FcCharSetCreate() MaaFcCharSetCreate
@ cdecl FcPatternGetString(ptr str long ptr) MaaFcPatternGetString
@ cdecl FcPatternGetCharSet(ptr str long ptr) MaaFcPatternGetCharSet
@ cdecl FcCharSetSubtract(ptr ptr) MaaFcCharSetSubtract
@ cdecl FcCharSetMerge(ptr ptr ptr) MaaFcCharSetMerge
@ cdecl FcCharSetFirstPage(ptr ptr ptr) MaaFcCharSetFirstPage
@ cdecl FcCharSetNextPage(ptr ptr ptr) MaaFcCharSetNextPage
@ cdecl FcCharSetDestroy(ptr) MaaFcCharSetDestroy
@ cdecl FcFontSetDestroy(ptr) MaaFcFontSetDestroy
@ cdecl FcPatternDestroy(ptr) MaaFcPatternDestroy
@ cdecl FcConfigDestroy(ptr) MaaFcConfigDestroy
@ cdecl MaaFcCharsetMapSize()
