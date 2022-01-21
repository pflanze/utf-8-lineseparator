/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

/*
  This is used to pass around read-only strings, as C strings and the
  information whether they need to be freed on release.

  String_clone does not copy the memory if needs_freeing is false,
  since it is then assumed that it is memory with static life time.
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

#define literal_String(literal)                 \
    (String) { false, "" literal }
#define borrowing_String(v)                     \
    (String) { false, (v) }
#define allocated_String(v)                     \
    (String) { true, (v) }
#define copy_String(v)                          \
    (String) { true, xstrdup(v) }

static
void String_release(String s) {
    if (s.needs_freeing) {
        free((void*)s.str);
    }
}

/*
  String_clone does not copy the memory if needs_freeing is false,
  since it is then assumed that it is memory with static life time.

  If you want a String_copy that always copies for mutation (or if the
  above assumption is violated), you'll have to add it to this
  library. Note that so far nothing in this library is designed for
  mutation.
*/
static
String String_clone(const String *s) {
    if (s->needs_freeing) {
        return copy_String(s->str);
    } else {
        return *s;
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
    return copy_String(out);
#undef QBUFSIZ
}


#endif /* STRING_H_ */
