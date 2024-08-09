#pragma once
#include <stdint.h>
#include <windef.h>

#define CALLBACK_FORWARDER_MAX_ARGS 4

int __stdcall maa_wine_bridge_main_loop_start();
int __stdcall maa_wine_bridge_main_loop_stop();
void invoke_in_main_loop_stdcall(void* callback, int arg_count, ...);
void invoke_in_main_loop(GSourceFunc callback, void *arg);
