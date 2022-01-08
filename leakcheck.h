/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef LEAKCHECK_H_
#define LEAKCHECK_H_

/*
  Simple leak checker, that simply verifies whether malloc calls are
  compensated by as many free calls.

  Because the leak sanitizer is somehow disabled or ignored when
  running in AFL's persistent mode (the failures that should be
  reported after leaving the main loop, are not detected).
*/

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


// Should be in a leakcheck.c but we're currently using a single
// binary object for everything.
int32_t leakcheck_active_allocs = 0;

static
void leakcheck_verify(bool abort_on_failure) {
    if (leakcheck_active_allocs != 0) {
        fprintf(stderr,
                "*** leakcheck failure: leakcheck_active_allocs = %i\n",
                leakcheck_active_allocs);
        if (abort_on_failure) abort();
    }
}

static
void *leakcheck_malloc(size_t x) {
    void *p = malloc(x);
    if (p) leakcheck_active_allocs++;
    return p;
}

static
void leakcheck_free(void *p) {
    free(p);
    /* if (p) */ leakcheck_active_allocs--;
}

static
char *leakcheck_strdup(const char *s) {
    char *t = strdup(s);
    if (t) leakcheck_active_allocs++;
    return t;
}


#define malloc(x) leakcheck_malloc(x)
#define free(x) leakcheck_free(x)
#define strdup(x) leakcheck_strdup(x)


#endif /* LEAKCHECK_H_ */
