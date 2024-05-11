#pragma once

#ifdef DL_FUNC_IMPL
#define DL_FUNC_DEF
#else
#define DL_FUNC_DEF extern
#endif

#define DEFINE_DL_FUNC(func) DL_FUNC_DEF typeof(func)* dl_##func;

#define FILL_DL_FUNC(lib, func)                       \
    do {                                              \
        dl_##func = (typeof(func)*)dlsym(lib, #func); \
    } while (0);
