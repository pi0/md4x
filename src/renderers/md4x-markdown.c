/*
 * MD4X: Markdown parser for C
 * (http://github.com/unjs/md4x)
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
#include <string.h>

#include "md4x-markdown.h"
#include "md4x-heal-wrap.h"
#include "entity.h"


#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199409L
    #if defined __GNUC__
        #define inline __inline__
    #elif defined _MSC_VER
        #define inline __inline
    #else
        #define inline
    #endif
#endif

#ifdef _WIN32
    #define snprintf _snprintf
#endif


typedef struct MD_MARKDOWN_tag MD_MARKDOWN;
struct MD_MARKDOWN_tag {
    void (*process_output)(const MD_CHAR*, MD_SIZE, void*);
    void* userdata;
    unsigned flags;
    int image_nesting_level;
    int quote_depth;
    int list_depth;
    int ol_counter;
    int in_code_block;
    int in_code_span;
    int need_newline;
    int need_indent;
    int li_opened;
    int in_frontmatter;

    /* Table state */
    int in_table;
    int in_thead;
    int thead_done;
    int current_col;
    int col_count;
    MD_ALIGN col_aligns[128];

    /* Code block fence */
    MD_CHAR fence_char;
    int fence_len;
};


/*********************************************
 ***  Markdown rendering helper functions ***
 *********************************************/

static inline void
render_verbatim(MD_MARKDOWN* r, const MD_CHAR* text, MD_SIZE size)
{
    r->process_output(text, size, r->userdata);
}

#define RENDER_VERBATIM(r, verbatim)                                    \
        render_verbatim((r), (verbatim), (MD_SIZE) (strlen(verbatim)))

static void
render_indent(MD_MARKDOWN* r)
{
    int i;
    for(i = 0; i < r->quote_depth; i++) {
        RENDER_VERBATIM(r, "> ");
    }
    for(i = 0; i < r->list_depth; i++) {
        RENDER_VERBATIM(r, "  ");
    }
}

static void
render_newline(MD_MARKDOWN* r)
{
    RENDER_VERBATIM(r, "\n");
}

static unsigned
hex_val(char ch)
{
    if('0' <= ch && ch <= '9')
        return ch - '0';
    if('a' <= ch && ch <= 'f')
        return ch - 'a' + 10;
    if('A' <= ch && ch <= 'F')
        return ch - 'A' + 10;
    return 0;
}

static void
render_utf8_codepoint(MD_MARKDOWN* r, unsigned codepoint,
                      void (*fn_append)(MD_MARKDOWN*, const MD_CHAR*, MD_SIZE))
{
    static const MD_CHAR utf8_replacement_char[] = { (char)0xef, (char)0xbf, (char)0xbd };

    unsigned char utf8[4];
    size_t n;

    if(codepoint <= 0x7f) {
        n = 1;
        utf8[0] = codepoint;
    } else if(codepoint <= 0x7ff) {
        n = 2;
        utf8[0] = 0xc0 | ((codepoint >>  6) & 0x1f);
        utf8[1] = 0x80 + ((codepoint >>  0) & 0x3f);
    } else if(codepoint <= 0xffff) {
        n = 3;
        utf8[0] = 0xe0 | ((codepoint >> 12) & 0xf);
        utf8[1] = 0x80 + ((codepoint >>  6) & 0x3f);
        utf8[2] = 0x80 + ((codepoint >>  0) & 0x3f);
    } else {
        n = 4;
        utf8[0] = 0xf0 | ((codepoint >> 18) & 0x7);
        utf8[1] = 0x80 + ((codepoint >> 12) & 0x3f);
        utf8[2] = 0x80 + ((codepoint >>  6) & 0x3f);
        utf8[3] = 0x80 + ((codepoint >>  0) & 0x3f);
    }

    if(0 < codepoint  &&  codepoint <= 0x10ffff)
        fn_append(r, (char*)utf8, (MD_SIZE)n);
    else
        fn_append(r, utf8_replacement_char, 3);
}

static void
render_entity(MD_MARKDOWN* r, const MD_CHAR* text, MD_SIZE size,
              void (*fn_append)(MD_MARKDOWN*, const MD_CHAR*, MD_SIZE))
{
    if(size > 3 && text[1] == '#') {
        unsigned codepoint = 0;

        if(text[2] == 'x' || text[2] == 'X') {
            MD_SIZE i;
            for(i = 3; i < size-1; i++)
                codepoint = 16 * codepoint + hex_val(text[i]);
        } else {
            MD_SIZE i;
            for(i = 2; i < size-1; i++)
                codepoint = 10 * codepoint + (text[i] - '0');
        }

        render_utf8_codepoint(r, codepoint, fn_append);
        return;
    } else {
        const ENTITY* ent;

        ent = entity_lookup(text, size);
        if(ent != NULL) {
            render_utf8_codepoint(r, ent->codepoints[0], fn_append);
            if(ent->codepoints[1])
                render_utf8_codepoint(r, ent->codepoints[1], fn_append);
            return;
        }
    }

    fn_append(r, text, size);
}

static void
render_attribute(MD_MARKDOWN* r, const MD_ATTRIBUTE* attr,
                 void (*fn_append)(MD_MARKDOWN*, const MD_CHAR*, MD_SIZE))
{
    int i;

    for(i = 0; attr->substr_offsets[i] < attr->size; i++) {
        MD_TEXTTYPE type = attr->substr_types[i];
        MD_OFFSET off = attr->substr_offsets[i];
        MD_SIZE size = attr->substr_offsets[i+1] - off;
        const MD_CHAR* text = attr->text + off;

        switch(type) {
            case MD_TEXT_NULLCHAR:  render_utf8_codepoint(r, 0x0000, render_verbatim); break;
            case MD_TEXT_ENTITY:    render_entity(r, text, size, fn_append); break;
            default:                fn_append(r, text, size); break;
        }
    }
}

static void
render_table_separator(MD_MARKDOWN* r)
{
    int i;
    render_indent(r);
    RENDER_VERBATIM(r, "|");
    for(i = 0; i < r->col_count; i++) {
        switch(r->col_aligns[i]) {
            case MD_ALIGN_LEFT:     RENDER_VERBATIM(r, " :--- |"); break;
            case MD_ALIGN_CENTER:   RENDER_VERBATIM(r, " :---: |"); break;
            case MD_ALIGN_RIGHT:    RENDER_VERBATIM(r, " ---: |"); break;
            default:                RENDER_VERBATIM(r, " --- |"); break;
        }
    }
    render_newline(r);
}


/******************************************
 ***  Markdown renderer implementation ***
 ******************************************/

static int
enter_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MD_MARKDOWN* r = (MD_MARKDOWN*) userdata;

    switch(type) {
        case MD_BLOCK_DOC:
            break;

        case MD_BLOCK_QUOTE:
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            r->quote_depth++;
            break;

        case MD_BLOCK_UL:
            if(r->need_newline && r->list_depth == 0) {
                render_newline(r);
                r->need_newline = 0;
            }
            r->ol_counter = 0;
            break;

        case MD_BLOCK_OL:
            if(r->need_newline && r->list_depth == 0) {
                render_newline(r);
                r->need_newline = 0;
            }
            r->ol_counter = ((MD_BLOCK_OL_DETAIL*)detail)->start;
            break;

        case MD_BLOCK_LI: {
            const MD_BLOCK_LI_DETAIL* li = (const MD_BLOCK_LI_DETAIL*)detail;
            render_indent(r);
            if(li->is_task) {
                if(li->task_mark == 'x' || li->task_mark == 'X') {
                    RENDER_VERBATIM(r, "- [x] ");
                } else {
                    RENDER_VERBATIM(r, "- [ ] ");
                }
            } else {
                if(r->ol_counter > 0) {
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%d. ", r->ol_counter);
                    RENDER_VERBATIM(r, buf);
                    r->ol_counter++;
                } else {
                    RENDER_VERBATIM(r, "- ");
                }
            }
            r->list_depth++;
            r->li_opened = 1;
            break;
        }

        case MD_BLOCK_HR:
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            render_indent(r);
            RENDER_VERBATIM(r, "---");
            render_newline(r);
            r->need_newline = 1;
            break;

        case MD_BLOCK_H: {
            const MD_BLOCK_H_DETAIL* h = (const MD_BLOCK_H_DETAIL*)detail;
            unsigned level = h->level;
            unsigned i;
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            render_indent(r);
            for(i = 0; i < level && i < 6; i++)
                RENDER_VERBATIM(r, "#");
            RENDER_VERBATIM(r, " ");
            break;
        }

        case MD_BLOCK_CODE: {
            const MD_BLOCK_CODE_DETAIL* code = (const MD_BLOCK_CODE_DETAIL*)detail;
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            render_indent(r);
            r->fence_char = code->fence_char;
            if(code->fence_char == '~') {
                RENDER_VERBATIM(r, "~~~");
                r->fence_len = 3;
            } else {
                RENDER_VERBATIM(r, "```");
                r->fence_len = 3;
            }
            if(code->info.text != NULL && code->info.size > 0) {
                render_attribute(r, &code->info, render_verbatim);
            }
            render_newline(r);
            r->in_code_block = 1;
            r->need_indent = 1;
            break;
        }

        case MD_BLOCK_HTML:
            /* Strip raw HTML blocks */
            break;

        case MD_BLOCK_P:
            if(r->need_newline && !r->li_opened) {
                render_newline(r);
                r->need_newline = 0;
            }
            if(!r->li_opened)
                render_indent(r);
            r->li_opened = 0;
            break;

        case MD_BLOCK_TABLE: {
            const MD_BLOCK_TABLE_DETAIL* tbl = (const MD_BLOCK_TABLE_DETAIL*)detail;
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            r->in_table = 1;
            r->col_count = (int)tbl->col_count;
            if(r->col_count > 128)
                r->col_count = 128;
            memset(r->col_aligns, 0, sizeof(r->col_aligns));
            break;
        }

        case MD_BLOCK_THEAD:
            r->in_thead = 1;
            r->thead_done = 0;
            break;

        case MD_BLOCK_TBODY:
            break;

        case MD_BLOCK_TR:
            render_indent(r);
            RENDER_VERBATIM(r, "|");
            r->current_col = 0;
            break;

        case MD_BLOCK_TH: {
            const MD_BLOCK_TD_DETAIL* td = (const MD_BLOCK_TD_DETAIL*)detail;
            RENDER_VERBATIM(r, " ");
            if(r->current_col < 128)
                r->col_aligns[r->current_col] = td->align;
            break;
        }

        case MD_BLOCK_TD:
            RENDER_VERBATIM(r, " ");
            break;

        case MD_BLOCK_FRONTMATTER:
            r->in_frontmatter = 1;
            break;

        case MD_BLOCK_COMPONENT: {
            const MD_BLOCK_COMPONENT_DETAIL* comp = (const MD_BLOCK_COMPONENT_DETAIL*)detail;
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            render_indent(r);
            RENDER_VERBATIM(r, "<");
            if(comp->tag_name.text != NULL && comp->tag_name.size > 0)
                render_attribute(r, &comp->tag_name, render_verbatim);
            if(comp->title != NULL && comp->title_size > 0) {
                RENDER_VERBATIM(r, " title=\"");
                render_verbatim(r, comp->title, comp->title_size);
                RENDER_VERBATIM(r, "\"");
            }
            RENDER_VERBATIM(r, ">");
            render_newline(r);
            render_newline(r);
            break;
        }

        case MD_BLOCK_ALERT: {
            const MD_BLOCK_ALERT_DETAIL* det = (const MD_BLOCK_ALERT_DETAIL*)detail;
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            r->quote_depth++;
            render_indent(r);
            RENDER_VERBATIM(r, "[!");
            if(det->type_name.text != NULL && det->type_name.size > 0)
                render_attribute(r, &det->type_name, render_verbatim);
            RENDER_VERBATIM(r, "]");
            render_newline(r);
            break;
        }

        case MD_BLOCK_TEMPLATE:
            /* Transparent — render children normally */
            break;
    }

    return 0;
}

static int
leave_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MD_MARKDOWN* r = (MD_MARKDOWN*) userdata;

    (void) detail;

    switch(type) {
        case MD_BLOCK_DOC:
            break;

        case MD_BLOCK_QUOTE:
            r->quote_depth--;
            break;

        case MD_BLOCK_UL:
            r->ol_counter = 0;
            r->need_newline = 1;
            break;

        case MD_BLOCK_OL:
            r->ol_counter = 0;
            r->need_newline = 1;
            break;

        case MD_BLOCK_LI:
            r->list_depth--;
            render_newline(r);
            break;

        case MD_BLOCK_HR:
            break;

        case MD_BLOCK_H:
            render_newline(r);
            r->need_newline = 1;
            break;

        case MD_BLOCK_CODE:
            render_indent(r);
            if(r->fence_char == '~') {
                RENDER_VERBATIM(r, "~~~");
            } else {
                RENDER_VERBATIM(r, "```");
            }
            render_newline(r);
            r->in_code_block = 0;
            r->need_newline = 1;
            break;

        case MD_BLOCK_HTML:
            break;

        case MD_BLOCK_P:
            render_newline(r);
            r->need_newline = 1;
            break;

        case MD_BLOCK_TABLE:
            r->in_table = 0;
            r->need_newline = 1;
            break;

        case MD_BLOCK_THEAD:
            r->in_thead = 0;
            break;

        case MD_BLOCK_TBODY:
            break;

        case MD_BLOCK_TR:
            render_newline(r);
            if(r->in_thead && !r->thead_done) {
                render_table_separator(r);
                r->thead_done = 1;
            }
            break;

        case MD_BLOCK_TH:
            RENDER_VERBATIM(r, " |");
            r->current_col++;
            break;

        case MD_BLOCK_TD:
            RENDER_VERBATIM(r, " |");
            r->current_col++;
            break;

        case MD_BLOCK_FRONTMATTER:
            r->in_frontmatter = 0;
            break;

        case MD_BLOCK_COMPONENT: {
            const MD_BLOCK_COMPONENT_DETAIL* comp = (const MD_BLOCK_COMPONENT_DETAIL*)detail;
            render_indent(r);
            RENDER_VERBATIM(r, "</");
            if(comp->tag_name.text != NULL && comp->tag_name.size > 0)
                render_attribute(r, &comp->tag_name, render_verbatim);
            RENDER_VERBATIM(r, ">");
            render_newline(r);
            r->need_newline = 1;
            break;
        }

        case MD_BLOCK_ALERT:
            r->quote_depth--;
            r->need_newline = 1;
            break;

        case MD_BLOCK_TEMPLATE:
            break;
    }

    return 0;
}

static int
enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    MD_MARKDOWN* r = (MD_MARKDOWN*) userdata;

    switch(type) {
        case MD_SPAN_EM:
            RENDER_VERBATIM(r, "*");
            break;

        case MD_SPAN_STRONG:
            RENDER_VERBATIM(r, "**");
            break;

        case MD_SPAN_A: {
            RENDER_VERBATIM(r, "[");
            break;
        }

        case MD_SPAN_IMG: {
            RENDER_VERBATIM(r, "![");
            r->image_nesting_level++;
            break;
        }

        case MD_SPAN_CODE:
            RENDER_VERBATIM(r, "`");
            r->in_code_span = 1;
            break;

        case MD_SPAN_DEL:
            RENDER_VERBATIM(r, "~~");
            break;

        case MD_SPAN_LATEXMATH:
            RENDER_VERBATIM(r, "$");
            break;

        case MD_SPAN_LATEXMATH_DISPLAY:
            RENDER_VERBATIM(r, "$$");
            break;

        case MD_SPAN_WIKILINK:
            /* Convert wiki link to regular link: [target]( */
            RENDER_VERBATIM(r, "[");
            break;

        case MD_SPAN_U:
            /* Underline has no standard markdown — use HTML tag */
            RENDER_VERBATIM(r, "<u>");
            break;

        case MD_SPAN_COMPONENT: {
            const MD_SPAN_COMPONENT_DETAIL* comp = (const MD_SPAN_COMPONENT_DETAIL*)detail;
            RENDER_VERBATIM(r, "<");
            if(comp->tag_name.text != NULL && comp->tag_name.size > 0)
                render_attribute(r, &comp->tag_name, render_verbatim);
            RENDER_VERBATIM(r, ">");
            break;
        }

        case MD_SPAN_SPAN:
            /* Generic span — transparent, just render content */
            break;
    }

    return 0;
}

static int
leave_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    MD_MARKDOWN* r = (MD_MARKDOWN*) userdata;

    switch(type) {
        case MD_SPAN_EM:
            RENDER_VERBATIM(r, "*");
            break;

        case MD_SPAN_STRONG:
            RENDER_VERBATIM(r, "**");
            break;

        case MD_SPAN_A: {
            const MD_SPAN_A_DETAIL* a = (const MD_SPAN_A_DETAIL*)detail;
            RENDER_VERBATIM(r, "](");
            render_attribute(r, &a->href, render_verbatim);
            if(a->title.text != NULL && a->title.size > 0) {
                RENDER_VERBATIM(r, " \"");
                render_attribute(r, &a->title, render_verbatim);
                RENDER_VERBATIM(r, "\"");
            }
            RENDER_VERBATIM(r, ")");
            break;
        }

        case MD_SPAN_IMG: {
            const MD_SPAN_IMG_DETAIL* img = (const MD_SPAN_IMG_DETAIL*)detail;
            RENDER_VERBATIM(r, "](");
            render_attribute(r, &img->src, render_verbatim);
            if(img->title.text != NULL && img->title.size > 0) {
                RENDER_VERBATIM(r, " \"");
                render_attribute(r, &img->title, render_verbatim);
                RENDER_VERBATIM(r, "\"");
            }
            RENDER_VERBATIM(r, ")");
            r->image_nesting_level--;
            break;
        }

        case MD_SPAN_CODE:
            RENDER_VERBATIM(r, "`");
            r->in_code_span = 0;
            break;

        case MD_SPAN_DEL:
            RENDER_VERBATIM(r, "~~");
            break;

        case MD_SPAN_LATEXMATH:
            RENDER_VERBATIM(r, "$");
            break;

        case MD_SPAN_LATEXMATH_DISPLAY:
            RENDER_VERBATIM(r, "$$");
            break;

        case MD_SPAN_WIKILINK: {
            const MD_SPAN_WIKILINK_DETAIL* wl = (const MD_SPAN_WIKILINK_DETAIL*)detail;
            RENDER_VERBATIM(r, "](");
            render_attribute(r, &wl->target, render_verbatim);
            RENDER_VERBATIM(r, ")");
            break;
        }

        case MD_SPAN_U:
            RENDER_VERBATIM(r, "</u>");
            break;

        case MD_SPAN_COMPONENT: {
            const MD_SPAN_COMPONENT_DETAIL* comp = (const MD_SPAN_COMPONENT_DETAIL*)detail;
            RENDER_VERBATIM(r, "</");
            if(comp->tag_name.text != NULL && comp->tag_name.size > 0)
                render_attribute(r, &comp->tag_name, render_verbatim);
            RENDER_VERBATIM(r, ">");
            break;
        }

        case MD_SPAN_SPAN:
            break;
    }

    return 0;
}

static int
text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    MD_MARKDOWN* r = (MD_MARKDOWN*) userdata;

    if(r->in_frontmatter)
        return 0;

    switch(type) {
        case MD_TEXT_NULLCHAR:
            render_utf8_codepoint(r, 0xFFFD, render_verbatim);
            break;

        case MD_TEXT_BR:
            RENDER_VERBATIM(r, "\\");
            render_newline(r);
            render_indent(r);
            break;

        case MD_TEXT_SOFTBR:
            render_newline(r);
            render_indent(r);
            break;

        case MD_TEXT_HTML:
            /* Strip all raw HTML (comments, custom tags, etc.) */
            break;

        case MD_TEXT_ENTITY:
            render_entity(r, text, size, render_verbatim);
            break;

        case MD_TEXT_CODE:
            if(r->in_code_block) {
                if(size == 1 && text[0] == '\n') {
                    render_newline(r);
                    r->need_indent = 1;
                } else {
                    if(r->need_indent) {
                        render_indent(r);
                        r->need_indent = 0;
                    }
                    render_verbatim(r, text, size);
                }
            } else {
                render_verbatim(r, text, size);
            }
            break;

        default:
            render_verbatim(r, text, size);
            break;
    }

    return 0;
}

static void
debug_log_callback(const char* msg, void* userdata)
{
    MD_MARKDOWN* r = (MD_MARKDOWN*) userdata;
    if(r->flags & MD_MARKDOWN_FLAG_DEBUG)
        fprintf(stderr, "MD4X: %s\n", msg);
}

int
md_markdown(const MD_CHAR* input, MD_SIZE input_size,
             void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
             void* userdata, unsigned parser_flags, unsigned renderer_flags)
{
    MD_MARKDOWN render;
    MD_PARSER parser;

    /* Heal-before-render: run md_heal first, then render the healed output. */
    if(renderer_flags & MD_MARKDOWN_FLAG_HEAL) {
        MD4X_HEAL_BUF hbuf;
        int ret;
        if(md4x_heal_input(input, input_size, &hbuf) != 0) {
            free(hbuf.data);
            return -1;
        }
        ret = md_markdown(hbuf.data, hbuf.size, process_output, userdata,
                           parser_flags, renderer_flags & ~MD_MARKDOWN_FLAG_HEAL);
        free(hbuf.data);
        return ret;
    }

    memset(&parser, 0, sizeof(parser));
    parser.flags = parser_flags;
    parser.enter_block = enter_block_callback;
    parser.leave_block = leave_block_callback;
    parser.enter_span = enter_span_callback;
    parser.leave_span = leave_span_callback;
    parser.text = text_callback;
    parser.debug_log = debug_log_callback;

    memset(&render, 0, sizeof(render));
    render.process_output = process_output;
    render.userdata = userdata;
    render.flags = renderer_flags;

    /* Consider skipping UTF-8 byte order mark (BOM). */
    if(renderer_flags & MD_MARKDOWN_FLAG_SKIP_UTF8_BOM  &&  sizeof(MD_CHAR) == 1) {
        static const MD_CHAR bom[3] = { (char)0xef, (char)0xbb, (char)0xbf };
        if(input_size >= sizeof(bom)  &&  memcmp(input, bom, sizeof(bom)) == 0) {
            input += sizeof(bom);
            input_size -= sizeof(bom);
        }
    }

    return md_parse(input, input_size, &parser, (void*) &render);
}
