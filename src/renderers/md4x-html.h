/*
 * MD4X: Markdown parser for C
 * (http://github.com/pi0/md4x)
 *
 * Copyright (c) 2016-2024 Martin Mitáš
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

#ifndef MD4X_HTML_H
#define MD4X_HTML_H

#include "md4x.h"

#ifdef __cplusplus
    extern "C" {
#endif


/* If set, debug output from md_parse() is sent to stderr. */
#define MD_HTML_FLAG_DEBUG                  0x0001
#define MD_HTML_FLAG_VERBATIM_ENTITIES      0x0002
#define MD_HTML_FLAG_SKIP_UTF8_BOM          0x0004
#define MD_HTML_FLAG_FULL_HTML              0x0008


/* Options for md_html_ex(). */
typedef struct MD_HTML_OPTS {
    const char* title;      /* Document title override (NULL = use frontmatter) */
    const char* css_url;    /* CSS stylesheet URL (NULL = omit) */
} MD_HTML_OPTS;


/* Render Markdown into HTML body content.
 *
 * Frontmatter blocks are suppressed from output.
 *
 * Returns -1 on error (if md_parse() fails.)
 * Returns 0 on success.
 */
int md_html(const MD_CHAR* input, MD_SIZE input_size,
            void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
            void* userdata, unsigned parser_flags, unsigned renderer_flags);

/* Extended HTML renderer with full-document support.
 *
 * When MD_HTML_FLAG_FULL_HTML is set, generates a complete HTML document.
 * If frontmatter exists, YAML title and description are used in <head>.
 * opts->title overrides the frontmatter title. opts may be NULL.
 */
int md_html_ex(const MD_CHAR* input, MD_SIZE input_size,
               void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
               void* userdata, unsigned parser_flags, unsigned renderer_flags,
               const MD_HTML_OPTS* opts);


#ifdef __cplusplus
    }  /* extern "C" { */
#endif

#endif  /* MD4X_HTML_H */
