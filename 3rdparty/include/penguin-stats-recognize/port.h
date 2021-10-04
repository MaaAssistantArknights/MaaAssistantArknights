#pragma once

// The way how the function is called
#if !defined(PENGUIN_CALL)
#if defined(_WIN32)
#define PENGUIN_CALL __stdcall
#else
#define PENGUIN_CALL
#endif /* _WIN32 */
#endif /* PENGUIN_CALL */

// The function exported symbols
#if defined _WIN32 || defined __CYGWIN__
#define PENGUIN_DLL_IMPORT __declspec(dllimport)
#define PENGUIN_DLL_EXPORT __declspec(dllexport)
#define PENGUIN_DLL_LOCAL
#else
#if __GNUC__ >= 4
#define PENGUIN_DLL_IMPORT __attribute__ ((visibility ("default")))
#define PENGUIN_DLL_EXPORT __attribute__ ((visibility ("default")))
#define PENGUIN_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define PENGUIN_DLL_IMPORT
#define PENGUIN_DLL_EXPORT
#define PENGUIN_DLL_LOCAL
#endif
#endif

#ifdef PENGUIN_DLL_EXPORTS // defined if we are building the DLL (instead of using it)
#define PENGUINAPI_PORT PENGUIN_DLL_EXPORT
#else
#define PENGUINAPI_PORT PENGUIN_DLL_IMPORT
#endif // PENGUIN_DLL_EXPORTS

#define PENGUINAPI PENGUINAPI_PORT PENGUIN_CALL

#define PENGUINLOCAL PENGUIN_DLL_LOCAL PENGUIN_CALL