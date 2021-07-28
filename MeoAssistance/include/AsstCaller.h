#pragma once

#include "Assistance.h"

#ifdef __cplusplus
extern "C" {
#endif

extern MEOAPI_PORT asst::Assistance* CreateAsst();
extern MEOAPI_PORT void DestoryAsst(asst::Assistance* p_asst);
extern MEOAPI_PORT bool AsstCatchEmulator(asst::Assistance* p_asst);
extern MEOAPI_PORT void AsstStart(asst::Assistance* p_asst, const char* task);
extern MEOAPI_PORT void AsstStop(asst::Assistance* p_asst);
extern MEOAPI_PORT bool AsstSetParam(asst::Assistance* p_asst, const char* type, const char* param, const char* value);
extern MEOAPI_PORT bool AsstGetParam(asst::Assistance* p_asst, const char* type, const char* param, char * buffer, int buffer_size);
extern MEOAPI_PORT bool CheckVersionUpdate(char * tag_buffer, int tag_bufsize, char * html_url_buffer, int html_bufsize, char * body_buffer, int body_bufsize);

#ifdef __cplusplus
}
#endif