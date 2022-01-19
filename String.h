/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef STRING_H_
#define STRING_H_

#include <stdbool.h>
#include "mem.h"


typedef struct {
    bool needs_freeing;
    const char* str;
} String;

#define noString (String) { false, NULL }

#define String_literal(literal)                 \
    (String) { false, "" literal }
#define String_borrowing(v)                     \
    (String) { false, (v) }
#define String_allocated(v)                     \
    (String) { true, (v) }
#define String_copy(v)                          \
    (String) { true, xstrdup(v) }

static
void String_release(String s) {
    if (s.needs_freeing) {
        free((void*)s.str);
    }
}


static
String String_quote_sh(const char *str) {
#define QBUFSIZ 1024
    char out[QBUFSIZ];
    int out_i = 0;
#define PUSH(c)                                     \
    if (out_i < (QBUFSIZ-3-1-1)) {                  \
        out[out_i++] = c;                           \
    } else {                                        \
        goto push_error;                            \
    }

    size_t len = strlen(str);
    PUSH('\'');
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (c == '\'') {
            PUSH('\'');
            PUSH('\\');
            PUSH('\'');
            PUSH('\'');
        } else {
            PUSH(c);
        }
    }
    PUSH('\'');
    goto finish;
push_error:
    out[out_i++] = '\'';
    out[out_i++] = '.';
    out[out_i++] = '.';
    out[out_i++] = '.';
finish:
    out[out_i++] = '\0';
    return String_copy(out);
#undef QBUFSIZ
}


#endif /* STRING_H_ */
