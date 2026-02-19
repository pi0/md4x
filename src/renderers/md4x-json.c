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

#include "md4x-json.h"
#include "md4x-props.h"


#ifdef _WIN32
    #define json_snprintf _snprintf
#else
    #define json_snprintf snprintf
#endif

#define JSON_MAX_DEPTH  256


/*************************************
 ***  JSON AST node data structs ***
 *************************************/

typedef enum {
    JSON_NODE_DOCUMENT,
    JSON_NODE_ELEMENT,
    JSON_NODE_TEXT
} JSON_NODE_KIND;

typedef struct JSON_NODE JSON_NODE;
struct JSON_NODE {
    JSON_NODE_KIND kind;
    const char* tag;

    JSON_NODE* first_child;
    JSON_NODE* last_child;
    JSON_NODE* next_sibling;

    /* Text value for text nodes, or literal content for leaf containers
     * (code_block, html_block, inline code). */
    char* text_value;
    MD_SIZE text_size;

    union {
        struct { int is_tight; unsigned start; char delimiter; } ol;
        struct { int is_tight; } ul;
        struct { int is_task; char task_mark; } li;
        struct { char* info; char* lang; char fence_char; char* filename; char* meta; unsigned* highlights; unsigned highlight_count; } code;
        struct { unsigned col_count; } table;
        struct { int align; } td;
        struct { char* href; char* title; } a;
        struct { char* src; char* title; } img;
        struct { char* target; } wikilink;
        struct { char* raw_props; MD_SIZE raw_props_size; } component;
        struct { char* name; } tmpl;
    } detail;

    /* Non-zero if the tag is heap-allocated (dynamic component tag). */
    int tag_is_dynamic;

    /* Raw inline attributes string from trailing {attrs}, or NULL.
     * Heap-allocated copy. Used for em, strong, code, del, u, a, img, span. */
    char* raw_attrs;
    MD_SIZE raw_attrs_size;
};

typedef struct {
    JSON_NODE* root;
    JSON_NODE* current;
    JSON_NODE* stack[JSON_MAX_DEPTH];
    int stack_depth;
    int image_nesting;
    int error;
} JSON_CTX;

/* Writer that streams output through a callback. */
typedef struct {
    void (*process_output)(const MD_CHAR*, MD_SIZE, void*);
    void* userdata;
} JSON_WRITER;


/*****************************
 ***  Memory management    ***
 *****************************/

static JSON_NODE*
json_node_new(const char* tag, JSON_NODE_KIND kind)
{
    JSON_NODE* node = (JSON_NODE*) calloc(1, sizeof(JSON_NODE));
    if(node == NULL)
        return NULL;
    node->tag = tag;
    node->kind = kind;
    return node;
}

static void
json_node_free(JSON_NODE* node)
{
    JSON_NODE* child;
    JSON_NODE* next;

    if(node == NULL)
        return;

    for(child = node->first_child; child != NULL; child = next) {
        next = child->next_sibling;
        json_node_free(child);
    }

    if(node->text_value)
        free(node->text_value);

    /* Free heap-allocated detail strings. */
    if(node->tag != NULL) {
        if(strcmp(node->tag, "pre") == 0) {
            if(node->detail.code.info) free(node->detail.code.info);
            if(node->detail.code.lang) free(node->detail.code.lang);
            if(node->detail.code.filename) free(node->detail.code.filename);
            if(node->detail.code.meta) free(node->detail.code.meta);
            if(node->detail.code.highlights) free(node->detail.code.highlights);
        } else if(strcmp(node->tag, "a") == 0) {
            if(node->detail.a.href) free(node->detail.a.href);
            if(node->detail.a.title) free(node->detail.a.title);
        } else if(strcmp(node->tag, "img") == 0) {
            if(node->detail.img.src) free(node->detail.img.src);
            if(node->detail.img.title) free(node->detail.img.title);
        } else if(strcmp(node->tag, "wikilink") == 0) {
            if(node->detail.wikilink.target) free(node->detail.wikilink.target);
        } else if(strcmp(node->tag, "template") == 0) {
            if(node->detail.tmpl.name) free(node->detail.tmpl.name);
        }
        if(node->tag_is_dynamic) {
            if(node->detail.component.raw_props) free(node->detail.component.raw_props);
            free((char*)node->tag);
        }
    }

    if(node->raw_attrs)
        free(node->raw_attrs);

    free(node);
}

static char*
json_attr_to_str(const MD_ATTRIBUTE* attr)
{
    char* str;

    if(attr->text == NULL)
        return NULL;

    str = (char*) malloc(attr->size + 1);
    if(str == NULL)
        return NULL;

    memcpy(str, attr->text, attr->size);
    str[attr->size] = '\0';
    return str;
}


/***********************************
 ***  AST tree building helpers  ***
 ***********************************/

static void
json_append_child(JSON_CTX* ctx, JSON_NODE* child)
{
    if(ctx->current->first_child == NULL) {
        ctx->current->first_child = child;
        ctx->current->last_child = child;
    } else {
        ctx->current->last_child->next_sibling = child;
        ctx->current->last_child = child;
    }
}

static void
json_push(JSON_CTX* ctx, JSON_NODE* node)
{
    if(ctx->stack_depth >= JSON_MAX_DEPTH) {
        ctx->error = 1;
        return;
    }
    ctx->stack[ctx->stack_depth++] = ctx->current;
    ctx->current = node;
}

static void
json_pop(JSON_CTX* ctx)
{
    if(ctx->stack_depth > 0)
        ctx->current = ctx->stack[--ctx->stack_depth];
}

/* Append text to a node's text_value buffer. */
static int
json_append_text(JSON_NODE* node, const char* src, MD_SIZE src_size)
{
    if(node->text_value == NULL) {
        node->text_value = (char*) malloc(src_size + 1);
        if(node->text_value == NULL) return -1;
        memcpy(node->text_value, src, src_size);
        node->text_value[src_size] = '\0';
        node->text_size = src_size;
    } else {
        char* merged = (char*) realloc(node->text_value, node->text_size + src_size + 1);
        if(merged == NULL) return -1;
        memcpy(merged + node->text_size, src, src_size);
        node->text_size += src_size;
        merged[node->text_size] = '\0';
        node->text_value = merged;
    }
    return 0;
}


/***********************************
 ***  md_parse() callbacks       ***
 ***********************************/

static const char* heading_tags[] = {
    "h0", "h1", "h2", "h3", "h4", "h5", "h6"
};

static int
json_enter_block(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    JSON_CTX* ctx = (JSON_CTX*) userdata;
    JSON_NODE* node;
    const char* tag;

    switch(type) {
        case MD_BLOCK_DOC:          tag = NULL; break;  /* document root */
        case MD_BLOCK_QUOTE:        tag = "blockquote"; break;
        case MD_BLOCK_UL:           tag = "ul"; break;
        case MD_BLOCK_OL:           tag = "ol"; break;
        case MD_BLOCK_LI:           tag = "li"; break;
        case MD_BLOCK_HR:           tag = "hr"; break;
        case MD_BLOCK_H: {
            const MD_BLOCK_H_DETAIL* d = (const MD_BLOCK_H_DETAIL*) detail;
            tag = (d->level >= 1 && d->level <= 6) ? heading_tags[d->level] : "h1";
            break;
        }
        case MD_BLOCK_CODE:         tag = "pre"; break;
        case MD_BLOCK_HTML:         tag = "html_block"; break;
        case MD_BLOCK_P:            tag = "p"; break;
        case MD_BLOCK_TABLE:        tag = "table"; break;
        case MD_BLOCK_THEAD:        tag = "thead"; break;
        case MD_BLOCK_TBODY:        tag = "tbody"; break;
        case MD_BLOCK_TR:           tag = "tr"; break;
        case MD_BLOCK_TH:           tag = "th"; break;
        case MD_BLOCK_TD:           tag = "td"; break;
        case MD_BLOCK_FRONTMATTER:  tag = "frontmatter"; break;
        case MD_BLOCK_COMPONENT:    tag = NULL; break;  /* handled below */
        case MD_BLOCK_TEMPLATE:     tag = NULL; break;  /* handled below */
        default:                    tag = "unknown"; break;
    }

    if(type == MD_BLOCK_DOC) {
        node = json_node_new(NULL, JSON_NODE_DOCUMENT);
    } else if(type == MD_BLOCK_COMPONENT) {
        const MD_BLOCK_COMPONENT_DETAIL* d = (const MD_BLOCK_COMPONENT_DETAIL*) detail;
        tag = json_attr_to_str(&d->tag_name);
        if(tag == NULL) { ctx->error = 1; return -1; }
        node = json_node_new(tag, JSON_NODE_ELEMENT);
        if(node == NULL) { free((char*)tag); ctx->error = 1; return -1; }
        node->tag_is_dynamic = 1;
        if(d->raw_props != NULL && d->raw_props_size > 0) {
            node->detail.component.raw_props = (char*) malloc(d->raw_props_size + 1);
            if(node->detail.component.raw_props != NULL) {
                memcpy(node->detail.component.raw_props, d->raw_props, d->raw_props_size);
                node->detail.component.raw_props[d->raw_props_size] = '\0';
                node->detail.component.raw_props_size = d->raw_props_size;
            }
        }
    } else if(type == MD_BLOCK_TEMPLATE) {
        const MD_BLOCK_TEMPLATE_DETAIL* d = (const MD_BLOCK_TEMPLATE_DETAIL*) detail;
        char* name_str = json_attr_to_str(&d->name);
        node = json_node_new("template", JSON_NODE_ELEMENT);
        if(node == NULL) { free(name_str); ctx->error = 1; return -1; }
        node->detail.tmpl.name = name_str;
    } else {
        node = json_node_new(tag, JSON_NODE_ELEMENT);
    }
    if(node == NULL) { ctx->error = 1; return -1; }

    /* Copy type-specific detail data. */
    switch(type) {
        case MD_BLOCK_UL: {
            const MD_BLOCK_UL_DETAIL* d = (const MD_BLOCK_UL_DETAIL*) detail;
            node->detail.ul.is_tight = d->is_tight;
            break;
        }
        case MD_BLOCK_OL: {
            const MD_BLOCK_OL_DETAIL* d = (const MD_BLOCK_OL_DETAIL*) detail;
            node->detail.ol.is_tight = d->is_tight;
            node->detail.ol.start = d->start;
            node->detail.ol.delimiter = d->mark_delimiter;
            break;
        }
        case MD_BLOCK_LI: {
            const MD_BLOCK_LI_DETAIL* d = (const MD_BLOCK_LI_DETAIL*) detail;
            node->detail.li.is_task = d->is_task;
            node->detail.li.task_mark = d->task_mark;
            break;
        }
        case MD_BLOCK_CODE: {
            const MD_BLOCK_CODE_DETAIL* d = (const MD_BLOCK_CODE_DETAIL*) detail;
            node->detail.code.info = json_attr_to_str(&d->info);
            node->detail.code.lang = json_attr_to_str(&d->lang);
            node->detail.code.fence_char = d->fence_char;
            node->detail.code.filename = json_attr_to_str(&d->filename);
            if(d->meta != NULL && d->meta_size > 0) {
                node->detail.code.meta = (char*) malloc(d->meta_size + 1);
                if(node->detail.code.meta != NULL) {
                    memcpy(node->detail.code.meta, d->meta, d->meta_size);
                    node->detail.code.meta[d->meta_size] = '\0';
                }
            }
            if(d->highlights != NULL && d->highlight_count > 0) {
                node->detail.code.highlights = (unsigned*) malloc(d->highlight_count * sizeof(unsigned));
                if(node->detail.code.highlights != NULL) {
                    memcpy(node->detail.code.highlights, d->highlights, d->highlight_count * sizeof(unsigned));
                    node->detail.code.highlight_count = d->highlight_count;
                }
            }
            break;
        }
        case MD_BLOCK_TABLE: {
            const MD_BLOCK_TABLE_DETAIL* d = (const MD_BLOCK_TABLE_DETAIL*) detail;
            node->detail.table.col_count = d->col_count;
            break;
        }
        case MD_BLOCK_TH:
        case MD_BLOCK_TD: {
            const MD_BLOCK_TD_DETAIL* d = (const MD_BLOCK_TD_DETAIL*) detail;
            node->detail.td.align = (int) d->align;
            break;
        }
        default:
            break;
    }

    if(ctx->current != NULL)
        json_append_child(ctx, node);
    else
        ctx->root = node;

    json_push(ctx, node);
    return ctx->error ? -1 : 0;
}

static int
json_leave_block(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    JSON_CTX* ctx = (JSON_CTX*) userdata;
    (void) type;
    (void) detail;
    json_pop(ctx);
    return 0;
}

static int
json_enter_span(MD_SPANTYPE type, void* detail, void* userdata)
{
    JSON_CTX* ctx = (JSON_CTX*) userdata;
    JSON_NODE* node;
    const char* tag;

    /* Inside an image: suppress nested spans, just accumulate alt text. */
    if(ctx->image_nesting > 0) {
        if(type == MD_SPAN_IMG)
            ctx->image_nesting++;
        return 0;
    }

    if(type == MD_SPAN_COMPONENT) {
        const MD_SPAN_COMPONENT_DETAIL* d = (const MD_SPAN_COMPONENT_DETAIL*) detail;
        tag = json_attr_to_str(&d->tag_name);
        if(tag == NULL) { ctx->error = 1; return -1; }
        node = json_node_new(tag, JSON_NODE_ELEMENT);
        if(node == NULL) { free((char*)tag); ctx->error = 1; return -1; }
        node->tag_is_dynamic = 1;
        if(d->raw_props != NULL && d->raw_props_size > 0) {
            node->detail.component.raw_props = (char*) malloc(d->raw_props_size + 1);
            if(node->detail.component.raw_props != NULL) {
                memcpy(node->detail.component.raw_props, d->raw_props, d->raw_props_size);
                node->detail.component.raw_props[d->raw_props_size] = '\0';
                node->detail.component.raw_props_size = d->raw_props_size;
            }
        }
    } else {
        switch(type) {
            case MD_SPAN_EM:                tag = "em"; break;
            case MD_SPAN_STRONG:            tag = "strong"; break;
            case MD_SPAN_A:                 tag = "a"; break;
            case MD_SPAN_IMG:               tag = "img"; break;
            case MD_SPAN_CODE:              tag = "code"; break;
            case MD_SPAN_DEL:               tag = "del"; break;
            case MD_SPAN_LATEXMATH:         tag = "math"; break;
            case MD_SPAN_LATEXMATH_DISPLAY: tag = "math-display"; break;
            case MD_SPAN_WIKILINK:          tag = "wikilink"; break;
            case MD_SPAN_U:                 tag = "u"; break;
            case MD_SPAN_SPAN:              tag = "span"; break;
            default:                        tag = "unknown"; break;
        }

        node = json_node_new(tag, JSON_NODE_ELEMENT);
        if(node == NULL) { ctx->error = 1; return -1; }

        switch(type) {
            case MD_SPAN_A: {
                const MD_SPAN_A_DETAIL* d = (const MD_SPAN_A_DETAIL*) detail;
                node->detail.a.href = json_attr_to_str(&d->href);
                node->detail.a.title = json_attr_to_str(&d->title);
                if(d->raw_attrs != NULL && d->raw_attrs_size > 0) {
                    node->raw_attrs = (char*) malloc(d->raw_attrs_size + 1);
                    if(node->raw_attrs != NULL) {
                        memcpy(node->raw_attrs, d->raw_attrs, d->raw_attrs_size);
                        node->raw_attrs[d->raw_attrs_size] = '\0';
                        node->raw_attrs_size = d->raw_attrs_size;
                    }
                }
                break;
            }
            case MD_SPAN_IMG: {
                const MD_SPAN_IMG_DETAIL* d = (const MD_SPAN_IMG_DETAIL*) detail;
                node->detail.img.src = json_attr_to_str(&d->src);
                node->detail.img.title = json_attr_to_str(&d->title);
                if(d->raw_attrs != NULL && d->raw_attrs_size > 0) {
                    node->raw_attrs = (char*) malloc(d->raw_attrs_size + 1);
                    if(node->raw_attrs != NULL) {
                        memcpy(node->raw_attrs, d->raw_attrs, d->raw_attrs_size);
                        node->raw_attrs[d->raw_attrs_size] = '\0';
                        node->raw_attrs_size = d->raw_attrs_size;
                    }
                }
                ctx->image_nesting = 1;
                break;
            }
            case MD_SPAN_WIKILINK: {
                const MD_SPAN_WIKILINK_DETAIL* d = (const MD_SPAN_WIKILINK_DETAIL*) detail;
                node->detail.wikilink.target = json_attr_to_str(&d->target);
                break;
            }
            case MD_SPAN_SPAN: {
                const MD_SPAN_SPAN_DETAIL* d = (const MD_SPAN_SPAN_DETAIL*) detail;
                if(d != NULL && d->raw_attrs != NULL && d->raw_attrs_size > 0) {
                    node->raw_attrs = (char*) malloc(d->raw_attrs_size + 1);
                    if(node->raw_attrs != NULL) {
                        memcpy(node->raw_attrs, d->raw_attrs, d->raw_attrs_size);
                        node->raw_attrs[d->raw_attrs_size] = '\0';
                        node->raw_attrs_size = d->raw_attrs_size;
                    }
                }
                break;
            }
            case MD_SPAN_EM:
            case MD_SPAN_STRONG:
            case MD_SPAN_CODE:
            case MD_SPAN_DEL:
            case MD_SPAN_U: {
                /* These spans may have trailing {attrs} via MD_SPAN_ATTRS_DETAIL. */
                if(detail != NULL) {
                    const MD_SPAN_ATTRS_DETAIL* d = (const MD_SPAN_ATTRS_DETAIL*) detail;
                    if(d->raw_attrs != NULL && d->raw_attrs_size > 0) {
                        node->raw_attrs = (char*) malloc(d->raw_attrs_size + 1);
                        if(node->raw_attrs != NULL) {
                            memcpy(node->raw_attrs, d->raw_attrs, d->raw_attrs_size);
                            node->raw_attrs[d->raw_attrs_size] = '\0';
                            node->raw_attrs_size = d->raw_attrs_size;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    json_append_child(ctx, node);
    json_push(ctx, node);
    return ctx->error ? -1 : 0;
}

static int
json_leave_span(MD_SPANTYPE type, void* detail, void* userdata)
{
    JSON_CTX* ctx = (JSON_CTX*) userdata;
    (void) detail;

    if(ctx->image_nesting > 0) {
        ctx->image_nesting--;
        if(ctx->image_nesting > 0)
            return 0;
        /* Leaving the image span: text_value has the accumulated alt text. */
    }

    json_pop(ctx);
    return 0;
}

static int
json_text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    JSON_CTX* ctx = (JSON_CTX*) userdata;
    JSON_NODE* node;
    JSON_NODE* prev;
    char* value = NULL;
    MD_SIZE value_size = 0;

    /* Inside an image: accumulate text as alt attribute. */
    if(ctx->image_nesting > 0) {
        if(type == MD_TEXT_SOFTBR) {
            if(json_append_text(ctx->current, " ", 1) != 0)
                { ctx->error = 1; return -1; }
        } else if(type == MD_TEXT_NULLCHAR) {
            char buf[3] = { (char)0xEF, (char)0xBF, (char)0xBD };
            if(json_append_text(ctx->current, buf, 3) != 0)
                { ctx->error = 1; return -1; }
        } else {
            if(json_append_text(ctx->current, text, size) != 0)
                { ctx->error = 1; return -1; }
        }
        return 0;
    }

    /* Leaf container nodes: accumulate text as literal on the parent node. */
    if(ctx->current->tag != NULL &&
       (strcmp(ctx->current->tag, "pre") == 0
        || strcmp(ctx->current->tag, "html_block") == 0
        || strcmp(ctx->current->tag, "code") == 0
        || strcmp(ctx->current->tag, "frontmatter") == 0
        || strcmp(ctx->current->tag, "math") == 0
        || strcmp(ctx->current->tag, "math-display") == 0)) {
        const char* src = text;
        MD_SIZE src_size = size;
        char nullchar_buf[3];

        if(type == MD_TEXT_NULLCHAR) {
            nullchar_buf[0] = (char)0xEF;
            nullchar_buf[1] = (char)0xBF;
            nullchar_buf[2] = (char)0xBD;
            src = nullchar_buf;
            src_size = 3;
        }

        if(json_append_text(ctx->current, src, src_size) != 0)
            { ctx->error = 1; return -1; }
        return 0;
    }

    switch(type) {
        case MD_TEXT_BR:
            /* Linebreak → ["br", {}] element node. */
            node = json_node_new("br", JSON_NODE_ELEMENT);
            if(node == NULL) { ctx->error = 1; return -1; }
            json_append_child(ctx, node);
            return 0;

        case MD_TEXT_SOFTBR:
            /* Softbreak → "\n" text. */
            value = (char*) malloc(2);
            if(value == NULL) { ctx->error = 1; return -1; }
            value[0] = '\n';
            value[1] = '\0';
            value_size = 1;
            break;

        case MD_TEXT_NULLCHAR:
            value = (char*) malloc(4);
            if(value == NULL) { ctx->error = 1; return -1; }
            /* U+FFFD in UTF-8 */
            value[0] = (char)0xEF;
            value[1] = (char)0xBF;
            value[2] = (char)0xBD;
            value[3] = '\0';
            value_size = 3;
            break;

        default:
            /* Normal text, entity, html_inline, code, latexmath. */
            value = (char*) malloc(size + 1);
            if(value == NULL) { ctx->error = 1; return -1; }
            memcpy(value, text, size);
            value[size] = '\0';
            value_size = size;
            break;
    }

    /* Merge consecutive text nodes. */
    prev = ctx->current->last_child;
    if(prev != NULL && prev->kind == JSON_NODE_TEXT
       && prev->text_value != NULL && value != NULL) {
        char* merged = (char*) realloc(prev->text_value, prev->text_size + value_size + 1);
        if(merged == NULL) { free(value); ctx->error = 1; return -1; }
        memcpy(merged + prev->text_size, value, value_size);
        prev->text_size += value_size;
        merged[prev->text_size] = '\0';
        prev->text_value = merged;
        free(value);
        return 0;
    }

    node = json_node_new(NULL, JSON_NODE_TEXT);
    if(node == NULL) { free(value); ctx->error = 1; return -1; }
    node->text_value = value;
    node->text_size = value_size;

    json_append_child(ctx, node);
    return 0;
}

static void
json_debug_log(const char* msg, void* userdata)
{
    (void) userdata;
    fprintf(stderr, "MD4X: %s\n", msg);
}


/****************************
 ***  JSON serialization  ***
 ****************************/

static void
json_write(JSON_WRITER* w, const char* data, MD_SIZE size)
{
    w->process_output(data, size, w->userdata);
}

static void
json_write_str(JSON_WRITER* w, const char* str)
{
    json_write(w, str, (MD_SIZE) strlen(str));
}

static void
json_write_escaped(JSON_WRITER* w, const char* str, MD_SIZE size)
{
    MD_OFFSET i;
    MD_OFFSET beg = 0;
    char esc[8];

    for(i = 0; i < size; i++) {
        unsigned char ch = (unsigned char) str[i];
        const char* replacement = NULL;
        int esc_len = 0;

        switch(ch) {
            case '"':   replacement = "\\\""; break;
            case '\\':  replacement = "\\\\"; break;
            case '\b':  replacement = "\\b"; break;
            case '\f':  replacement = "\\f"; break;
            case '\n':  replacement = "\\n"; break;
            case '\r':  replacement = "\\r"; break;
            case '\t':  replacement = "\\t"; break;
            default:
                if(ch < 0x20) {
                    json_snprintf(esc, sizeof(esc), "\\u%04x", ch);
                    replacement = esc;
                    esc_len = 6;
                }
                break;
        }

        if(replacement != NULL) {
            if(i > beg)
                json_write(w, str + beg, i - beg);
            if(esc_len == 0)
                esc_len = (int) strlen(replacement);
            json_write(w, replacement, (MD_SIZE) esc_len);
            beg = i + 1;
        }
    }

    if(i > beg)
        json_write(w, str + beg, i - beg);
}

static void
json_write_string(JSON_WRITER* w, const char* str, MD_SIZE size)
{
    json_write(w, "\"", 1);
    json_write_escaped(w, str, size);
    json_write(w, "\"", 1);
}

static const char*
json_align_str(int align)
{
    switch(align) {
        case MD_ALIGN_LEFT:    return "left";
        case MD_ALIGN_CENTER:  return "center";
        case MD_ALIGN_RIGHT:   return "right";
        default:               return NULL;
    }
}

/* Write parsed component props from a raw props string.
 * Uses the shared md_parse_props() parser from md4x-props.h.
 * Returns number of props written. */
static int
json_write_component_props(JSON_WRITER* w, const char* raw, MD_SIZE size)
{
    MD_PARSED_PROPS parsed;
    int n_written = 0;
    int i;

    md_parse_props(raw, size, &parsed);

    /* Write #id. */
    if(parsed.id != NULL && parsed.id_size > 0) {
        if(n_written > 0) json_write(w, ",", 1);
        json_write_str(w, "\"id\":");
        json_write_string(w, parsed.id, parsed.id_size);
        n_written++;
    }

    /* Write regular props. */
    for(i = 0; i < parsed.n_props; i++) {
        const MD_PROP* p = &parsed.props[i];

        if(n_written > 0) json_write(w, ",", 1);

        switch(p->type) {
            case MD_PROP_STRING:
                json_write(w, "\"", 1);
                json_write_escaped(w, p->key, p->key_size);
                json_write_str(w, "\":");
                json_write_string(w, p->value, p->value_size);
                n_written++;
                break;

            case MD_PROP_BOOLEAN:
                json_write(w, "\"", 1);
                json_write_escaped(w, p->key, p->key_size);
                json_write_str(w, "\":true");
                n_written++;
                break;

            case MD_PROP_BIND:
                json_write(w, "\":", 2);
                json_write_escaped(w, p->key, p->key_size);
                json_write_str(w, "\":");
                json_write(w, p->value, p->value_size);
                n_written++;
                break;
        }
    }

    /* Write merged class. */
    if(parsed.class_len > 0) {
        if(n_written > 0) json_write(w, ",", 1);
        json_write_str(w, "\"class\":");
        json_write_string(w, parsed.class_buf, parsed.class_len);
        n_written++;
    }

    return n_written;
}

/* Write the props object for an element node. */
static void
json_write_props(JSON_WRITER* w, const JSON_NODE* node)
{
    int has_prop = 0;

    json_write(w, "{", 1);

    if(strcmp(node->tag, "ol") == 0) {
        if(node->detail.ol.start != 1) {
            char buf[32];
            json_snprintf(buf, sizeof(buf), "\"start\":%u", node->detail.ol.start);
            json_write_str(w, buf);
            has_prop = 1;
        }
    }
    else if(strcmp(node->tag, "li") == 0 && node->detail.li.is_task) {
        json_write_str(w, "\"task\":true,\"checked\":");
        json_write_str(w, (node->detail.li.task_mark == 'x' || node->detail.li.task_mark == 'X') ? "true" : "false");
        has_prop = 1;
    }
    else if(strcmp(node->tag, "pre") == 0) {
        if(node->detail.code.lang != NULL && node->detail.code.lang[0] != '\0') {
            json_write_str(w, "\"language\":");
            json_write_string(w, node->detail.code.lang, (MD_SIZE) strlen(node->detail.code.lang));
            has_prop = 1;
        }
        if(node->detail.code.filename != NULL && node->detail.code.filename[0] != '\0') {
            if(has_prop) json_write(w, ",", 1);
            json_write_str(w, "\"filename\":");
            json_write_string(w, node->detail.code.filename, (MD_SIZE) strlen(node->detail.code.filename));
            has_prop = 1;
        }
        if(node->detail.code.highlights != NULL && node->detail.code.highlight_count > 0) {
            unsigned hi;
            char buf[16];
            if(has_prop) json_write(w, ",", 1);
            json_write_str(w, "\"highlights\":[");
            for(hi = 0; hi < node->detail.code.highlight_count; hi++) {
                if(hi > 0) json_write(w, ",", 1);
                json_snprintf(buf, sizeof(buf), "%u", node->detail.code.highlights[hi]);
                json_write_str(w, buf);
            }
            json_write(w, "]", 1);
            has_prop = 1;
        }
        if(node->detail.code.meta != NULL && node->detail.code.meta[0] != '\0') {
            if(has_prop) json_write(w, ",", 1);
            json_write_str(w, "\"meta\":");
            json_write_string(w, node->detail.code.meta, (MD_SIZE) strlen(node->detail.code.meta));
            has_prop = 1;
        }
    }
    else if(strcmp(node->tag, "th") == 0 || strcmp(node->tag, "td") == 0) {
        const char* align = json_align_str(node->detail.td.align);
        if(align != NULL) {
            json_write_str(w, "\"align\":\"");
            json_write_str(w, align);
            json_write(w, "\"", 1);
            has_prop = 1;
        }
    }
    else if(strcmp(node->tag, "a") == 0) {
        if(node->detail.a.href != NULL) {
            json_write_str(w, "\"href\":");
            json_write_string(w, node->detail.a.href, (MD_SIZE) strlen(node->detail.a.href));
            has_prop = 1;
        }
        if(node->detail.a.title != NULL && node->detail.a.title[0] != '\0') {
            if(has_prop) json_write(w, ",", 1);
            json_write_str(w, "\"title\":");
            json_write_string(w, node->detail.a.title, (MD_SIZE) strlen(node->detail.a.title));
            has_prop = 1;
        }
    }
    else if(strcmp(node->tag, "img") == 0) {
        if(node->detail.img.src != NULL) {
            json_write_str(w, "\"src\":");
            json_write_string(w, node->detail.img.src, (MD_SIZE) strlen(node->detail.img.src));
            has_prop = 1;
        }
        if(node->text_value != NULL) {
            if(has_prop) json_write(w, ",", 1);
            json_write_str(w, "\"alt\":");
            json_write_string(w, node->text_value, node->text_size);
            has_prop = 1;
        }
        if(node->detail.img.title != NULL && node->detail.img.title[0] != '\0') {
            if(has_prop) json_write(w, ",", 1);
            json_write_str(w, "\"title\":");
            json_write_string(w, node->detail.img.title, (MD_SIZE) strlen(node->detail.img.title));
            has_prop = 1;
        }
    }
    else if(strcmp(node->tag, "wikilink") == 0) {
        if(node->detail.wikilink.target != NULL) {
            json_write_str(w, "\"target\":");
            json_write_string(w, node->detail.wikilink.target, (MD_SIZE) strlen(node->detail.wikilink.target));
            has_prop = 1;
        }
    }
    else if(strcmp(node->tag, "template") == 0) {
        if(node->detail.tmpl.name != NULL) {
            json_write_str(w, "\"name\":");
            json_write_string(w, node->detail.tmpl.name, (MD_SIZE) strlen(node->detail.tmpl.name));
            has_prop = 1;
        }
    }
    else if(node->tag_is_dynamic) {
        /* Component: parse raw props string. */
        if(node->detail.component.raw_props != NULL && node->detail.component.raw_props_size > 0) {
            has_prop = json_write_component_props(w, node->detail.component.raw_props,
                                                  node->detail.component.raw_props_size);
        }
    }

    /* Merge inline attributes from trailing {attrs} syntax. */
    if(node->raw_attrs != NULL && node->raw_attrs_size > 0) {
        /* Pre-parse to check if there are any props to write. */
        MD_PARSED_PROPS check;
        md_parse_props(node->raw_attrs, node->raw_attrs_size, &check);
        if(check.n_props > 0 || check.id != NULL || check.class_len > 0) {
            if(has_prop) json_write(w, ",", 1);
            json_write_component_props(w, node->raw_attrs, node->raw_attrs_size);
            has_prop = 1;
        }
    }

    (void) has_prop;
    json_write(w, "}", 1);
}

static void
json_serialize_node(JSON_WRITER* w, const JSON_NODE* node)
{
    const JSON_NODE* child;
    int first;

    switch(node->kind) {
        case JSON_NODE_DOCUMENT:
            json_write_str(w, "{\"type\":\"comark\",\"value\":[");
            first = 1;
            for(child = node->first_child; child != NULL; child = child->next_sibling) {
                if(!first) json_write(w, ",", 1);
                json_serialize_node(w, child);
                first = 0;
            }
            json_write_str(w, "]}");
            break;

        case JSON_NODE_TEXT:
            json_write_string(w, node->text_value, node->text_size);
            break;

        case JSON_NODE_ELEMENT:
            json_write_str(w, "[\"");
            json_write_str(w, node->tag);
            json_write_str(w, "\",");

            /* Write props. */
            json_write_props(w, node);

            /* Special handling for code_block ("pre"): emit inner ["code", {}, literal]. */
            if(strcmp(node->tag, "pre") == 0) {
                json_write_str(w, ",[\"code\",{");
                if(node->detail.code.lang != NULL && node->detail.code.lang[0] != '\0') {
                    json_write_str(w, "\"class\":\"language-");
                    json_write_escaped(w, node->detail.code.lang, (MD_SIZE) strlen(node->detail.code.lang));
                    json_write(w, "\"", 1);
                }
                json_write_str(w, "},");
                if(node->text_value != NULL)
                    json_write_string(w, node->text_value, node->text_size);
                else
                    json_write_str(w, "\"\"");
                json_write(w, "]", 1);
            }
            /* html_block and frontmatter: emit literal as text child. */
            else if(node->text_value != NULL
                    && (strcmp(node->tag, "html_block") == 0
                        || strcmp(node->tag, "frontmatter") == 0)) {
                json_write(w, ",", 1);
                json_write_string(w, node->text_value, node->text_size);
            }
            /* Inline code, math, math-display: emit literal as text child. */
            else if(node->text_value != NULL
                    && (strcmp(node->tag, "code") == 0
                        || strcmp(node->tag, "math") == 0
                        || strcmp(node->tag, "math-display") == 0)) {
                json_write(w, ",", 1);
                json_write_string(w, node->text_value, node->text_size);
            }
            /* img: void element, no children (alt is in props). */
            else if(strcmp(node->tag, "img") == 0) {
                /* No children emitted. */
            }
            /* Regular container: emit children. */
            else {
                for(child = node->first_child; child != NULL; child = child->next_sibling) {
                    json_write(w, ",", 1);
                    json_serialize_node(w, child);
                }
            }

            json_write(w, "]", 1);
            break;
    }
}


/**************************************
 ***  Public API                    ***
 **************************************/

int
md_json(const MD_CHAR* input, MD_SIZE input_size,
        void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
        void* userdata, unsigned parser_flags, unsigned renderer_flags)
{
    JSON_CTX ctx;
    JSON_WRITER writer;
    int ret;

    MD_PARSER parser = {
        0,
        parser_flags,
        json_enter_block,
        json_leave_block,
        json_enter_span,
        json_leave_span,
        json_text,
        (renderer_flags & MD_JSON_FLAG_DEBUG) ? json_debug_log : NULL,
        NULL
    };

    memset(&ctx, 0, sizeof(ctx));

#ifndef MD4X_USE_ASCII
    /* Skip UTF-8 BOM. */
    if(renderer_flags & MD_JSON_FLAG_SKIP_UTF8_BOM) {
        if(input_size >= 3 && (unsigned char)input[0] == 0xEF
           && (unsigned char)input[1] == 0xBB && (unsigned char)input[2] == 0xBF) {
            input += 3;
            input_size -= 3;
        }
    }
#endif

    ret = md_parse(input, input_size, &parser, (void*) &ctx);

    if(ret != 0 || ctx.error != 0) {
        json_node_free(ctx.root);
        return -1;
    }

    /* Serialize the AST to JSON via the output callback. */
    writer.process_output = process_output;
    writer.userdata = userdata;
    json_serialize_node(&writer, ctx.root);
    json_write(&writer, "\n", 1);

    json_node_free(ctx.root);
    return 0;
}
