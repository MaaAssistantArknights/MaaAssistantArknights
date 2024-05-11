#include "dl_maacore.h"

static void lib_init() __attribute__((constructor));

void lib_init() {
    dl_init_maacore();
}
