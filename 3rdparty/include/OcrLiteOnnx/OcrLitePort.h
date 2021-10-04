#pragma once

#if !defined(__JNI__) && !defined(__EXEC__)

// The way how the function is called
#if !defined(OCRLITE_CALL)
#if defined(_WIN32)
#define OCRLITE_CALL __stdcall
#else
#define OCRLITE_CALL
#endif /* _WIN32 */
#endif /* OCRLITE_CALL */

#if defined _WIN32 || defined __CYGWIN__
#define OCRLITE_EXPORT __declspec(dllexport)
#define OCRLITE_IMPORT __declspec(dllimport)
#define OCRLITE_LOCAL
#else   // ! defined _WIN32 || defined __CYGWIN__
#if __GNUC__ >= 4
#define OCRLITE_EXPORT __attribute__ ((visibility ("default")))
#define OCRLITE_IMPORT __attribute__ ((visibility ("default")))
#define OCRLITE_LOCAL  __attribute__ ((visibility ("hidden")))
#else   // ! __GNUC__ >= 4
#define OCRLITE_EXPORT
#define OCRLITE_IMPORT
#endif  // End __GNUC__ >= 4
#endif  // End defined _WIN32 || defined __CYGWIN__

#ifdef __CLIB__
#define OCRLITE_PORT OCRLITE_EXPORT
#else
#define OCRLITE_PORT OCRLITE_IMPORT
#endif // OCRLITE_PORT

#define OCR_API OCRLITE_PORT OCRLITE_CALL

#define OCR_LOCAL OCRLITE_LOCAL OCRLITE_CALL

#else // __JNI__ || __EXEC__

#define OCRLITE_PORT
#define OCR_API
#define OCR_LOCAL

#endif // __JNI__ || __EXEC__
