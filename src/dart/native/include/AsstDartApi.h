#ifndef ASST_DART_API_H_
#define ASST_DART_API_H_
#include "dart_api.h"
#include "AsstCaller.h"

DART_EXPORT AsstHandle AsstCreateWithDartPort(Dart_Port *dart_port);
DART_EXPORT void InitDartVM(void *data);

#endif