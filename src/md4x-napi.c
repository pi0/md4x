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

#include <node_api.h>
#include <stdlib.h>
#include <string.h>
#include "md4x.h"
#include "md4x-html.h"
#include "md4x-ast.h"
#include "md4x-ansi.h"
#include "md4x-meta.h"
#include "md4x-text.h"


/* Growable output buffer */
typedef struct {
    char* data;
    unsigned size;
    unsigned cap;
} napi_buf;

static void napi_buf_append(const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    napi_buf* buf = (napi_buf*) userdata;
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


/* Generic renderer wrapper */
typedef int (*md4x_render_fn)(const MD_CHAR*, MD_SIZE,
    void (*)(const MD_CHAR*, MD_SIZE, void*), void*, unsigned, unsigned);

static napi_value
render_impl(napi_env env, napi_callback_info info, md4x_render_fn fn)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if(argc < 1) {
        napi_throw_error(env, NULL, "Expected 1 argument");
        return NULL;
    }

    /* Get input string length, then read */
    size_t input_size;
    napi_get_value_string_utf8(env, argv[0], NULL, 0, &input_size);
    char* input = (char*) malloc(input_size + 1);
    if(!input) {
        napi_throw_error(env, NULL, "Allocation failed");
        return NULL;
    }
    napi_get_value_string_utf8(env, argv[0], input, input_size + 1, &input_size);

    /* Render with all extensions enabled */
    napi_buf buf = { NULL, 0, 0 };
    int ret = fn(input, (unsigned) input_size, napi_buf_append, &buf,
                 MD_DIALECT_ALL, 0);
    free(input);

    if(ret != 0) {
        free(buf.data);
        napi_throw_error(env, NULL, "Markdown parsing failed");
        return NULL;
    }

    napi_value result;
    napi_create_string_utf8(env, buf.data ? buf.data : "", buf.size, &result);
    free(buf.data);
    return result;
}


/* Exported functions */

static napi_value md4x_napi_to_html(napi_env env, napi_callback_info info)
{
    return render_impl(env, info, md_html);
}

static napi_value md4x_napi_to_ast(napi_env env, napi_callback_info info)
{
    return render_impl(env, info, md_ast);
}

static napi_value md4x_napi_to_ansi(napi_env env, napi_callback_info info)
{
    return render_impl(env, info, md_ansi);
}

static napi_value md4x_napi_to_meta(napi_env env, napi_callback_info info)
{
    return render_impl(env, info, md_meta);
}

static napi_value md4x_napi_to_text(napi_env env, napi_callback_info info)
{
    return render_impl(env, info, md_text);
}


/* Module initialization */
static napi_value init(napi_env env, napi_value exports)
{
    napi_property_descriptor props[] = {
        { "renderToHtml", NULL, md4x_napi_to_html, NULL, NULL, NULL, napi_default, NULL },
        { "renderToAST", NULL, md4x_napi_to_ast, NULL, NULL, NULL, napi_default, NULL },
        { "renderToAnsi", NULL, md4x_napi_to_ansi, NULL, NULL, NULL, napi_default, NULL },
        { "renderToMeta", NULL, md4x_napi_to_meta, NULL, NULL, NULL, napi_default, NULL },
        { "renderToText", NULL, md4x_napi_to_text, NULL, NULL, NULL, napi_default, NULL },
    };
    napi_define_properties(env, exports, 5, props);
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
