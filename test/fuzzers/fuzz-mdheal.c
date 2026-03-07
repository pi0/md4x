
#include <stdint.h>
#include <stdlib.h>
#include "md4x-heal.h"


static void
process_output(const char* text, unsigned size, void* userdata)
{
   return;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* md_heal has no flags, use all input as markdown */
    if(size == 0) {
        return 0;
    }

    md_heal((const char*)data, (unsigned)size, process_output, NULL);
    return 0;
}
