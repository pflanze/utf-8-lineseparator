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

static inline
void *xmalloc(size_t size) {
    void *p = malloc(size);
    if (!p) die_outofmemory();
    return p;
}


static
char *xstrdup(const char *str) {
    char *res= strdup(str);
    if (!res) die_outofmemory();
    return res;
}


void *xmemcpy(const void *src, size_t n) {
    void *p = xmalloc(n);
    memcpy(p, src, n);
    return p;
}


#endif /* MEM_H_ */
