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

#include "md4x-ansi.h"
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


/* ANSI escape sequences */
#define ANSI_RESET          "\033[0m"
#define ANSI_BOLD           "\033[1m"
#define ANSI_BOLD_OFF       "\033[22m"
#define ANSI_DIM            "\033[2m"
#define ANSI_DIM_OFF        "\033[22m"
#define ANSI_ITALIC         "\033[3m"
#define ANSI_ITALIC_OFF     "\033[23m"
#define ANSI_UNDERLINE      "\033[4m"
#define ANSI_UNDERLINE_OFF  "\033[24m"
#define ANSI_STRIKETHROUGH  "\033[9m"
#define ANSI_STRIKE_OFF     "\033[29m"

#define ANSI_COLOR_BLUE     "\033[34m"
#define ANSI_COLOR_CYAN     "\033[36m"
#define ANSI_COLOR_MAGENTA  "\033[35m"
#define ANSI_COLOR_YELLOW   "\033[33m"
#define ANSI_COLOR_GREEN    "\033[32m"
#define ANSI_COLOR_DEFAULT  "\033[39m"

/* Compound styles */
#define ANSI_HEADING        "\033[1;35m"
#define ANSI_LINK           "\033[4;34m"
#define ANSI_LINK_URL       "\033[2;34m"

/* OSC 8 hyperlinks: \033]8;;URL\033\\ to open, \033]8;;\033\\ to close */
#define ANSI_HYPERLINK_OPEN "\033]8;;"
#define ANSI_HYPERLINK_SEP  "\033\\"
#define ANSI_HYPERLINK_CLOSE "\033]8;;\033\\"

/* Box-drawing characters (UTF-8) */
#define HORIZONTAL_RULE     "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80" \
                            "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80" \
                            "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80" \
                            "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80" \
                            "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"

/* Blockquote bar (UTF-8: vertical bar U+2502) */
#define QUOTE_BAR           "\xe2\x94\x82"


typedef struct MD_ANSI_tag MD_ANSI;
struct MD_ANSI_tag {
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
};


/*********************************************
 ***  ANSI rendering helper functions  ***
 *********************************************/

static inline void
render_verbatim(MD_ANSI* r, const MD_CHAR* text, MD_SIZE size)
{
    r->process_output(text, size, r->userdata);
}

#define RENDER_VERBATIM(r, verbatim)                                    \
        render_verbatim((r), (verbatim), (MD_SIZE) (strlen(verbatim)))

static inline void
render_ansi(MD_ANSI* r, const char* code)
{
    if(!(r->flags & MD_ANSI_FLAG_NO_COLOR))
        RENDER_VERBATIM(r, code);
}

static void
render_indent(MD_ANSI* r)
{
    int i;
    for(i = 0; i < r->quote_depth; i++) {
        render_ansi(r, ANSI_DIM);
        RENDER_VERBATIM(r, "  " QUOTE_BAR " ");
        render_ansi(r, ANSI_DIM_OFF);
    }
    for(i = 0; i < r->list_depth; i++) {
        RENDER_VERBATIM(r, "  ");
    }
}

static void
render_newline(MD_ANSI* r)
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
render_utf8_codepoint(MD_ANSI* r, unsigned codepoint,
                      void (*fn_append)(MD_ANSI*, const MD_CHAR*, MD_SIZE))
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
render_entity(MD_ANSI* r, const MD_CHAR* text, MD_SIZE size,
              void (*fn_append)(MD_ANSI*, const MD_CHAR*, MD_SIZE))
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
render_attribute(MD_ANSI* r, const MD_ATTRIBUTE* attr,
                 void (*fn_append)(MD_ANSI*, const MD_CHAR*, MD_SIZE))
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
 ***  ANSI renderer implementation  ***
 **************************************/

static int
enter_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MD_ANSI* r = (MD_ANSI*) userdata;

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
                    render_ansi(r, ANSI_COLOR_GREEN);
                    RENDER_VERBATIM(r, "[x] ");
                    render_ansi(r, ANSI_COLOR_DEFAULT);
                } else {
                    RENDER_VERBATIM(r, "[ ] ");
                }
            } else {
                /* Check parent: is this inside OL or UL? We track via ol_counter. */
                if(r->ol_counter > 0) {
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%d. ", r->ol_counter);
                    render_ansi(r, ANSI_DIM);
                    RENDER_VERBATIM(r, buf);
                    render_ansi(r, ANSI_DIM_OFF);
                    r->ol_counter++;
                } else {
                    render_ansi(r, ANSI_DIM);
                    RENDER_VERBATIM(r, "* ");
                    render_ansi(r, ANSI_DIM_OFF);
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
            render_ansi(r, ANSI_DIM);
            RENDER_VERBATIM(r, HORIZONTAL_RULE);
            render_ansi(r, ANSI_DIM_OFF);
            render_newline(r);
            r->need_newline = 1;
            break;

        case MD_BLOCK_H:
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            render_indent(r);
            render_ansi(r, ANSI_HEADING);
            break;

        case MD_BLOCK_CODE:
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            r->in_code_block = 1;
            r->need_indent = 1;
            render_ansi(r, ANSI_DIM);
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
            render_ansi(r, ANSI_BOLD);
            break;

        case MD_BLOCK_TD:
            break;

        case MD_BLOCK_FRONTMATTER:
            render_ansi(r, ANSI_DIM);
            break;

        case MD_BLOCK_COMPONENT:
            if(r->need_newline) {
                render_newline(r);
                r->need_newline = 0;
            }
            render_ansi(r, ANSI_COLOR_CYAN);
            break;
    }

    return 0;
}

static int
leave_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MD_ANSI* r = (MD_ANSI*) userdata;

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
            render_ansi(r, ANSI_RESET);
            render_newline(r);
            r->need_newline = 1;
            break;

        case MD_BLOCK_CODE:
            render_ansi(r, ANSI_DIM_OFF);
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
            render_indent(r);
            render_ansi(r, ANSI_DIM);
            RENDER_VERBATIM(r, HORIZONTAL_RULE);
            render_ansi(r, ANSI_DIM_OFF);
            render_newline(r);
            break;

        case MD_BLOCK_TBODY:
            break;

        case MD_BLOCK_TR:
            render_newline(r);
            break;

        case MD_BLOCK_TH:
            render_ansi(r, ANSI_BOLD_OFF);
            RENDER_VERBATIM(r, "\t");
            break;

        case MD_BLOCK_TD:
            RENDER_VERBATIM(r, "\t");
            break;

        case MD_BLOCK_FRONTMATTER:
            render_ansi(r, ANSI_DIM_OFF);
            r->need_newline = 1;
            break;

        case MD_BLOCK_COMPONENT:
            render_ansi(r, ANSI_COLOR_DEFAULT);
            r->need_newline = 1;
            break;
    }

    return 0;
}

static int
enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    MD_ANSI* r = (MD_ANSI*) userdata;

    if(type == MD_SPAN_IMG)
        r->image_nesting_level++;

    if(r->image_nesting_level > 0 && type != MD_SPAN_IMG)
        return 0;

    switch(type) {
        case MD_SPAN_EM:                render_ansi(r, ANSI_ITALIC); break;
        case MD_SPAN_STRONG:            render_ansi(r, ANSI_BOLD); break;
        case MD_SPAN_U:                 render_ansi(r, ANSI_UNDERLINE); break;
        case MD_SPAN_A: {
            const MD_SPAN_A_DETAIL* a = (const MD_SPAN_A_DETAIL*) detail;
            /* OSC 8 hyperlink: makes text clickable in supported terminals */
            if(!(r->flags & MD_ANSI_FLAG_NO_COLOR) && a->href.size > 0) {
                RENDER_VERBATIM(r, ANSI_HYPERLINK_OPEN);
                render_attribute(r, &a->href, render_verbatim);
                RENDER_VERBATIM(r, ANSI_HYPERLINK_SEP);
            }
            render_ansi(r, ANSI_LINK);
            break;
        }
        case MD_SPAN_IMG:
            render_ansi(r, ANSI_DIM);
            RENDER_VERBATIM(r, "[image: ");
            break;
        case MD_SPAN_CODE:              render_ansi(r, ANSI_COLOR_CYAN); break;
        case MD_SPAN_DEL:               render_ansi(r, ANSI_STRIKETHROUGH); break;
        case MD_SPAN_LATEXMATH:         render_ansi(r, ANSI_COLOR_YELLOW); break;
        case MD_SPAN_LATEXMATH_DISPLAY: render_ansi(r, ANSI_COLOR_YELLOW); break;
        case MD_SPAN_WIKILINK:          render_ansi(r, ANSI_LINK); break;
        case MD_SPAN_COMPONENT:         render_ansi(r, ANSI_COLOR_CYAN); break;
        case MD_SPAN_SPAN:              break;  /* Transparent: no special styling */
    }

    return 0;
}

static int
leave_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    MD_ANSI* r = (MD_ANSI*) userdata;

    if(type == MD_SPAN_IMG)
        r->image_nesting_level--;

    if(r->image_nesting_level > 0)
        return 0;

    switch(type) {
        case MD_SPAN_EM:                render_ansi(r, ANSI_ITALIC_OFF); break;
        case MD_SPAN_STRONG:            render_ansi(r, ANSI_BOLD_OFF); break;
        case MD_SPAN_U:                 render_ansi(r, ANSI_UNDERLINE_OFF); break;
        case MD_SPAN_A: {
            const MD_SPAN_A_DETAIL* a = (const MD_SPAN_A_DETAIL*) detail;
            render_ansi(r, ANSI_RESET);
            /* Close OSC 8 hyperlink */
            if(!(r->flags & MD_ANSI_FLAG_NO_COLOR) && a->href.size > 0)
                RENDER_VERBATIM(r, ANSI_HYPERLINK_CLOSE);
            /* Show URL as dim fallback for terminals without OSC 8 */
            if(a->href.size > 0 && !a->is_autolink) {
                render_ansi(r, ANSI_LINK_URL);
                RENDER_VERBATIM(r, " (");
                render_attribute(r, &a->href, render_verbatim);
                RENDER_VERBATIM(r, ")");
                render_ansi(r, ANSI_RESET);
            }
            break;
        }
        case MD_SPAN_IMG:
            RENDER_VERBATIM(r, "]");
            render_ansi(r, ANSI_DIM_OFF);
            break;
        case MD_SPAN_CODE:              render_ansi(r, ANSI_COLOR_DEFAULT); break;
        case MD_SPAN_DEL:               render_ansi(r, ANSI_STRIKE_OFF); break;
        case MD_SPAN_LATEXMATH:         render_ansi(r, ANSI_COLOR_DEFAULT); break;
        case MD_SPAN_LATEXMATH_DISPLAY: render_ansi(r, ANSI_COLOR_DEFAULT); break;
        case MD_SPAN_WIKILINK:          render_ansi(r, ANSI_RESET); break;
        case MD_SPAN_COMPONENT:         render_ansi(r, ANSI_COLOR_DEFAULT); break;
        case MD_SPAN_SPAN:              break;  /* Transparent: no special styling */
    }

    return 0;
}

static int
text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    MD_ANSI* r = (MD_ANSI*) userdata;

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
            /* Raw HTML: output verbatim in terminal */
            render_verbatim(r, text, size);
            break;

        case MD_TEXT_ENTITY:
            render_entity(r, text, size, render_verbatim);
            break;

        case MD_TEXT_CODE:
            if(r->in_code_block) {
                /* Inside code block: the parser sends each line and its \n
                 * as separate callbacks. We use need_indent to track when
                 * we need to emit the indent prefix at line start. */
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
                /* Inline code span */
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
    MD_ANSI* r = (MD_ANSI*) userdata;
    if(r->flags & MD_ANSI_FLAG_DEBUG)
        fprintf(stderr, "MD4X: %s\n", msg);
}

int
md_ansi(const MD_CHAR* input, MD_SIZE input_size,
        void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
        void* userdata, unsigned parser_flags, unsigned renderer_flags)
{
    MD_ANSI render;
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
    if(renderer_flags & MD_ANSI_FLAG_SKIP_UTF8_BOM  &&  sizeof(MD_CHAR) == 1) {
        static const MD_CHAR bom[3] = { (char)0xef, (char)0xbb, (char)0xbf };
        if(input_size >= sizeof(bom)  &&  memcmp(input, bom, sizeof(bom)) == 0) {
            input += sizeof(bom);
            input_size -= sizeof(bom);
        }
    }

    return md_parse(input, input_size, &parser, (void*) &render);
}
