#pragma once

#include "Assistance.h"

#ifdef __cplusplus
extern "C" {
#endif

extern __declspec(dllexport) asst::Assistance* CreateAsst();
extern __declspec(dllexport) void DestoryAsst(asst::Assistance* p_asst);
extern __declspec(dllexport) bool AsstCatchSimulator(asst::Assistance* p_asst);
extern __declspec(dllexport) void AsstStart(asst::Assistance* p_asst, const char* task);
extern __declspec(dllexport) void AsstStop(asst::Assistance* p_asst);
extern __declspec(dllexport) bool AsstSetParam(asst::Assistance* p_asst, const char* type, const char* param, const char* value);

#ifdef __cplusplus
}
#endif