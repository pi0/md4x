/*
 * MD4X: Markdown parser for C
 * (http://github.com/pi0/md4x)
 *
 * Copyright (c) 2026 Pooya Parsa <pooya@pi0.io>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "md4x.h"
#include "md4x-html.h"
#include "md4x-ast.h"
#include "md4x-ansi.h"
#include "md4x-meta.h"
#include "md4x-text.h"


/* Stub main for wasi libc (we are a library, not a program) */
int main(void) { return 0; }

/* Result storage (global — WASM is single-threaded) */
static char* g_result_data = NULL;
static unsigned g_result_size = 0;

/* Growable output buffer */
typedef struct {
    char* data;
    unsigned size;
    unsigned cap;
} md4x_buf;

static void buf_append(const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    md4x_buf* buf = (md4x_buf*) userdata;
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


/* Memory management exports */

__attribute__((export_name("md4x_alloc")))
void* md4x_alloc(unsigned size)
{
    return malloc(size);
}

__attribute__((export_name("md4x_free")))
void md4x_free(void* ptr)
{
    free(ptr);
}


/* Result accessors */

__attribute__((export_name("md4x_result_ptr")))
unsigned md4x_result_ptr(void)
{
    return (unsigned)(size_t) g_result_data;
}

__attribute__((export_name("md4x_result_size")))
unsigned md4x_result_size(void)
{
    return g_result_size;
}


/* Renderer wrappers */

typedef int (*md4x_render_fn)(const MD_CHAR*, MD_SIZE,
    void (*)(const MD_CHAR*, MD_SIZE, void*), void*, unsigned, unsigned);

static int render(md4x_render_fn fn, const char* input, unsigned input_size)
{
    md4x_buf buf = { NULL, 0, 0 };
    int ret = fn(input, input_size, buf_append, &buf, MD_DIALECT_ALL, 0);
    if(ret != 0) {
        free(buf.data);
        g_result_data = NULL;
        g_result_size = 0;
        return -1;
    }
    g_result_data = buf.data;
    g_result_size = buf.size;
    return 0;
}

__attribute__((export_name("md4x_to_html")))
int md4x_to_html(const char* input, unsigned input_size,
                 unsigned renderer_flags)
{
    md4x_buf buf = { NULL, 0, 0 };
    int ret = md_html(input, input_size, buf_append, &buf,
                      MD_DIALECT_ALL, renderer_flags);
    if(ret != 0) {
        free(buf.data);
        g_result_data = NULL;
        g_result_size = 0;
        return -1;
    }
    g_result_data = buf.data;
    g_result_size = buf.size;
    return 0;
}

__attribute__((export_name("md4x_to_ast")))
int md4x_to_ast(const char* input, unsigned input_size)
{
    return render(md_ast, input, input_size);
}

__attribute__((export_name("md4x_to_ansi")))
int md4x_to_ansi(const char* input, unsigned input_size)
{
    return render(md_ansi, input, input_size);
}

__attribute__((export_name("md4x_to_meta")))
int md4x_to_meta(const char* input, unsigned input_size)
{
    return render(md_meta, input, input_size);
}

__attribute__((export_name("md4x_to_text")))
int md4x_to_text(const char* input, unsigned input_size)
{
    return render(md_text, input, input_size);
}
