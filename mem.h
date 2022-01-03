/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef MEM_H_
#define MEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static
void die_outofmemory() {
    fprintf(stderr, "Out of memory, aborting\n");
    abort();
}

static
char *xstrdup(const char *str) {
    char *res= strdup(str);
    if (!res) die_outofmemory();
    return res;
}

#endif /* MEM_H_ */
