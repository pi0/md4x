/*
 * MD4X — Shared component property parser
 *
 * Parses the raw props string from {key="value" bool #id .class :bind='json'}
 * into a structured intermediate representation that renderers consume.
 *
 * Copyright (c) md4x contributors
 * Licensed under MIT license.
 */

#ifndef MD4X_PROPS_H
#define MD4X_PROPS_H

#include "md4x.h"
#include <string.h>

#define MD_MAX_PROPS        32
#define MD_CLASS_BUF_SIZE   512

typedef enum MD_PROP_TYPE {
    MD_PROP_STRING,     /* key="value", key='value', or key=value */
    MD_PROP_BOOLEAN,    /* bare word (no value) */
    MD_PROP_BIND        /* :key='value' (JSON passthrough) */
} MD_PROP_TYPE;

typedef struct MD_PROP {
    MD_PROP_TYPE type;
    const MD_CHAR* key;     /* Key name (points into raw string, NOT null-terminated) */
    MD_SIZE key_size;
    const MD_CHAR* value;   /* Value (points into raw string, NOT null-terminated). NULL for boolean. */
    MD_SIZE value_size;
} MD_PROP;

typedef struct MD_PARSED_PROPS {
    MD_PROP props[MD_MAX_PROPS];
    int n_props;

    /* Merged class names (space-separated). */
    MD_CHAR class_buf[MD_CLASS_BUF_SIZE];
    MD_SIZE class_len;

    /* ID shorthand from #id (last one wins). Points into raw string. */
    const MD_CHAR* id;
    MD_SIZE id_size;
} MD_PARSED_PROPS;


/* Parse a raw props string into structured MD_PARSED_PROPS.
 * The raw string should be the content between { and } (exclusive).
 * All key/value pointers reference the original raw buffer (zero-copy). */
static void
md_parse_props(const MD_CHAR* raw, MD_SIZE size, MD_PARSED_PROPS* out)
{
    MD_OFFSET i = 0;

    memset(out, 0, sizeof(MD_PARSED_PROPS));

    if(raw == NULL || size == 0)
        return;

    while(i < size) {
        /* Skip whitespace. */
        while(i < size && (raw[i] == ' ' || raw[i] == '\t'))
            i++;
        if(i >= size)
            break;

        if(raw[i] == '#') {
            /* #id shorthand → store as id (last wins). */
            MD_OFFSET start = ++i;
            while(i < size && raw[i] != ' ' && raw[i] != '\t' && raw[i] != '}')
                i++;
            if(i > start) {
                out->id = raw + start;
                out->id_size = i - start;
            }
        }
        else if(raw[i] == '.') {
            /* .class shorthand → append to merged class buffer. */
            MD_OFFSET start = ++i;
            while(i < size && raw[i] != ' ' && raw[i] != '\t' && raw[i] != '}' && raw[i] != '.')
                i++;
            if(i > start) {
                MD_SIZE len = i - start;
                if(out->class_len > 0 && out->class_len + 1 < MD_CLASS_BUF_SIZE) {
                    out->class_buf[out->class_len++] = ' ';
                }
                if(out->class_len + len < MD_CLASS_BUF_SIZE) {
                    memcpy(out->class_buf + out->class_len, raw + start, len);
                    out->class_len += len;
                }
            }
        }
        else {
            /* key="value", key='value', key=value, :key='json', or bare boolean. */
            MD_OFFSET key_start = i;
            int is_bind = 0;

            if(raw[i] == ':') {
                is_bind = 1;
                key_start = ++i;
            }

            while(i < size && raw[i] != '=' && raw[i] != ' ' && raw[i] != '\t' && raw[i] != '}')
                i++;

            if(i > key_start && i < size && raw[i] == '=') {
                /* key=... */
                MD_OFFSET key_end = i;
                i++;  /* skip '=' */

                if(i < size && (raw[i] == '"' || raw[i] == '\'')) {
                    /* Quoted value. */
                    char quote = raw[i];
                    MD_OFFSET val_start = ++i;
                    while(i < size && raw[i] != quote)
                        i++;

                    if(out->n_props < MD_MAX_PROPS) {
                        MD_PROP* p = &out->props[out->n_props++];
                        p->type = is_bind ? MD_PROP_BIND : MD_PROP_STRING;
                        p->key = raw + key_start;
                        p->key_size = key_end - key_start;
                        p->value = raw + val_start;
                        p->value_size = i - val_start;
                    }
                    if(i < size) i++;  /* skip closing quote */
                } else {
                    /* Unquoted value. */
                    MD_OFFSET val_start = i;
                    while(i < size && raw[i] != ' ' && raw[i] != '\t' && raw[i] != '}')
                        i++;

                    if(out->n_props < MD_MAX_PROPS) {
                        MD_PROP* p = &out->props[out->n_props++];
                        p->type = is_bind ? MD_PROP_BIND : MD_PROP_STRING;
                        p->key = raw + key_start;
                        p->key_size = key_end - key_start;
                        p->value = raw + val_start;
                        p->value_size = i - val_start;
                    }
                }
            } else if(i > key_start) {
                /* Bare word → boolean prop. */
                if(out->n_props < MD_MAX_PROPS) {
                    MD_PROP* p = &out->props[out->n_props++];
                    p->type = MD_PROP_BOOLEAN;
                    p->key = raw + key_start;
                    p->key_size = i - key_start;
                    p->value = NULL;
                    p->value_size = 0;
                }
            }
        }
    }
}


#endif /* MD4X_PROPS_H */
