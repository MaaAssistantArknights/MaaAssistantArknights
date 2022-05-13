#include "AsstDartApi.h"
#include "AsstCaller.h"
#include "dart_api.h"
#include "dart_api_dl.h"

#include<iostream>
#include<string.h>

DART_EXPORT AsstHandle AsstCreateWithDartPort(Dart_Port *dart_port) 
{
    std::cout << "CPP: Dart_Port Pointer " << dart_port << std::endl;
    std::cout << "CPP: Native Port: " << *dart_port << std::endl;
    AsstApiCallback dart_cb = [](int msg, const char* detail, void* custom_arg) 
    {
        Dart_CObject obj;
        obj.type = Dart_CObject_kString;
        obj.value.as_string = const_cast<char*>(detail);
        Dart_Port port = *(Dart_Port *)custom_arg;
        std::cout << "CPP: Native Port in closure: " << port << std::endl;
        bool res = Dart_PostCObject(port, &obj); 
        if (!res) {
            std::cerr << "CPP: Dart_PostCObject failed" << std::endl;
        } else {
            std::cout << "CPP: Successfully Sent message: " << detail << std::endl;
        }
    };

    return AsstCreateEx(dart_cb, dart_port);
}

DART_EXPORT void InitDartVM(void *data)
{
    std::cout << "CPP: InitDartVM" << std::endl;
    Dart_InitializeParams* params = (Dart_InitializeParams*) data;
    char* res = Dart_Initialize(params);
    if (res) {
        std::cerr << "CPP: Dart_Initialize failed:" << res << std::endl;
    } else {
        std::cout << "CPP: Dart_Initialize success" << std::endl;
    }
    free(res);
}