#include "dart_api_dl.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void callback(int, const char *, void *);
void init_dart_api(void *);

void init_dart_api(void *api_data) {
    Dart_InitializeApiDL(api_data);
}

void native_free(void *ptr) {
    free(ptr);
}

void callback(int retval, const char *json_str, void *custom_arg) {
    Dart_Port port = *(Dart_Port *)custom_arg;

    int n_digits = 1;
    int k = retval;
    while (k > 10) {
        n_digits++;
        k /= 10;
    }
    Dart_CObject obj;
    int required_len = n_digits + strlen(json_str) + 30;
    char* retptr = calloc(required_len, sizeof(char));
    snprintf(retptr, required_len, "{\"retval\":%d,\"payload\":%s}", retval, json_str);
    obj.type = Dart_CObject_kString;
    obj.value.as_int64 = (uintptr_t)retptr;
    printf("C: Sending Pointer %p to dart through port %ld\n", retptr, (int64_t) port);
    char res = Dart_PostCObject_DL(port, &obj);
    if (res == 0) {
        printf("C: Failed to send message to Dart\n");
    }
}
