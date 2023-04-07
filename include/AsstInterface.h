#pragma once

#define C_INTERFACE(name, ...)              \
    struct name##_vftable                   \
    {                                       \
        void* ptr;                          \
        void (*DestructorPtr)(void*);       \
    };                                      \
    struct name##_struct                    \
    {                                       \
        struct name##_vftable* vftable_ptr; \
        __VA_ARGS__                         \
    };

#define CXX_INTERFACE(name, ...) \
    struct name                  \
    {                            \
        virtual ~name() = 0;     \
        __VA_ARGS__              \
    };

#ifdef __cplusplus
#define ASST_INTERFACE(name, ...)           \
    CXX_INTERFACE(name, __VA_ARGS__)   \
    extern "C"                         \
    {                                  \
        C_INTERFACE(name, __VA_ARGS__) \
    }
#else
#define ASST_INTERFACE(name, ...) C_INTERFACE(name, __VA_ARGS__)
#endif
