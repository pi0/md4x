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

#ifndef MD4X_JSON_H
#define MD4X_JSON_H

#include <stdio.h>
#include <string.h>
#include <yaml.h>
#include "md4x.h"

#ifdef _WIN32
    #define json_snprintf _snprintf
#else
    #define json_snprintf snprintf
#endif


/* Writer that streams output through a callback. */
typedef struct {
    void (*process_output)(const MD_CHAR*, MD_SIZE, void*);
    void* userdata;
} JSON_WRITER;


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


/*******************************
 ***  YAML-to-JSON helpers   ***
 *******************************/

/* Helper: check if a string matches a value (case-insensitive, known length). */
static int
yaml_streq_ci(const char* s, MD_SIZE len, const char* lit, MD_SIZE lit_len)
{
    MD_SIZE i;
    if(len != lit_len) return 0;
    for(i = 0; i < len; i++) {
        char c = s[i];
        if(c >= 'A' && c <= 'Z') c += 32;
        if(c != lit[i]) return 0;
    }
    return 1;
}

/* Helper: check if a value string looks like a JSON number. */
static int
yaml_is_number(const char* s, MD_SIZE len)
{
    MD_SIZE i = 0;
    int has_digit = 0;
    int has_dot = 0;

    if(len == 0) return 0;

    /* Optional leading sign. */
    if(s[0] == '-' || s[0] == '+') {
        i++;
        if(i >= len) return 0;
    }

    for(; i < len; i++) {
        if(s[i] >= '0' && s[i] <= '9') {
            has_digit = 1;
        } else if(s[i] == '.' && !has_dot) {
            has_dot = 1;
        } else {
            return 0;
        }
    }
    return has_digit;
}

/* Write a YAML scalar as a typed JSON value.
 * Applies YAML 1.1 type resolution for plain scalars. */
static void
json_write_yaml_scalar(JSON_WRITER* w, const yaml_event_t* event)
{
    const char* val = (const char*) event->data.scalar.value;
    MD_SIZE len = (MD_SIZE) event->data.scalar.length;
    yaml_scalar_style_t style = event->data.scalar.style;

    /* Quoted scalars are always strings. */
    if(style == YAML_SINGLE_QUOTED_SCALAR_STYLE
       || style == YAML_DOUBLE_QUOTED_SCALAR_STYLE) {
        json_write_string(w, val, len);
        return;
    }

    /* Plain scalars: apply type coercion. */
    if(len == 0) {
        json_write_str(w, "null");
        return;
    }
    if(yaml_streq_ci(val, len, "null", 4) || (len == 1 && val[0] == '~')) {
        json_write_str(w, "null");
        return;
    }
    if(yaml_streq_ci(val, len, "true", 4) || yaml_streq_ci(val, len, "yes", 3)
       || yaml_streq_ci(val, len, "on", 2)) {
        json_write_str(w, "true");
        return;
    }
    if(yaml_streq_ci(val, len, "false", 5) || yaml_streq_ci(val, len, "no", 2)
       || yaml_streq_ci(val, len, "off", 3)) {
        json_write_str(w, "false");
        return;
    }
    if(yaml_is_number(val, len)) {
        json_write(w, val, len);
        return;
    }

    /* Default: string (also covers literal/folded block scalars). */
    json_write_string(w, val, len);
}

/* Forward declarations for recursive YAML-to-JSON writing. */
static int json_write_yaml_value(JSON_WRITER* w, yaml_parser_t* yp);

/* Write a YAML mapping as JSON object key-value pairs (without outer braces).
 * Assumes MAPPING_START has been consumed. Returns number of pairs, or -1 on error. */
static int
json_write_yaml_mapping(JSON_WRITER* w, yaml_parser_t* yp)
{
    yaml_event_t event;
    int n = 0;

    while(1) {
        if(!yaml_parser_parse(yp, &event))
            return -1;

        if(event.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }

        if(event.type != YAML_SCALAR_EVENT) {
            yaml_event_delete(&event);
            return -1;
        }

        if(n > 0)
            json_write(w, ",", 1);

        /* Write key. */
        json_write(w, "\"", 1);
        json_write_escaped(w, (const char*) event.data.scalar.value,
                           (MD_SIZE) event.data.scalar.length);
        json_write_str(w, "\":");
        yaml_event_delete(&event);

        /* Write value (recursive). */
        if(json_write_yaml_value(w, yp) < 0)
            return -1;

        n++;
    }
    return n;
}

/* Write a YAML sequence as a JSON array.
 * Assumes SEQUENCE_START has been consumed. Returns 0 on success, -1 on error. */
static int
json_write_yaml_sequence(JSON_WRITER* w, yaml_parser_t* yp)
{
    yaml_event_t event;
    int n = 0;

    json_write(w, "[", 1);

    while(1) {
        if(!yaml_parser_parse(yp, &event))
            return -1;

        if(event.type == YAML_SEQUENCE_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }

        if(n > 0)
            json_write(w, ",", 1);

        if(event.type == YAML_SCALAR_EVENT) {
            json_write_yaml_scalar(w, &event);
            yaml_event_delete(&event);
        } else if(event.type == YAML_MAPPING_START_EVENT) {
            yaml_event_delete(&event);
            json_write(w, "{", 1);
            if(json_write_yaml_mapping(w, yp) < 0)
                return -1;
            json_write(w, "}", 1);
        } else if(event.type == YAML_SEQUENCE_START_EVENT) {
            yaml_event_delete(&event);
            if(json_write_yaml_sequence(w, yp) < 0)
                return -1;
        } else {
            yaml_event_delete(&event);
            return -1;
        }

        n++;
    }

    json_write(w, "]", 1);
    return 0;
}

/* Write the next YAML value (scalar, mapping, or sequence) as JSON.
 * Returns 0 on success, -1 on error. */
static int
json_write_yaml_value(JSON_WRITER* w, yaml_parser_t* yp)
{
    yaml_event_t event;

    if(!yaml_parser_parse(yp, &event))
        return -1;

    if(event.type == YAML_SCALAR_EVENT) {
        json_write_yaml_scalar(w, &event);
        yaml_event_delete(&event);
        return 0;
    }
    if(event.type == YAML_MAPPING_START_EVENT) {
        yaml_event_delete(&event);
        json_write(w, "{", 1);
        if(json_write_yaml_mapping(w, yp) < 0)
            return -1;
        json_write(w, "}", 1);
        return 0;
    }
    if(event.type == YAML_SEQUENCE_START_EVENT) {
        yaml_event_delete(&event);
        return json_write_yaml_sequence(w, yp);
    }
    if(event.type == YAML_ALIAS_EVENT) {
        yaml_event_delete(&event);
        json_write_str(w, "null");
        return 0;
    }

    yaml_event_delete(&event);
    return -1;
}

/* Write parsed YAML frontmatter as JSON props using libyaml.
 * Supports nested objects, arrays, and all YAML scalar types.
 * Returns number of top-level props written. */
static int
json_write_yaml_props(JSON_WRITER* w, const char* text, MD_SIZE size)
{
    yaml_parser_t yp;
    yaml_event_t event;
    int n_written = 0;

    if(!yaml_parser_initialize(&yp))
        return 0;

    yaml_parser_set_input_string(&yp, (const unsigned char*) text, size);

    /* Consume STREAM_START. */
    if(!yaml_parser_parse(&yp, &event)) goto done;
    if(event.type != YAML_STREAM_START_EVENT) { yaml_event_delete(&event); goto done; }
    yaml_event_delete(&event);

    /* Consume DOCUMENT_START. */
    if(!yaml_parser_parse(&yp, &event)) goto done;
    if(event.type != YAML_DOCUMENT_START_EVENT) { yaml_event_delete(&event); goto done; }
    yaml_event_delete(&event);

    /* Expect top-level MAPPING_START. */
    if(!yaml_parser_parse(&yp, &event)) goto done;
    if(event.type != YAML_MAPPING_START_EVENT) { yaml_event_delete(&event); goto done; }
    yaml_event_delete(&event);

    n_written = json_write_yaml_mapping(w, &yp);
    if(n_written < 0) n_written = 0;

done:
    yaml_parser_delete(&yp);
    return n_written;
}

/* Convert a YAML document to JSON.
 * Handles any top-level value (mapping, sequence, scalar).
 * Returns 0 on success, -1 on error. */
static int
md_yaml_to_json(const MD_CHAR* input, MD_SIZE input_size,
                void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
                void* userdata)
{
    yaml_parser_t yp;
    yaml_event_t event;
    JSON_WRITER w;
    int ret = -1;

    w.process_output = process_output;
    w.userdata = userdata;

    if(!yaml_parser_initialize(&yp))
        return -1;

    yaml_parser_set_input_string(&yp, (const unsigned char*) input, input_size);

    /* Consume STREAM_START. */
    if(!yaml_parser_parse(&yp, &event)) goto done;
    if(event.type != YAML_STREAM_START_EVENT) { yaml_event_delete(&event); goto done; }
    yaml_event_delete(&event);

    /* Consume DOCUMENT_START. */
    if(!yaml_parser_parse(&yp, &event)) goto done;
    if(event.type != YAML_DOCUMENT_START_EVENT) { yaml_event_delete(&event); goto done; }
    yaml_event_delete(&event);

    /* Write the top-level value (any type). */
    if(json_write_yaml_value(&w, &yp) == 0)
        ret = 0;

done:
    yaml_parser_delete(&yp);
    return ret;
}


#endif  /* MD4X_JSON_H */
