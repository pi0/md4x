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
#include <string.h>

#include "md4x-text.h"
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


typedef struct MD_TEXT_tag MD_TEXT;
struct MD_TEXT_tag {
    void (*process_output)(const MD_CHAR*, MD_SIZE, void*);
    void* userdata;
    unsigned flags;
    int image_nesting_level;
    int quote_depth;
    int list_depth;
    int ol_counter;
    int in_code_block;
    int need_newline;       /* pending newline before next block */
    int need_indent;        /* emit indent prefix on next code text */
    int li_opened;          /* just opened a list item (bullet already printed) */
    int in_frontmatter;     /* skip frontmatter content */
};


/*********************************************
 ***  Text rendering helper functions  ***
 *********************************************/

static inline void
render_verbatim(MD_TEXT* r, const MD_CHAR* text, MD_SIZE size)
{
    r->process_output(text, size, r->userdata);
}

#define RENDER_VERBATIM(r, verbatim)                                    \
        render_verbatim((r), (verbatim), (MD_SIZE) (strlen(verbatim)))

static void
render_indent(MD_TEXT* r)
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
render_newline(MD_TEXT* r)
{
    RENDER_VERBATIM(r, "\n");
}

static unsigned
hex_val(char ch)
{
    if('0' <= ch && ch <= '9')
        return ch - '0';
    if('A' <= ch && ch <= 'Z')
        return ch - 'A' + 10;
    else
        return ch - 'a' + 10;
}

static void
render_utf8_codepoint(MD_TEXT* r, unsigned codepoint,
                      void (*fn_append)(MD_TEXT*, const MD_CHAR*, MD_SIZE))
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
render_entity(MD_TEXT* r, const MD_CHAR* text, MD_SIZE size,
              void (*fn_append)(MD_TEXT*, const MD_CHAR*, MD_SIZE))
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
render_attribute(MD_TEXT* r, const MD_ATTRIBUTE* attr,
                 void (*fn_append)(MD_TEXT*, const MD_CHAR*, MD_SIZE))
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


/**************************************
 ***  Text renderer implementation  ***
 **************************************/

static int
enter_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MD_TEXT* r = (MD_TEXT*) userdata;

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
                    RENDER_VERBATIM(r, "[x] ");
                } else {
                    RENDER_VERBATIM(r, "[ ] ");
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

        case MD_BLOCK_H:
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            render_indent(r);
            break;

        case MD_BLOCK_CODE:
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            r->in_code_block = 1;
            r->need_indent = 1;
            break;

        case MD_BLOCK_HTML:
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

        case MD_BLOCK_TABLE:
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            break;

        case MD_BLOCK_THEAD:
            break;

        case MD_BLOCK_TBODY:
            break;

        case MD_BLOCK_TR:
            render_indent(r);
            break;

        case MD_BLOCK_TH:
            break;

        case MD_BLOCK_TD:
            break;

        case MD_BLOCK_FRONTMATTER:
            r->in_frontmatter = 1;
            break;

        case MD_BLOCK_COMPONENT:
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            break;

        case MD_BLOCK_ALERT: {
            const MD_BLOCK_ALERT_DETAIL* det = (const MD_BLOCK_ALERT_DETAIL*) detail;
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            r->quote_depth++;
            render_indent(r);
            if(det->type_name.text != NULL && det->type_name.size > 0)
                render_attribute(r, &det->type_name, render_verbatim);
            render_newline(r);
            break;
        }

        case MD_BLOCK_TEMPLATE:
            break;
    }

    return 0;
}

static int
leave_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MD_TEXT* r = (MD_TEXT*) userdata;

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
            r->need_newline = 1;
            break;

        case MD_BLOCK_THEAD:
            break;

        case MD_BLOCK_TBODY:
            break;

        case MD_BLOCK_TR:
            render_newline(r);
            break;

        case MD_BLOCK_TH:
            RENDER_VERBATIM(r, "\t");
            break;

        case MD_BLOCK_TD:
            RENDER_VERBATIM(r, "\t");
            break;

        case MD_BLOCK_FRONTMATTER:
            r->in_frontmatter = 0;
            break;

        case MD_BLOCK_COMPONENT:
            r->need_newline = 1;
            break;

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
    MD_TEXT* r = (MD_TEXT*) userdata;

    (void) detail;

    if(type == MD_SPAN_IMG)
        r->image_nesting_level++;

    return 0;
}

static int
leave_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    MD_TEXT* r = (MD_TEXT*) userdata;

    (void) detail;

    if(type == MD_SPAN_IMG)
        r->image_nesting_level--;

    return 0;
}

static int
text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    MD_TEXT* r = (MD_TEXT*) userdata;

    if(r->in_frontmatter)
        return 0;

    switch(type) {
        case MD_TEXT_NULLCHAR:
            render_utf8_codepoint(r, 0x0000, render_verbatim);
            break;

        case MD_TEXT_BR:
            render_newline(r);
            render_indent(r);
            break;

        case MD_TEXT_SOFTBR:
            if(r->image_nesting_level == 0) {
                render_newline(r);
                render_indent(r);
            } else {
                RENDER_VERBATIM(r, " ");
            }
            break;

        case MD_TEXT_HTML:
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
                        RENDER_VERBATIM(r, "  ");
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
    MD_TEXT* r = (MD_TEXT*) userdata;
    if(r->flags & MD_TEXT_FLAG_DEBUG)
        fprintf(stderr, "MD4X: %s\n", msg);
}

int
md_text(const MD_CHAR* input, MD_SIZE input_size,
        void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
        void* userdata, unsigned parser_flags, unsigned renderer_flags)
{
    MD_TEXT render;
    MD_PARSER parser = {
        0,
        parser_flags,
        enter_block_callback,
        leave_block_callback,
        enter_span_callback,
        leave_span_callback,
        text_callback,
        debug_log_callback,
        NULL
    };

    memset(&render, 0, sizeof(render));
    render.process_output = process_output;
    render.userdata = userdata;
    render.flags = renderer_flags;

    /* Consider skipping UTF-8 byte order mark (BOM). */
    if(renderer_flags & MD_TEXT_FLAG_SKIP_UTF8_BOM  &&  sizeof(MD_CHAR) == 1) {
        static const MD_CHAR bom[3] = { (char)0xef, (char)0xbb, (char)0xbf };
        if(input_size >= sizeof(bom)  &&  memcmp(input, bom, sizeof(bom)) == 0) {
            input += sizeof(bom);
            input_size -= sizeof(bom);
        }
    }

    return md_parse(input, input_size, &parser, (void*) &render);
}
