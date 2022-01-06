/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef STRING_PERROR_H_
#define STRING_PERROR_H_

#include <stdio.h>
#include <string.h>
#include "string.h"


static
int perror_str(const char *fmt, const char *arg1) {
#define EBUFSIZ 256
    char msg[EBUFSIZ];
    strerror_r(errno, msg, EBUFSIZ);
    int res = fprintf(stderr, fmt, arg1);
    if (res < 0) return res;
    return fprintf(stderr, ": %s\n", msg);
#undef EBUFSIZ
}

static
int perror_string(const char *fmt, String str /* taking ownership */) {
    int res = perror_str(fmt, str.str);
    string_release(str);
    return res;
}


#endif /* STRING_PERROR_H_ */
