#pragma once

// The way how the function is called
#if !defined(ASST_CALL)
#if defined(_WIN32)
#define ASST_CALL __stdcall
#else
#define ASST_CALL
#endif /* _WIN32 */
#endif /* ASST_CALL */

// The function exported symbols
#if defined _WIN32 || defined __CYGWIN__
#define ASST_DLL_IMPORT __declspec(dllimport)
#define ASST_DLL_EXPORT __declspec(dllexport)
#define ASST_DLL_LOCAL
#else
#if __GNUC__ >= 4
#define ASST_DLL_IMPORT __attribute__((visibility("default")))
#define ASST_DLL_EXPORT __attribute__((visibility("default")))
#define ASST_DLL_LOCAL __attribute__((visibility("hidden")))
#else
#define ASST_DLL_IMPORT
#define ASST_DLL_EXPORT
#define ASST_DLL_LOCAL
#endif
#endif

#ifdef ASST_DLL_EXPORTS // defined if we are building the DLL (instead of using it)
#define ASSTAPI_PORT ASST_DLL_EXPORT
#else
#define ASSTAPI_PORT ASST_DLL_IMPORT
#endif // ASST_DLL_EXPORTS

#define ASSTAPI ASSTAPI_PORT ASST_CALL

#define ASSTLOCAL ASST_DLL_LOCAL ASST_CALL
