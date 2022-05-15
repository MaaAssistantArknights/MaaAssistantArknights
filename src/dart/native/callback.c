#include "dart_api_dl.h"
#include <string.h>
#include <stdio.h>
void callback(int, const char *, void *);
void init_dart_api(void *);
void init_dart_api(void *api_data) {
    Dart_InitializeApiDL(api_data);
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
    int required_len = n_digits + strlen(json_str) + 1;
    char retstr[required_len];
    retstr[required_len-1] = 0;
    sprintf(retstr, "{\"retval\": %d, \"payload\": %s }", retval, json_str);
    obj.type = Dart_CObject_kString;
    obj.value.as_string = retstr;
    printf("C: Sending Pointer %ld to dart through port %ld\n", (int64_t)json_str, port);
    char res = Dart_PostCObject_DL(port, &obj);
    if (res == 0) {
        printf("C: Failed to send message to Dart\n");
    }
}