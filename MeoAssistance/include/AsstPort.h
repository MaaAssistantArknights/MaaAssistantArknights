#pragma once

// The way how the function is called
#if !defined(MEO_CALL)
#if defined(_WIN32)
#define MEO_CALL __stdcall
#else
#define MEO_CALL
#endif /* _WIN32 */
#endif /* MEO_CALL */

// The function exported symbols
#if defined _WIN32 || defined __CYGWIN__
#define MEO_DLL_IMPORT __declspec(dllimport)
#define MEO_DLL_EXPORT __declspec(dllexport)
#define MEO_DLL_LOCAL
#else
#if __GNUC__ >= 4
#define MEO_DLL_IMPORT __attribute__ ((visibility ("default")))
#define MEO_DLL_EXPORT __attribute__ ((visibility ("default")))
#define MEO_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define MEO_DLL_IMPORT
#define MEO_DLL_EXPORT
#define MEO_DLL_LOCAL
#endif
#endif

#ifdef MEO_DLL_EXPORTS // defined if we are building the DLL (instead of using it)
#define MEOAPI_PORT MEO_DLL_EXPORT
#else
#define MEOAPI_PORT MEO_DLL_IMPORT
#endif // MEO_DLL_EXPORTS

#define MEOAPI MEOAPI_PORT MEO_CALL

#define MEOLOCAL MEO_DLL_LOCAL MEO_CALL