/*
 * MD4X: Markdown parser for C
 * (http://github.com/pi0/md4x)
 *
 * Shared heal-before-render wrapper (header-only).
 * Include this in renderer .c files that support MD_*_FLAG_HEAL.
 */

#ifndef MD4X_HEAL_WRAP_H
#define MD4X_HEAL_WRAP_H

#include <stdlib.h>
#include <string.h>
#include "md4x-heal.h"

/* Common heal flag value — all renderers use 0x0100. */
#define MD4X_FLAG_HEAL  0x0100

typedef struct {
    char* data;
    unsigned size;
    unsigned cap;
} MD4X_HEAL_BUF;

static void
md4x_heal_buf_append(const char* text, unsigned size, void* userdata)
{
    MD4X_HEAL_BUF* buf = (MD4X_HEAL_BUF*) userdata;
    if(buf->size + size > buf->cap) {
        unsigned new_cap = buf->cap + buf->cap / 2 + size + 256;
        char* p = (char*) realloc(buf->data, new_cap);
        if(!p) return;
        buf->data = p;
        buf->cap = new_cap;
    }
    memcpy(buf->data + buf->size, text, size);
    buf->size += size;
}

/* Run md_heal and return the healed buffer. Caller must free buf->data.
 * Returns 0 on success, -1 on error. */
static int
md4x_heal_input(const MD_CHAR* input, MD_SIZE input_size, MD4X_HEAL_BUF* buf)
{
    buf->data = NULL;
    buf->size = 0;
    buf->cap = 0;
    return md_heal(input, input_size, md4x_heal_buf_append, buf);
}

#endif  /* MD4X_HEAL_WRAP_H */
