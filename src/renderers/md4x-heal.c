/*
 * MD4X: Markdown parser for C
 * (http://github.com/pi0/md4x)
 *
 * Copyright (c) 2026 Pooya Parsa <pooya@pi0.io>
 *
 * Based on logic from https://github.com/vercel/streamdown/tree/main/packages/remend
 * Written by Hayden Bleasel <https://github.com/haydenbleasel>
 * Copyright (c) 2023 Vercel Inc.
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

#include "md4x-heal.h"


#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199409L
    #if defined __GNUC__
        #define inline __inline__
    #elif defined _MSC_VER
        #define inline __inline
    #else
        #define inline
    #endif
#endif


/***************************
 ***  Growable buffer    ***
 ***************************/

typedef struct {
    char* data;
    unsigned size;
    unsigned cap;
} HEAL_BUF;

static inline void
buf_init(HEAL_BUF* buf, unsigned initial_cap)
{
    buf->data = (char*) malloc(initial_cap);
    buf->size = 0;
    buf->cap = buf->data ? initial_cap : 0;
}

static inline void
buf_append(HEAL_BUF* buf, const char* s, unsigned len)
{
    if(len == 0) return;
    if(buf->size + len > buf->cap) {
        unsigned new_cap = buf->cap + buf->cap / 2 + len + 64;
        char* p = (char*) realloc(buf->data, new_cap);
        if(!p) return;
        buf->data = p;
        buf->cap = new_cap;
    }
    memcpy(buf->data + buf->size, s, len);
    buf->size += len;
}

static inline void
buf_append_ch(HEAL_BUF* buf, char c)
{
    buf_append(buf, &c, 1);
}

static inline void
buf_free(HEAL_BUF* buf)
{
    free(buf->data);
    buf->data = NULL;
    buf->size = 0;
    buf->cap = 0;
}


/***************************
 ***  Utility helpers    ***
 ***************************/

static inline int
is_word_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

static inline int
is_escaped(const char* text, unsigned pos)
{
    unsigned n = 0;
    while(pos > 0 && text[pos - 1] == '\\') {
        n++;
        pos--;
    }
    return (n % 2) != 0;
}

/* Check if position i is part of a ``` sequence */
static inline int
is_triple_backtick(const char* text, unsigned size, unsigned i)
{
    /* Check if i is the start of ``` */
    if(i + 2 < size && text[i] == '`' && text[i+1] == '`' && text[i+2] == '`')
        return 1;
    /* Check if i is the middle of ``` */
    if(i >= 1 && i + 1 < size && text[i-1] == '`' && text[i] == '`' && text[i+1] == '`')
        return 1;
    /* Check if i is the end of ``` */
    if(i >= 2 && text[i-2] == '`' && text[i-1] == '`' && text[i] == '`')
        return 1;
    return 0;
}

/* Find the start of the current line (returns index after preceding newline) */
static inline unsigned
line_start(const char* text, unsigned pos)
{
    while(pos > 0 && text[pos - 1] != '\n')
        pos--;
    return pos;
}

/* Find the end of the current line (returns index of newline or size) */
static inline unsigned
line_end(const char* text, unsigned size, unsigned pos)
{
    while(pos < size && text[pos] != '\n')
        pos++;
    return pos;
}

/* Check if a line is a horizontal rule (3+ of same marker with only whitespace) */
static inline int
is_horizontal_rule(const char* text, unsigned size, unsigned marker_pos, char marker)
{
    unsigned ls = line_start(text, marker_pos);
    unsigned le = line_end(text, size, marker_pos);
    unsigned count = 0;
    unsigned i;
    for(i = ls; i < le; i++) {
        if(text[i] == marker) {
            count++;
        } else if(text[i] != ' ' && text[i] != '\t') {
            return 0;
        }
    }
    return count >= 3;
}


/***************************
 ***  Context tracking   ***
 ***************************/

/* Count triple-backtick fences up to a position to determine code block state */
static int
in_fenced_code_block(const char* text, unsigned size, unsigned pos)
{
    int inside = 0;
    unsigned i = 0;
    (void) size;
    while(i < pos) {
        if(text[i] == '`' && i + 2 < pos && text[i+1] == '`' && text[i+2] == '`') {
            if(!is_escaped(text, i))
                inside = !inside;
            i += 3;
            /* Skip rest of opening fence line */
            while(i < pos && text[i] != '\n') i++;
        } else {
            i++;
        }
    }
    return inside;
}

/* Count ``` sequences in the entire text */
static unsigned
count_fences(const char* text, unsigned size)
{
    unsigned count = 0;
    unsigned i = 0;
    while(i < size) {
        if(text[i] == '`' && i + 2 < size && text[i+1] == '`' && text[i+2] == '`') {
            if(!is_escaped(text, i))
                count++;
            i += 3;
            while(i < size && text[i] == '`') i++;
        } else {
            i++;
        }
    }
    return count;
}

/* Count single backticks that are NOT part of ``` and NOT escaped */
static unsigned
count_single_backticks(const char* text, unsigned size)
{
    unsigned count = 0;
    unsigned i;
    for(i = 0; i < size; i++) {
        if(text[i] == '`' && !is_escaped(text, i) && !is_triple_backtick(text, size, i))
            count++;
    }
    return count;
}

/* Check if position is inside a completed inline code span */
static int
in_complete_inline_code(const char* text, unsigned size, unsigned pos)
{
    unsigned i;
    int in_code = 0;
    unsigned code_start = 0;
    for(i = 0; i < size; i++) {
        if(text[i] == '`' && !is_escaped(text, i) && !is_triple_backtick(text, size, i)) {
            if(!in_code) {
                in_code = 1;
                code_start = i;
            } else {
                /* Found closing backtick */
                if(pos > code_start && pos < i)
                    return 1;
                in_code = 0;
            }
        }
    }
    return 0;
}

/* Check if position is inside $...$ or $$...$$ */
static int
in_math_block(const char* text, unsigned size, unsigned pos)
{
    int in_block = 0;     /* inside $$ */
    int in_inline = 0;    /* inside $ */
    unsigned i;
    for(i = 0; i < size && i < pos; i++) {
        if(text[i] == '\\') { i++; continue; }
        if(text[i] == '$') {
            if(i + 1 < size && text[i+1] == '$') {
                in_block = !in_block;
                i++;
            } else if(!in_block) {
                in_inline = !in_inline;
            }
        }
    }
    return in_block || in_inline;
}

/* Check if position is inside a link/image URL: ](... */
static int
in_link_url(const char* text, unsigned pos)
{
    unsigned i;
    if(pos == 0) return 0;
    for(i = pos; i > 0; i--) {
        if(text[i - 1] == '\n') return 0;
        if(text[i - 1] == ')') return 0;
        if(text[i - 1] == '(') {
            if(i >= 2 && text[i - 2] == ']')
                return 1;
            return 0;
        }
    }
    return 0;
}

/* Check if position is inside an HTML tag: <...> */
static int
in_html_tag(const char* text, unsigned pos)
{
    unsigned i;
    if(pos == 0) return 0;
    for(i = pos; i > 0; i--) {
        if(text[i - 1] == '\n') return 0;
        if(text[i - 1] == '>') return 0;
        if(text[i - 1] == '<') {
            if(i < pos) {
                char next = text[i];
                return (next >= 'a' && next <= 'z') ||
                       (next >= 'A' && next <= 'Z') || next == '/';
            }
            return 0;
        }
    }
    return 0;
}


/***************************
 ***  Setext headings    ***
 ***************************/

/* If the last line is just 1-2 dashes or equals, append ZWS to prevent
 * it from being interpreted as a setext heading underline. */
static void
heal_setext_heading(HEAL_BUF* buf)
{
    unsigned ls, le, i;
    char marker;
    int count;
    unsigned prev_le;

    if(buf->size == 0) return;

    /* Find last line */
    le = buf->size;
    ls = le;
    while(ls > 0 && buf->data[ls - 1] != '\n') ls--;

    /* Check if last line is 1-2 of same marker */
    if(le - ls < 1 || le - ls > 2) return;
    marker = buf->data[ls];
    if(marker != '-' && marker != '=') return;
    count = 1;
    for(i = ls + 1; i < le; i++) {
        if(buf->data[i] != marker) return;
        count++;
    }
    if(count > 2) return; /* 3+ is a valid thematic break / heading */

    /* Check that there is a previous line with content */
    if(ls == 0) return;
    prev_le = ls - 1;
    if(prev_le == 0) return;
    i = prev_le;
    while(i > 0 && buf->data[i - 1] != '\n') i--;
    /* Check previous line is not empty */
    if(i == prev_le) return;

    /* Append zero-width space */
    buf_append(buf, "\xE2\x80\x8B", 3);
}


/***************************
 ***  Code block heal    ***
 ***************************/

/* If there's an unclosed fenced code block, close it */
static void
heal_code_block(HEAL_BUF* buf)
{
    unsigned fences = count_fences(buf->data, buf->size);
    if(fences % 2 != 0) {
        /* Need to close the code block */
        if(buf->size > 0 && buf->data[buf->size - 1] != '\n')
            buf_append_ch(buf, '\n');
        buf_append(buf, "```", 3);
    }
}


/***************************
 ***  Inline code heal   ***
 ***************************/

/* If there's an unclosed inline code backtick, close it */
static void
heal_inline_code(HEAL_BUF* buf)
{
    unsigned fences, singles;

    /* Don't heal inside an unclosed fenced code block */
    fences = count_fences(buf->data, buf->size);
    if(fences % 2 != 0) return;

    singles = count_single_backticks(buf->data, buf->size);
    if(singles % 2 != 0) {
        buf_append_ch(buf, '`');
    }
}


/***************************
 ***  Emphasis healing   ***
 ***************************/

/* Counting functions matching remend's approach.
 * These count delimiter sequences while tracking fenced code blocks inline. */

/* Count ** pairs greedily (consumes 2 chars at a time), skipping code blocks */
static unsigned
count_double_asterisks(const char* text, unsigned size)
{
    unsigned count = 0;
    unsigned i;
    int in_code = 0;
    for(i = 0; i < size; i++) {
        if(text[i] == '`' && i + 2 < size && text[i+1] == '`' && text[i+2] == '`') {
            in_code = !in_code;
            i += 2;
            continue;
        }
        if(in_code) continue;
        if(text[i] == '*' && i + 1 < size && text[i+1] == '*') {
            count++;
            i++; /* skip second * */
        }
    }
    return count;
}

/* Count __ pairs greedily, skipping code blocks */
static unsigned
count_double_underscores(const char* text, unsigned size)
{
    unsigned count = 0;
    unsigned i;
    int in_code = 0;
    for(i = 0; i < size; i++) {
        if(text[i] == '`' && i + 2 < size && text[i+1] == '`' && text[i+2] == '`') {
            in_code = !in_code;
            i += 2;
            continue;
        }
        if(in_code) continue;
        if(text[i] == '_' && i + 1 < size && text[i+1] == '_') {
            count++;
            i++;
        }
    }
    return count;
}

/* Count *** triples, handling consecutive asterisk runs.
 * *** = 1, **** = 1, ***** = 1, ****** = 2, etc. */
static unsigned
count_triple_asterisks(const char* text, unsigned size)
{
    unsigned count = 0;
    unsigned consecutive = 0;
    unsigned i;
    int in_code = 0;
    for(i = 0; i < size; i++) {
        if(text[i] == '`' && i + 2 < size && text[i+1] == '`' && text[i+2] == '`') {
            if(consecutive >= 3) count += consecutive / 3;
            consecutive = 0;
            in_code = !in_code;
            i += 2;
            continue;
        }
        if(in_code) continue;
        if(text[i] == '*') {
            consecutive++;
        } else {
            if(consecutive >= 3) count += consecutive / 3;
            consecutive = 0;
        }
    }
    if(consecutive >= 3) count += consecutive / 3;
    return count;
}

/* Count single asterisks matching remend's logic:
 * - Skip escaped, in code blocks, in math
 * - First * in *** counts as single (can close italic)
 * - First * in ** does NOT count
 * - Skip word-internal and whitespace-flanked */
static unsigned
count_single_asterisks(const char* text, unsigned size)
{
    unsigned count = 0;
    unsigned i;
    int in_code = 0;
    for(i = 0; i < size; i++) {
        if(text[i] == '`' && i + 2 < size && text[i+1] == '`' && text[i+2] == '`') {
            in_code = !in_code;
            i += 2;
            continue;
        }
        if(in_code) continue;
        if(text[i] != '*') continue;

        {
            char prev = (i > 0) ? text[i - 1] : 0;
            char next = (i + 1 < size) ? text[i + 1] : 0;

            if(prev == '\\') continue;
            if(in_math_block(text, size, i)) continue;

            /* Special handling for *** sequences */
            if(prev != '*' && next == '*') {
                char next2 = (i + 2 < size) ? text[i + 2] : 0;
                if(next2 == '*') {
                    /* First * in ***: count it (can close italic) */
                    count++;
                    continue;
                }
                /* First * in **: skip */
                continue;
            }
            /* Second or later * in a sequence: skip */
            if(prev == '*') continue;

            /* Word-internal: skip */
            if(is_word_char(prev) && is_word_char(next)) continue;

            /* Whitespace flanked on both sides: skip */
            {
                int prev_ws = (prev == 0 || prev == ' ' || prev == '\t' || prev == '\n');
                int next_ws = (next == 0 || next == ' ' || next == '\t' || next == '\n');
                if(prev_ws && next_ws) continue;
            }

            count++;
        }
    }
    return count;
}

/* Count single underscores (not part of __, not escaped, not word-internal) */
static unsigned
count_single_underscores(const char* text, unsigned size)
{
    unsigned count = 0;
    unsigned i;
    int in_code = 0;
    for(i = 0; i < size; i++) {
        if(text[i] == '`' && i + 2 < size && text[i+1] == '`' && text[i+2] == '`') {
            in_code = !in_code;
            i += 2;
            continue;
        }
        if(in_code) continue;
        if(text[i] != '_') continue;

        {
            char prev = (i > 0) ? text[i - 1] : 0;
            char next = (i + 1 < size) ? text[i + 1] : 0;

            if(prev == '\\') continue;
            if(in_math_block(text, size, i)) continue;
            if(in_link_url(text, i)) continue;
            if(in_html_tag(text, i)) continue;
            if(prev == '_' || next == '_') continue;
            if(is_word_char(prev) && is_word_char(next)) continue;
        }

        count++;
    }
    return count;
}

/* Count ~~ pairs, skipping code blocks */
static unsigned
count_double_tildes(const char* text, unsigned size)
{
    unsigned count = 0;
    unsigned i;
    int in_code = 0;
    for(i = 0; i < size; i++) {
        if(text[i] == '`' && i + 2 < size && text[i+1] == '`' && text[i+2] == '`') {
            in_code = !in_code;
            i += 2;
            continue;
        }
        if(in_code) continue;
        if(text[i] == '~' && i + 1 < size && text[i+1] == '~') {
            count++;
            i++;
        }
    }
    return count;
}

/* Count $$ pairs, skipping code blocks */
static unsigned
count_double_dollars(const char* text, unsigned size)
{
    unsigned count = 0;
    unsigned i;
    int in_code = 0;
    for(i = 0; i < size; i++) {
        if(text[i] == '`' && i + 2 < size && text[i+1] == '`' && text[i+2] == '`') {
            in_code = !in_code;
            i += 2;
            continue;
        }
        if(in_code) continue;
        if(text[i] == '$' && i + 1 < size && text[i+1] == '$') {
            count++;
            i++;
        }
    }
    return count;
}


/* Check if content between markers is meaningful (not just whitespace/markers) */
static int
has_meaningful_content(const char* text, unsigned start, unsigned end)
{
    unsigned i;
    for(i = start; i < end; i++) {
        char c = text[i];
        if(c != ' ' && c != '\t' && c != '\n' && c != '\r' &&
           c != '*' && c != '_' && c != '~' && c != '`')
            return 1;
    }
    return 0;
}

/* Match bold pattern at end of text: (**)(non-* text, optionally ending with *)$
 * Returns the index of the ** opener, or size if no match. */
static unsigned
match_bold_at_end(const char* text, unsigned size)
{
    unsigned i;
    if(size < 3) return size;

    /* Scan backwards from end to find the last ** that's followed by non-* content */
    for(i = size - 1; i >= 2; i--) {
        if(text[i - 1] == '*' && text[i - 2] == '*') {
            /* Check nothing before this is * (not part of ***) */
            if(i >= 3 && text[i - 3] == '*') continue;
            /* Content after must not start with * */
            if(text[i] == '*') continue;
            /* Verify remaining content (from i to end) has no ** (is non-star) */
            {
                unsigned j;
                int has_double_star = 0;
                for(j = i; j + 1 < size; j++) {
                    if(text[j] == '*' && text[j+1] == '*') { has_double_star = 1; break; }
                }
                if(!has_double_star) return i - 2;
            }
        }
        if(i == 2) break;
    }
    return size;
}

/* Heal bold (**) — uses remend's boldPattern: /(\*\*)([^*]*\*?)$/ */
static void
heal_bold(HEAL_BUF* buf)
{
    const char* text = buf->data;
    unsigned size = buf->size;
    unsigned marker_pos;
    unsigned pairs;

    if(in_fenced_code_block(text, size, size)) return;

    marker_pos = match_bold_at_end(text, size);
    if(marker_pos >= size) return;

    /* Check not in code block or complete inline code */
    if(in_complete_inline_code(text, size, marker_pos)) return;

    /* Check content after marker is meaningful */
    if(!has_meaningful_content(text, marker_pos + 2, size)) return;

    /* Check not a horizontal rule */
    if(is_horizontal_rule(text, size, marker_pos, '*')) return;

    pairs = count_double_asterisks(text, size);
    if(pairs % 2 != 0) {
        /* Half-complete: **content* → **content** */
        if(text[size - 1] == '*' && size > marker_pos + 3) {
            buf_append_ch(buf, '*');
        } else {
            buf_append(buf, "**", 2);
        }
    }
}

/* Heal italic (*) */
static void
heal_italic_asterisk(HEAL_BUF* buf)
{
    const char* text = buf->data;
    unsigned size = buf->size;
    unsigned singles;

    if(in_fenced_code_block(text, size, size)) return;

    singles = count_single_asterisks(text, size);
    if(singles % 2 != 0) {
        buf_append_ch(buf, '*');
    }
}

/* Heal italic (__) */
static void
heal_italic_double_underscore(HEAL_BUF* buf)
{
    const char* text = buf->data;
    unsigned size = buf->size;
    unsigned pairs;

    if(in_fenced_code_block(text, size, size)) return;

    /* Half-complete: __text_ at end -> __text__ */
    if(size >= 4 && text[size - 1] == '_' &&
       text[size - 2] != '_' && text[size - 2] != '\\') {
        pairs = count_double_underscores(text, size);
        if(pairs % 2 != 0) {
            buf_append_ch(buf, '_');
            return;
        }
    }

    pairs = count_double_underscores(text, size);
    if(pairs % 2 != 0) {
        buf_append(buf, "__", 2);
    }
}

/* Heal italic (_) */
static void
heal_italic_underscore(HEAL_BUF* buf)
{
    const char* text = buf->data;
    unsigned size = buf->size;
    unsigned singles;

    if(in_fenced_code_block(text, size, size)) return;

    singles = count_single_underscores(text, size);
    if(singles % 2 != 0) {
        /* Append before trailing newlines */
        unsigned end = size;
        while(end > 0 && buf->data[end - 1] == '\n') end--;

        if(end < size) {
            /* Insert _ before trailing newlines */
            char* tail = (char*) malloc(size - end);
            if(!tail) return;
            memcpy(tail, buf->data + end, size - end);
            buf->size = end;
            buf_append_ch(buf, '_');
            buf_append(buf, tail, size - end);
            free(tail);
        } else {
            buf_append_ch(buf, '_');
        }
    }
}

/* Heal bold-italic (***) */
static void
heal_bold_italic(HEAL_BUF* buf)
{
    const char* text = buf->data;
    unsigned size = buf->size;
    unsigned triples;

    if(in_fenced_code_block(text, size, size)) return;

    /* Skip if text is all asterisks (e.g., ****) */
    {
        unsigned i;
        int all_stars = 1;
        for(i = 0; i < size; i++) {
            if(text[i] != '*') { all_stars = 0; break; }
        }
        if(all_stars) return;
    }

    triples = count_triple_asterisks(text, size);
    if(triples % 2 != 0) {
        /* Check if ** and * are independently balanced
         * If so, *** is overlapping closers, not opening bold-italic */
        unsigned doubles = count_double_asterisks(text, size);
        unsigned singles = count_single_asterisks(text, size);
        if(doubles % 2 == 0 && singles % 2 == 0) return;
        buf_append(buf, "***", 3);
    }
}


/***************************
 ***  Strikethrough heal ***
 ***************************/

static void
heal_strikethrough(HEAL_BUF* buf)
{
    const char* text = buf->data;
    unsigned size = buf->size;
    unsigned pairs;

    if(in_fenced_code_block(text, size, size)) return;

    /* Half-complete: ~~text~ at end -> ~~text~~ */
    if(size >= 4 && text[size - 1] == '~' &&
       text[size - 2] != '~' && text[size - 2] != '\\') {
        unsigned i = size - 2;
        while(i > 0) {
            if(text[i] == '~' && i > 0 && text[i-1] == '~') {
                if(has_meaningful_content(text, i + 1, size - 1)) {
                    buf_append_ch(buf, '~');
                    return;
                }
            }
            i--;
        }
    }

    pairs = count_double_tildes(text, size);
    if(pairs % 2 != 0) {
        /* Verify there's content after the opening ~~ */
        unsigned i;
        for(i = size; i >= 2; i--) {
            if(text[i-2] == '~' && text[i-1] == '~') {
                if(i < size && has_meaningful_content(text, i, size)) {
                    buf_append(buf, "~~", 2);
                    return;
                }
            }
        }
    }
}


/***************************
 ***  KaTeX heal         ***
 ***************************/

static void
heal_katex(HEAL_BUF* buf)
{
    const char* text = buf->data;
    unsigned size = buf->size;
    unsigned pairs;

    if(in_fenced_code_block(text, size, size)) return;

    pairs = count_double_dollars(text, size);
    if(pairs % 2 != 0) {
        /* Check if the $$ content contains a newline (block math) */
        unsigned i;
        int has_newline = 0;
        /* Find the opening $$ */
        for(i = size; i >= 2; i--) {
            if(text[i-2] == '$' && text[i-1] == '$') {
                unsigned j;
                for(j = i; j < size; j++) {
                    if(text[j] == '\n') { has_newline = 1; break; }
                }
                break;
            }
        }
        if(has_newline) {
            if(size > 0 && text[size - 1] != '\n')
                buf_append_ch(buf, '\n');
        }
        buf_append(buf, "$$", 2);
    }
}


/***************************
 ***  Link/image heal    ***
 ***************************/

/* Find matching opening bracket going backwards from closeIndex */
static int
find_matching_open_bracket(const char* text, unsigned close_idx)
{
    int depth = 0;
    int i;
    for(i = (int)close_idx; i >= 0; i--) {
        if(text[i] == ']') depth++;
        else if(text[i] == '[') {
            depth--;
            if(depth == 0) return i;
        }
    }
    return -1;
}

static void
heal_links_and_images(HEAL_BUF* buf)
{
    const char* text = buf->data;
    unsigned size = buf->size;
    int i;

    if(in_fenced_code_block(text, size, size)) return;

    /* Case 1: Incomplete URL — [text](url  (no closing paren) */
    for(i = (int)size - 1; i >= 1; i--) {
        if(text[i] == '(' && text[i - 1] == ']') {
            /* Found ]( — check if there's a closing ) after */
            unsigned j;
            int has_close = 0;
            for(j = (unsigned)(i + 1); j < size; j++) {
                if(text[j] == ')') { has_close = 1; break; }
                if(text[j] == '\n') break;
            }
            if(!has_close) {
                /* Find matching [ */
                int open = find_matching_open_bracket(text, (unsigned)(i - 1));
                if(open >= 0) {
                    int is_image = (open > 0 && text[open - 1] == '!');
                    if(is_image) {
                        /* Remove entire image markup */
                        unsigned img_start = (unsigned)(open - 1);
                        buf->size = img_start;
                        /* Trim trailing whitespace */
                        while(buf->size > 0 && (buf->data[buf->size - 1] == ' ' || buf->data[buf->size - 1] == '\t'))
                            buf->size--;
                    } else {
                        /* Complete the link with a placeholder */
                        buf->size = (unsigned)(i + 1); /* keep ]( */
                        buf_append(buf, ")", 1);
                    }
                    return;
                }
            }
            break;
        }
    }

    /* Case 2: Incomplete text — [text  (no closing ]) */
    for(i = (int)size - 1; i >= 0; i--) {
        if(text[i] == '[' && !is_escaped(text, (unsigned)i)) {
            /* Check this [ doesn't have a matching ] */
            unsigned j;
            int has_close = 0;
            for(j = (unsigned)(i + 1); j < size; j++) {
                if(text[j] == ']' && !is_escaped(text, j)) { has_close = 1; break; }
            }
            if(!has_close) {
                int is_image = (i > 0 && text[i - 1] == '!');
                if(is_image) {
                    /* Remove entire image start */
                    buf->size = (unsigned)(i - 1);
                    while(buf->size > 0 && (buf->data[buf->size - 1] == ' ' || buf->data[buf->size - 1] == '\t'))
                        buf->size--;
                } else {
                    /* Just remove the opening [ */
                    unsigned after = (unsigned)(i + 1);
                    memmove(buf->data + i, buf->data + after, buf->size - after);
                    buf->size -= 1;
                }
                return;
            }
        }
    }
}


/***************************
 ***  HTML tag heal      ***
 ***************************/

static void
heal_html_tag(HEAL_BUF* buf)
{
    const char* text = buf->data;
    unsigned size = buf->size;
    int i;

    /* Find unclosed < at end */
    for(i = (int)size - 1; i >= 0; i--) {
        if(text[i] == '>') return; /* Tag is closed */
        if(text[i] == '\n') return; /* Tags don't span lines */
        if(text[i] == '<') {
            /* Verify it looks like a tag (next char is letter or /) */
            if((unsigned)(i + 1) < size) {
                char next = text[i + 1];
                if((next >= 'a' && next <= 'z') ||
                   (next >= 'A' && next <= 'Z') || next == '/') {
                    /* Check not inside code block */
                    if(!in_fenced_code_block(text, size, (unsigned)i)) {
                        buf->size = (unsigned)i;
                        /* Trim trailing whitespace */
                        while(buf->size > 0 &&
                              (buf->data[buf->size - 1] == ' ' || buf->data[buf->size - 1] == '\t'))
                            buf->size--;
                    }
                }
            }
            return;
        }
    }
}


/***************************
 ***  Comparison ops     ***
 ***************************/

/* Escape > as \> in list items where it's a comparison operator, not a blockquote.
 * Pattern: list-marker + space + > + optional = + digit */
static void
heal_comparison_operators(HEAL_BUF* buf)
{
    const char* text = buf->data;
    unsigned size = buf->size;
    unsigned i = 0;

    while(i < size) {
        /* Find start of line */
        unsigned ls = i;

        /* Skip whitespace */
        while(i < size && (text[i] == ' ' || text[i] == '\t')) i++;

        /* Check for list marker */
        int is_list = 0;
        if(i < size) {
            if(text[i] == '-' || text[i] == '*' || text[i] == '+') {
                unsigned after = i + 1;
                if(after < size && text[after] == ' ') {
                    is_list = 1;
                    i = after + 1;
                }
            } else if(text[i] >= '0' && text[i] <= '9') {
                unsigned j = i;
                while(j < size && text[j] >= '0' && text[j] <= '9') j++;
                if(j < size && (text[j] == '.' || text[j] == ')')) {
                    j++;
                    if(j < size && text[j] == ' ') {
                        is_list = 1;
                        i = j + 1;
                    }
                }
            }
        }

        if(is_list && i < size && text[i] == '>') {
            /* Check what follows > */
            unsigned gt_pos = i;
            i++;
            int has_eq = 0;
            if(i < size && text[i] == '=') { has_eq = 1; i++; }
            /* Skip optional spaces */
            while(i < size && text[i] == ' ') i++;
            /* Optional $ */
            if(i < size && text[i] == '$') i++;
            /* Check for digit */
            if(i < size && text[i] >= '0' && text[i] <= '9') {
                /* This is a comparison operator — need to escape > */
                if(!in_fenced_code_block(text, size, gt_pos)) {
                    /* Insert \ before > */
                    unsigned old_size = buf->size;
                    buf_append_ch(buf, 0); /* grow by 1 */
                    memmove(buf->data + gt_pos + 1, buf->data + gt_pos, old_size - gt_pos);
                    buf->data[gt_pos] = '\\';
                    size = buf->size;
                    i++; /* adjust for inserted char */
                }
            }
        }

        /* Skip to end of line */
        while(i < size && text[i] != '\n') i++;
        if(i < size) i++; /* skip \n */

        (void) ls;
    }
}


/***************************
 ***  Main heal function ***
 ***************************/

int md_heal(const char* input, unsigned input_size,
            void (*process_output)(const char*, unsigned, void*),
            void* userdata)
{
    HEAL_BUF buf;

    if(input_size == 0) {
        return 0;
    }

    buf_init(&buf, input_size + 64);
    if(!buf.data) return -1;

    /* Copy input to buffer */
    buf_append(&buf, input, input_size);

    /* Strip trailing single space (preserve double space for line break) */
    if(buf.size > 0 && buf.data[buf.size - 1] == ' ') {
        if(buf.size < 2 || buf.data[buf.size - 2] != ' ')
            buf.size--;
    }

    /* Apply healers in priority order (matching remend's priority system):
     * -10: comparison operators
     * -5:  HTML tags
     *  0:  setext headings
     * 10:  links/images
     * 20:  bold-italic (***)
     * 30:  bold (**)
     * 40:  italic (__)
     * 41:  italic (*)
     * 42:  italic (_)
     * 50:  inline code
     * 60:  strikethrough
     * 70:  katex
     * last: code blocks (fenced)
     */
    heal_comparison_operators(&buf);
    heal_html_tag(&buf);
    heal_setext_heading(&buf);
    heal_links_and_images(&buf);
    heal_bold_italic(&buf);
    heal_bold(&buf);
    heal_italic_double_underscore(&buf);
    heal_italic_asterisk(&buf);
    heal_italic_underscore(&buf);
    heal_inline_code(&buf);
    heal_strikethrough(&buf);
    heal_katex(&buf);
    heal_code_block(&buf);

    /* Output result */
    if(buf.size > 0)
        process_output(buf.data, buf.size, userdata);

    buf_free(&buf);
    return 0;
}
