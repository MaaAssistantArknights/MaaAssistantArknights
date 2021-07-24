#pragma once

#include "Assistance.h"

#ifdef __cplusplus
extern "C" {
#endif

extern __declspec(dllexport) asst::Assistance* CreateAsst();
extern __declspec(dllexport) void DestoryAsst(asst::Assistance* p_asst);
extern __declspec(dllexport) bool AsstCatchEmulator(asst::Assistance* p_asst);
extern __declspec(dllexport) void AsstStart(asst::Assistance* p_asst, const char* task);
extern __declspec(dllexport) void AsstStop(asst::Assistance* p_asst);
extern __declspec(dllexport) bool AsstSetParam(asst::Assistance* p_asst, const char* type, const char* param, const char* value);
extern __declspec(dllexport) bool AsstGetParam(asst::Assistance* p_asst, const char* type, const char* param, char * buffer, int buffer_size);
extern __declspec(dllexport) bool CheckVersionUpdate(char * tag_buffer, int tag_bufsize, char * html_url_buffer, int html_bufsize, char * body_buffer, int body_bufsize);

#ifdef __cplusplus
}
#endif