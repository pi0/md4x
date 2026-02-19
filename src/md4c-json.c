/*
 * MD4C: Markdown parser for C
 * (http://github.com/mity/md4c)
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

#include "md4c-json.h"


#ifdef _WIN32
    #define json_snprintf _snprintf
#else
    #define json_snprintf snprintf
#endif

#define JSON_MAX_DEPTH  256


/*************************************
 ***  JSON AST node data structs   ***
 *************************************/

typedef enum {
    JSON_NODE_BLOCK,
    JSON_NODE_SPAN,
    JSON_NODE_TEXT
} JSON_NODE_KIND;

typedef struct JSON_NODE JSON_NODE;
struct JSON_NODE {
    JSON_NODE_KIND kind;
    const char* type_name;

    JSON_NODE* first_child;
    JSON_NODE* last_child;
    JSON_NODE* next_sibling;

    char* text_value;
    MD_SIZE text_size;

    union {
        struct { const char* list_type; int is_tight; unsigned start; char delimiter; } list;
        struct { int is_task; char task_mark; } li;
        struct { unsigned level; } h;
        struct { char* info; char* lang; char fence_char; } code;
        struct { unsigned col_count; unsigned head_row_count; unsigned body_row_count; } table;
        struct { int align; } td;
        struct { char* destination; char* title; int is_autolink; } a;
        struct { char* destination; char* title; } img;
        struct { char* target; } wikilink;
    } detail;
};

typedef struct {
    JSON_NODE* root;
    JSON_NODE* current;
    JSON_NODE* stack[JSON_MAX_DEPTH];
    int stack_depth;
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
json_node_new(const char* type_name, JSON_NODE_KIND kind)
{
    JSON_NODE* node = (JSON_NODE*) calloc(1, sizeof(JSON_NODE));
    if(node == NULL)
        return NULL;
    node->type_name = type_name;
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
    if(strcmp(node->type_name, "code_block") == 0) {
        if(node->detail.code.info) free(node->detail.code.info);
        if(node->detail.code.lang) free(node->detail.code.lang);
    } else if(strcmp(node->type_name, "link") == 0) {
        if(node->detail.a.destination) free(node->detail.a.destination);
        if(node->detail.a.title) free(node->detail.a.title);
    } else if(strcmp(node->type_name, "image") == 0) {
        if(node->detail.img.destination) free(node->detail.img.destination);
        if(node->detail.img.title) free(node->detail.img.title);
    } else if(strcmp(node->type_name, "wikilink") == 0) {
        if(node->detail.wikilink.target) free(node->detail.wikilink.target);
    }

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


/***********************************
 ***  md_parse() callbacks       ***
 ***********************************/

static int
json_enter_block(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    JSON_CTX* ctx = (JSON_CTX*) userdata;
    JSON_NODE* node;
    const char* type_name;

    switch(type) {
        case MD_BLOCK_DOC:          type_name = "document"; break;
        case MD_BLOCK_QUOTE:        type_name = "block_quote"; break;
        case MD_BLOCK_UL:           type_name = "list"; break;
        case MD_BLOCK_OL:           type_name = "list"; break;
        case MD_BLOCK_LI:           type_name = "item"; break;
        case MD_BLOCK_HR:           type_name = "thematic_break"; break;
        case MD_BLOCK_H:            type_name = "heading"; break;
        case MD_BLOCK_CODE:         type_name = "code_block"; break;
        case MD_BLOCK_HTML:         type_name = "html_block"; break;
        case MD_BLOCK_P:            type_name = "paragraph"; break;
        case MD_BLOCK_TABLE:        type_name = "table"; break;
        case MD_BLOCK_THEAD:        type_name = "table_head"; break;
        case MD_BLOCK_TBODY:        type_name = "table_body"; break;
        case MD_BLOCK_TR:           type_name = "table_row"; break;
        case MD_BLOCK_TH:           type_name = "table_header_cell"; break;
        case MD_BLOCK_TD:           type_name = "table_cell"; break;
        case MD_BLOCK_FRONTMATTER:  type_name = "frontmatter"; break;
        default:                    type_name = "unknown"; break;
    }

    node = json_node_new(type_name, JSON_NODE_BLOCK);
    if(node == NULL) { ctx->error = 1; return -1; }

    /* Copy type-specific detail data. */
    switch(type) {
        case MD_BLOCK_UL: {
            const MD_BLOCK_UL_DETAIL* d = (const MD_BLOCK_UL_DETAIL*) detail;
            node->detail.list.list_type = "bullet";
            node->detail.list.is_tight = d->is_tight;
            break;
        }
        case MD_BLOCK_OL: {
            const MD_BLOCK_OL_DETAIL* d = (const MD_BLOCK_OL_DETAIL*) detail;
            node->detail.list.list_type = "ordered";
            node->detail.list.is_tight = d->is_tight;
            node->detail.list.start = d->start;
            node->detail.list.delimiter = d->mark_delimiter;
            break;
        }
        case MD_BLOCK_LI: {
            const MD_BLOCK_LI_DETAIL* d = (const MD_BLOCK_LI_DETAIL*) detail;
            node->detail.li.is_task = d->is_task;
            node->detail.li.task_mark = d->task_mark;
            break;
        }
        case MD_BLOCK_H: {
            const MD_BLOCK_H_DETAIL* d = (const MD_BLOCK_H_DETAIL*) detail;
            node->detail.h.level = d->level;
            break;
        }
        case MD_BLOCK_CODE: {
            const MD_BLOCK_CODE_DETAIL* d = (const MD_BLOCK_CODE_DETAIL*) detail;
            node->detail.code.info = json_attr_to_str(&d->info);
            node->detail.code.lang = json_attr_to_str(&d->lang);
            node->detail.code.fence_char = d->fence_char;
            break;
        }
        case MD_BLOCK_TABLE: {
            const MD_BLOCK_TABLE_DETAIL* d = (const MD_BLOCK_TABLE_DETAIL*) detail;
            node->detail.table.col_count = d->col_count;
            node->detail.table.head_row_count = d->head_row_count;
            node->detail.table.body_row_count = d->body_row_count;
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
    const char* type_name;

    switch(type) {
        case MD_SPAN_EM:                type_name = "emph"; break;
        case MD_SPAN_STRONG:            type_name = "strong"; break;
        case MD_SPAN_A:                 type_name = "link"; break;
        case MD_SPAN_IMG:               type_name = "image"; break;
        case MD_SPAN_CODE:              type_name = "code"; break;
        case MD_SPAN_DEL:               type_name = "delete"; break;
        case MD_SPAN_LATEXMATH:         type_name = "latex_math"; break;
        case MD_SPAN_LATEXMATH_DISPLAY: type_name = "latex_math_display"; break;
        case MD_SPAN_WIKILINK:          type_name = "wikilink"; break;
        case MD_SPAN_U:                 type_name = "underline"; break;
        default:                        type_name = "unknown"; break;
    }

    node = json_node_new(type_name, JSON_NODE_SPAN);
    if(node == NULL) { ctx->error = 1; return -1; }

    switch(type) {
        case MD_SPAN_A: {
            const MD_SPAN_A_DETAIL* d = (const MD_SPAN_A_DETAIL*) detail;
            node->detail.a.destination = json_attr_to_str(&d->href);
            node->detail.a.title = json_attr_to_str(&d->title);
            node->detail.a.is_autolink = d->is_autolink;
            break;
        }
        case MD_SPAN_IMG: {
            const MD_SPAN_IMG_DETAIL* d = (const MD_SPAN_IMG_DETAIL*) detail;
            node->detail.img.destination = json_attr_to_str(&d->src);
            node->detail.img.title = json_attr_to_str(&d->title);
            break;
        }
        case MD_SPAN_WIKILINK: {
            const MD_SPAN_WIKILINK_DETAIL* d = (const MD_SPAN_WIKILINK_DETAIL*) detail;
            node->detail.wikilink.target = json_attr_to_str(&d->target);
            break;
        }
        default:
            break;
    }

    json_append_child(ctx, node);
    json_push(ctx, node);
    return ctx->error ? -1 : 0;
}

static int
json_leave_span(MD_SPANTYPE type, void* detail, void* userdata)
{
    JSON_CTX* ctx = (JSON_CTX*) userdata;
    (void) type;
    (void) detail;
    json_pop(ctx);
    return 0;
}

static int
json_text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    JSON_CTX* ctx = (JSON_CTX*) userdata;
    JSON_NODE* node;
    JSON_NODE* prev;
    const char* type_name;
    char* value = NULL;
    MD_SIZE value_size = 0;

    /* Leaf container nodes: accumulate text as literal on the parent node
     * instead of creating child text nodes (matches commonmark.js behavior
     * where code_block, html_block, and code are leaf nodes with literal). */
    if(strcmp(ctx->current->type_name, "code_block") == 0
       || strcmp(ctx->current->type_name, "html_block") == 0
       || strcmp(ctx->current->type_name, "code") == 0) {
        const char* src = text;
        MD_SIZE src_size = size;
        char nullchar_buf[4];

        if(type == MD_TEXT_NULLCHAR) {
            nullchar_buf[0] = (char)0xEF;
            nullchar_buf[1] = (char)0xBF;
            nullchar_buf[2] = (char)0xBD;
            nullchar_buf[3] = '\0';
            src = nullchar_buf;
            src_size = 3;
        }

        if(ctx->current->text_value == NULL) {
            ctx->current->text_value = (char*) malloc(src_size + 1);
            if(ctx->current->text_value == NULL) { ctx->error = 1; return -1; }
            memcpy(ctx->current->text_value, src, src_size);
            ctx->current->text_value[src_size] = '\0';
            ctx->current->text_size = src_size;
        } else {
            char* merged = (char*) realloc(ctx->current->text_value, ctx->current->text_size + src_size + 1);
            if(merged == NULL) { ctx->error = 1; return -1; }
            memcpy(merged + ctx->current->text_size, src, src_size);
            ctx->current->text_size += src_size;
            merged[ctx->current->text_size] = '\0';
            ctx->current->text_value = merged;
        }
        return 0;
    }

    switch(type) {
        case MD_TEXT_NORMAL:
        case MD_TEXT_CODE:
        case MD_TEXT_LATEXMATH:
            type_name = "text";
            value = (char*) malloc(size + 1);
            if(value == NULL) { ctx->error = 1; return -1; }
            memcpy(value, text, size);
            value[size] = '\0';
            value_size = size;
            break;

        case MD_TEXT_NULLCHAR:
            type_name = "text";
            value = (char*) malloc(4);
            if(value == NULL) { ctx->error = 1; return -1; }
            /* U+FFFD in UTF-8 */
            value[0] = (char)0xEF;
            value[1] = (char)0xBF;
            value[2] = (char)0xBD;
            value[3] = '\0';
            value_size = 3;
            break;

        case MD_TEXT_BR:
            type_name = "linebreak";
            break;

        case MD_TEXT_SOFTBR:
            type_name = "softbreak";
            break;

        case MD_TEXT_ENTITY:
            type_name = "text";
            value = (char*) malloc(size + 1);
            if(value == NULL) { ctx->error = 1; return -1; }
            memcpy(value, text, size);
            value[size] = '\0';
            value_size = size;
            break;

        case MD_TEXT_HTML:
            type_name = "html_inline";
            value = (char*) malloc(size + 1);
            if(value == NULL) { ctx->error = 1; return -1; }
            memcpy(value, text, size);
            value[size] = '\0';
            value_size = size;
            break;

        default:
            type_name = "text";
            value = (char*) malloc(size + 1);
            if(value == NULL) { ctx->error = 1; return -1; }
            memcpy(value, text, size);
            value[size] = '\0';
            value_size = size;
            break;
    }

    /* Merge consecutive text nodes of the same type. */
    prev = ctx->current->last_child;
    if(prev != NULL && prev->kind == JSON_NODE_TEXT
       && prev->type_name == type_name
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

    node = json_node_new(type_name, JSON_NODE_TEXT);
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
    fprintf(stderr, "MD4C: %s\n", msg);
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
json_write_indent(JSON_WRITER* w, int depth)
{
    int i;
    for(i = 0; i < depth; i++)
        json_write(w, "  ", 2);
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
json_write_string_value(JSON_WRITER* w, const char* str, MD_SIZE size)
{
    json_write(w, "\"", 1);
    json_write_escaped(w, str, size);
    json_write(w, "\"", 1);
}

static void
json_write_char_value(JSON_WRITER* w, char ch)
{
    json_write(w, "\"", 1);
    json_write_escaped(w, &ch, 1);
    json_write(w, "\"", 1);
}

static const char*
json_align_str(int align)
{
    switch(align) {
        case MD_ALIGN_LEFT:    return "left";
        case MD_ALIGN_CENTER:  return "center";
        case MD_ALIGN_RIGHT:   return "right";
        default:               return "default";
    }
}

static void
json_serialize_node(JSON_WRITER* w, const JSON_NODE* node, int depth)
{
    const JSON_NODE* child;
    char num_buf[32];
    int first;

    json_write_indent(w, depth);
    json_write_str(w, "{\n");

    /* "type" */
    json_write_indent(w, depth + 1);
    json_write_str(w, "\"type\": ");
    json_write_string_value(w, node->type_name, (MD_SIZE) strlen(node->type_name));

    /* Type-specific properties. */
    if(strcmp(node->type_name, "heading") == 0) {
        json_snprintf(num_buf, sizeof(num_buf), "%u", node->detail.h.level);
        json_write_str(w, ",\n");
        json_write_indent(w, depth + 1);
        json_write_str(w, "\"level\": ");
        json_write_str(w, num_buf);
    }
    else if(strcmp(node->type_name, "list") == 0) {
        json_write_str(w, ",\n");
        json_write_indent(w, depth + 1);
        json_write_str(w, "\"listType\": \"");
        json_write_str(w, node->detail.list.list_type);
        json_write_str(w, "\"");
        json_write_str(w, ",\n");
        json_write_indent(w, depth + 1);
        json_write_str(w, "\"listTight\": ");
        json_write_str(w, node->detail.list.is_tight ? "true" : "false");
        if(strcmp(node->detail.list.list_type, "ordered") == 0) {
            json_snprintf(num_buf, sizeof(num_buf), "%u", node->detail.list.start);
            json_write_str(w, ",\n");
            json_write_indent(w, depth + 1);
            json_write_str(w, "\"listStart\": ");
            json_write_str(w, num_buf);
            json_write_str(w, ",\n");
            json_write_indent(w, depth + 1);
            json_write_str(w, "\"listDelimiter\": ");
            json_write_str(w, node->detail.list.delimiter == ')' ? "\"paren\"" : "\"period\"");
        }
    }
    else if(strcmp(node->type_name, "item") == 0 && node->detail.li.is_task) {
        json_write_str(w, ",\n");
        json_write_indent(w, depth + 1);
        json_write_str(w, "\"task\": true,\n");
        json_write_indent(w, depth + 1);
        json_write_str(w, "\"checked\": ");
        json_write_str(w, (node->detail.li.task_mark == 'x' || node->detail.li.task_mark == 'X') ? "true" : "false");
    }
    else if(strcmp(node->type_name, "code_block") == 0) {
        if(node->detail.code.info != NULL) {
            json_write_str(w, ",\n");
            json_write_indent(w, depth + 1);
            json_write_str(w, "\"info\": ");
            json_write_string_value(w, node->detail.code.info, (MD_SIZE) strlen(node->detail.code.info));
        }
        if(node->detail.code.fence_char != '\0') {
            json_write_str(w, ",\n");
            json_write_indent(w, depth + 1);
            json_write_str(w, "\"fence\": ");
            json_write_char_value(w, node->detail.code.fence_char);
        }
    }
    else if(strcmp(node->type_name, "table") == 0) {
        json_snprintf(num_buf, sizeof(num_buf), "%u", node->detail.table.col_count);
        json_write_str(w, ",\n");
        json_write_indent(w, depth + 1);
        json_write_str(w, "\"columns\": ");
        json_write_str(w, num_buf);
        json_snprintf(num_buf, sizeof(num_buf), "%u", node->detail.table.head_row_count);
        json_write_str(w, ",\n");
        json_write_indent(w, depth + 1);
        json_write_str(w, "\"header_rows\": ");
        json_write_str(w, num_buf);
        json_snprintf(num_buf, sizeof(num_buf), "%u", node->detail.table.body_row_count);
        json_write_str(w, ",\n");
        json_write_indent(w, depth + 1);
        json_write_str(w, "\"body_rows\": ");
        json_write_str(w, num_buf);
    }
    else if(strcmp(node->type_name, "table_header_cell") == 0 || strcmp(node->type_name, "table_cell") == 0) {
        json_write_str(w, ",\n");
        json_write_indent(w, depth + 1);
        json_write_str(w, "\"align\": \"");
        json_write_str(w, json_align_str(node->detail.td.align));
        json_write_str(w, "\"");
    }
    else if(strcmp(node->type_name, "link") == 0) {
        if(node->detail.a.destination != NULL) {
            json_write_str(w, ",\n");
            json_write_indent(w, depth + 1);
            json_write_str(w, "\"destination\": ");
            json_write_string_value(w, node->detail.a.destination, (MD_SIZE) strlen(node->detail.a.destination));
        }
        if(node->detail.a.title != NULL) {
            json_write_str(w, ",\n");
            json_write_indent(w, depth + 1);
            json_write_str(w, "\"title\": ");
            json_write_string_value(w, node->detail.a.title, (MD_SIZE) strlen(node->detail.a.title));
        }
        if(node->detail.a.is_autolink) {
            json_write_str(w, ",\n");
            json_write_indent(w, depth + 1);
            json_write_str(w, "\"autolink\": true");
        }
    }
    else if(strcmp(node->type_name, "image") == 0) {
        if(node->detail.img.destination != NULL) {
            json_write_str(w, ",\n");
            json_write_indent(w, depth + 1);
            json_write_str(w, "\"destination\": ");
            json_write_string_value(w, node->detail.img.destination, (MD_SIZE) strlen(node->detail.img.destination));
        }
        if(node->detail.img.title != NULL) {
            json_write_str(w, ",\n");
            json_write_indent(w, depth + 1);
            json_write_str(w, "\"title\": ");
            json_write_string_value(w, node->detail.img.title, (MD_SIZE) strlen(node->detail.img.title));
        }
    }
    else if(strcmp(node->type_name, "wikilink") == 0) {
        if(node->detail.wikilink.target != NULL) {
            json_write_str(w, ",\n");
            json_write_indent(w, depth + 1);
            json_write_str(w, "\"target\": ");
            json_write_string_value(w, node->detail.wikilink.target, (MD_SIZE) strlen(node->detail.wikilink.target));
        }
    }

    /* Literal for leaf nodes (text nodes and leaf containers like code_block). */
    if(node->text_value != NULL) {
        json_write_str(w, ",\n");
        json_write_indent(w, depth + 1);
        json_write_str(w, "\"literal\": ");
        json_write_string_value(w, node->text_value, node->text_size);
    }

    /* Children array for container nodes (skip for leaf containers with literal). */
    if((node->kind == JSON_NODE_BLOCK || node->kind == JSON_NODE_SPAN)
       && node->text_value == NULL) {
        json_write_str(w, ",\n");
        json_write_indent(w, depth + 1);
        json_write_str(w, "\"children\": [");

        if(node->first_child != NULL) {
            json_write_str(w, "\n");
            first = 1;
            for(child = node->first_child; child != NULL; child = child->next_sibling) {
                if(!first)
                    json_write_str(w, ",\n");
                json_serialize_node(w, child, depth + 2);
                first = 0;
            }
            json_write_str(w, "\n");
            json_write_indent(w, depth + 1);
        }

        json_write_str(w, "]");
    }

    json_write_str(w, "\n");
    json_write_indent(w, depth);
    json_write_str(w, "}");
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

#ifndef MD4C_USE_ASCII
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
    json_serialize_node(&writer, ctx.root, 0);
    json_write(&writer, "\n", 1);

    json_node_free(ctx.root);
    return 0;
}
