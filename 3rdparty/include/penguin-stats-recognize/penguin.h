#pragma once

#include "port.h"

typedef unsigned char      uint8_t;

#ifdef __cplusplus
extern "C" {
#endif

    PENGUINAPI_PORT const char* PENGUIN_CALL get_info();
    void PENGUINAPI load_server(const char* server);
    void PENGUINAPI load_json(const char* stage_index, const char* hash_index);
    void PENGUINAPI load_templ(const char* itemId, const uint8_t* buffer, size_t size);
    void PENGUINAPI load_templ_with_data(const char* itemId, int rows, int cols, int type, void* data);
    PENGUINAPI_PORT const char* PENGUIN_CALL recognize(const uint8_t* buffer, size_t size);
    PENGUINAPI_PORT const char* PENGUIN_CALL recognize_with_data(int rows, int cols, int type, void* data);

#ifdef __cplusplus
}
#endif