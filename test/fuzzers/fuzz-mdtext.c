
#include <stdint.h>
#include <stdlib.h>
#include "md4x-text.h"


static void
process_output(const MD_CHAR* text, MD_SIZE size, void* userdata)
{
   return;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    unsigned parser_flags, renderer_flags;

    if(size < 2 * sizeof(unsigned)) {
        return 0;
    }
    parser_flags = ((unsigned*)data)[0];
    renderer_flags = ((unsigned*)data)[1];
    data += 2 * sizeof(unsigned);
    size -= 2 * sizeof(unsigned);

    md_text((const MD_CHAR*)data, size, process_output, NULL, parser_flags, renderer_flags);
    return 0;
}
