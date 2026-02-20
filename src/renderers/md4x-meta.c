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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md4x-meta.h"
#include "md4x-json.h"
#include "entity.h"


/*****************************
 ***  Internal data types  ***
 *****************************/

typedef struct {
    unsigned level;
    char* text;
    MD_SIZE text_size;
} META_HEADING;

typedef struct {
    /* Frontmatter raw text. */
    char* fm_text;
    MD_SIZE fm_size;
    MD_SIZE fm_cap;
    int in_frontmatter;

    /* Headings array. */
    META_HEADING* headings;
    int heading_count;
    int heading_cap;

    /* Current heading accumulator. */
    int in_heading;
    unsigned heading_level;
    char* heading_buf;
    MD_SIZE heading_buf_size;
    MD_SIZE heading_buf_cap;

    int error;
} META_CTX;


/**********************************
 ***  Text accumulation helpers ***
 **********************************/

static int
meta_buf_append(char** buf, MD_SIZE* size, MD_SIZE* cap,
                const char* text, MD_SIZE text_size)
{
    if(*size + text_size > *cap) {
        MD_SIZE new_cap = *cap + *cap / 2 + text_size + 64;
        char* p = (char*) realloc(*buf, new_cap);
        if(p == NULL) return -1;
        *buf = p;
        *cap = new_cap;
    }
    memcpy(*buf + *size, text, text_size);
    *size += text_size;
    return 0;
}

static unsigned
hex_val(char ch)
{
    if(ch >= '0' && ch <= '9') return ch - '0';
    if(ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if(ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return 0;
}

/* Encode a Unicode codepoint as UTF-8 into a buffer. Returns bytes written. */
static MD_SIZE
encode_utf8(unsigned codepoint, char* out)
{
    if(codepoint <= 0x7f) {
        out[0] = (char) codepoint;
        return 1;
    } else if(codepoint <= 0x7ff) {
        out[0] = (char)(0xc0 | ((codepoint >> 6) & 0x1f));
        out[1] = (char)(0x80 | (codepoint & 0x3f));
        return 2;
    } else if(codepoint <= 0xffff) {
        out[0] = (char)(0xe0 | ((codepoint >> 12) & 0xf));
        out[1] = (char)(0x80 | ((codepoint >> 6) & 0x3f));
        out[2] = (char)(0x80 | (codepoint & 0x3f));
        return 3;
    } else if(codepoint <= 0x10ffff) {
        out[0] = (char)(0xf0 | ((codepoint >> 18) & 0x7));
        out[1] = (char)(0x80 | ((codepoint >> 12) & 0x3f));
        out[2] = (char)(0x80 | ((codepoint >> 6) & 0x3f));
        out[3] = (char)(0x80 | (codepoint & 0x3f));
        return 4;
    }
    /* U+FFFD replacement character */
    out[0] = (char)0xef;
    out[1] = (char)0xbf;
    out[2] = (char)0xbd;
    return 3;
}

/* Resolve an HTML entity to UTF-8 and append to the heading buffer. */
static int
meta_append_entity(META_CTX* ctx, const MD_CHAR* text, MD_SIZE size)
{
    char utf8[8];
    MD_SIZE n;

    if(size > 3 && text[1] == '#') {
        unsigned codepoint = 0;
        if(text[2] == 'x' || text[2] == 'X') {
            MD_SIZE i;
            for(i = 3; i < size - 1; i++)
                codepoint = 16 * codepoint + hex_val(text[i]);
        } else {
            MD_SIZE i;
            for(i = 2; i < size - 1; i++)
                codepoint = 10 * codepoint + (unsigned)(text[i] - '0');
        }
        n = encode_utf8(codepoint, utf8);
        return meta_buf_append(&ctx->heading_buf, &ctx->heading_buf_size,
                               &ctx->heading_buf_cap, utf8, n);
    } else {
        const ENTITY* ent = entity_lookup(text, size);
        if(ent != NULL) {
            n = encode_utf8(ent->codepoints[0], utf8);
            if(meta_buf_append(&ctx->heading_buf, &ctx->heading_buf_size,
                               &ctx->heading_buf_cap, utf8, n) != 0)
                return -1;
            if(ent->codepoints[1]) {
                n = encode_utf8(ent->codepoints[1], utf8);
                return meta_buf_append(&ctx->heading_buf, &ctx->heading_buf_size,
                                       &ctx->heading_buf_cap, utf8, n);
            }
            return 0;
        }
    }

    /* Unknown entity: pass through as-is. */
    return meta_buf_append(&ctx->heading_buf, &ctx->heading_buf_size,
                           &ctx->heading_buf_cap, text, size);
}


/***********************************
 ***  md_parse() callbacks       ***
 ***********************************/

static int
meta_enter_block(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    META_CTX* ctx = (META_CTX*) userdata;

    if(type == MD_BLOCK_FRONTMATTER) {
        ctx->in_frontmatter = 1;
    } else if(type == MD_BLOCK_H) {
        const MD_BLOCK_H_DETAIL* d = (const MD_BLOCK_H_DETAIL*) detail;
        ctx->in_heading = 1;
        ctx->heading_level = d->level;
        ctx->heading_buf_size = 0;
    }

    return 0;
}

static int
meta_leave_block(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    META_CTX* ctx = (META_CTX*) userdata;

    (void) detail;

    if(type == MD_BLOCK_FRONTMATTER) {
        ctx->in_frontmatter = 0;
    } else if(type == MD_BLOCK_H) {
        /* Store the completed heading. */
        if(ctx->heading_count >= ctx->heading_cap) {
            int new_cap = ctx->heading_cap == 0 ? 8 : ctx->heading_cap * 2;
            META_HEADING* p = (META_HEADING*) realloc(ctx->headings,
                                 (size_t)new_cap * sizeof(META_HEADING));
            if(p == NULL) { ctx->error = 1; return -1; }
            ctx->headings = p;
            ctx->heading_cap = new_cap;
        }

        ctx->headings[ctx->heading_count].level = ctx->heading_level;

        /* Copy accumulated text. */
        if(ctx->heading_buf_size > 0) {
            char* text = (char*) malloc(ctx->heading_buf_size + 1);
            if(text == NULL) { ctx->error = 1; return -1; }
            memcpy(text, ctx->heading_buf, ctx->heading_buf_size);
            text[ctx->heading_buf_size] = '\0';
            ctx->headings[ctx->heading_count].text = text;
            ctx->headings[ctx->heading_count].text_size = ctx->heading_buf_size;
        } else {
            ctx->headings[ctx->heading_count].text = NULL;
            ctx->headings[ctx->heading_count].text_size = 0;
        }

        ctx->heading_count++;
        ctx->in_heading = 0;
    }

    return 0;
}

static int
meta_enter_span(MD_SPANTYPE type, void* detail, void* userdata)
{
    (void) type;
    (void) detail;
    (void) userdata;
    return 0;
}

static int
meta_leave_span(MD_SPANTYPE type, void* detail, void* userdata)
{
    (void) type;
    (void) detail;
    (void) userdata;
    return 0;
}

static int
meta_text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    META_CTX* ctx = (META_CTX*) userdata;

    if(ctx->in_frontmatter) {
        if(meta_buf_append(&ctx->fm_text, &ctx->fm_size, &ctx->fm_cap,
                           text, size) != 0)
            { ctx->error = 1; return -1; }
        return 0;
    }

    if(ctx->in_heading) {
        switch(type) {
            case MD_TEXT_SOFTBR:
            case MD_TEXT_BR:
                if(meta_buf_append(&ctx->heading_buf, &ctx->heading_buf_size,
                                   &ctx->heading_buf_cap, " ", 1) != 0)
                    { ctx->error = 1; return -1; }
                break;

            case MD_TEXT_NULLCHAR: {
                char buf[3] = { (char)0xEF, (char)0xBF, (char)0xBD };
                if(meta_buf_append(&ctx->heading_buf, &ctx->heading_buf_size,
                                   &ctx->heading_buf_cap, buf, 3) != 0)
                    { ctx->error = 1; return -1; }
                break;
            }

            case MD_TEXT_ENTITY:
                if(meta_append_entity(ctx, text, size) != 0)
                    { ctx->error = 1; return -1; }
                break;

            default:
                if(meta_buf_append(&ctx->heading_buf, &ctx->heading_buf_size,
                                   &ctx->heading_buf_cap, text, size) != 0)
                    { ctx->error = 1; return -1; }
                break;
        }
    }

    return 0;
}

static void
meta_debug_log(const char* msg, void* userdata)
{
    (void) userdata;
    fprintf(stderr, "MD4X: %s\n", msg);
}


/**************************************
 ***  JSON output                   ***
 **************************************/

static void
meta_serialize(JSON_WRITER* w, META_CTX* ctx)
{
    int has_prop = 0;
    int i;

    json_write(w, "{", 1);

    /* Write frontmatter YAML as top-level JSON props. */
    if(ctx->fm_text != NULL && ctx->fm_size > 0) {
        has_prop = json_write_yaml_props(w, ctx->fm_text, ctx->fm_size) > 0;
    }

    /* Write headings array. */
    if(has_prop) json_write(w, ",", 1);
    json_write_str(w, "\"headings\":[");

    for(i = 0; i < ctx->heading_count; i++) {
        char buf[16];

        if(i > 0) json_write(w, ",", 1);

        json_write_str(w, "{\"level\":");
        json_snprintf(buf, sizeof(buf), "%u", ctx->headings[i].level);
        json_write_str(w, buf);

        json_write_str(w, ",\"text\":");
        if(ctx->headings[i].text != NULL) {
            json_write_string(w, ctx->headings[i].text,
                              ctx->headings[i].text_size);
        } else {
            json_write_str(w, "\"\"");
        }

        json_write(w, "}", 1);
    }

    json_write_str(w, "]}\n");
}

static void
meta_free(META_CTX* ctx)
{
    int i;

    free(ctx->fm_text);
    free(ctx->heading_buf);

    for(i = 0; i < ctx->heading_count; i++)
        free(ctx->headings[i].text);
    free(ctx->headings);
}


/**************************************
 ***  Public API                    ***
 **************************************/

int
md_meta(const MD_CHAR* input, MD_SIZE input_size,
        void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
        void* userdata, unsigned parser_flags, unsigned renderer_flags)
{
    META_CTX ctx;
    JSON_WRITER writer;
    int ret;

    MD_PARSER parser = {
        0,
        parser_flags,
        meta_enter_block,
        meta_leave_block,
        meta_enter_span,
        meta_leave_span,
        meta_text,
        (renderer_flags & MD_META_FLAG_DEBUG) ? meta_debug_log : NULL,
        NULL
    };

    memset(&ctx, 0, sizeof(ctx));

#ifndef MD4X_USE_ASCII
    /* Skip UTF-8 BOM. */
    if(renderer_flags & MD_META_FLAG_SKIP_UTF8_BOM) {
        if(input_size >= 3 && (unsigned char)input[0] == 0xEF
           && (unsigned char)input[1] == 0xBB && (unsigned char)input[2] == 0xBF) {
            input += 3;
            input_size -= 3;
        }
    }
#endif

    ret = md_parse(input, input_size, &parser, (void*) &ctx);

    if(ret != 0 || ctx.error != 0) {
        meta_free(&ctx);
        return -1;
    }

    /* Serialize metadata to JSON via the output callback. */
    writer.process_output = process_output;
    writer.userdata = userdata;
    meta_serialize(&writer, &ctx);

    meta_free(&ctx);
    return 0;
}
