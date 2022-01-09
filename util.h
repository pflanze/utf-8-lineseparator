/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>
#include <stdio.h>


#define UNUSED __attribute__ ((unused))

#define MAX2(a,b)                               \
    ((a) < (b) ? (b) : (a))

#define MAX3(a,b,c)                             \
    ((a) < (b) ? MAX2(b, c) : MAX2(a, c))


#define ABORT() abort()

#define WARN_(msg, ...)                                         \
    fprintf(stderr, msg "\n", __VA_ARGS__)
#define DIE_(msg, ...)                                          \
    do { fprintf(stderr, msg "\n", __VA_ARGS__); ABORT(); } while(0)
#define WARN(msg)                                               \
    fprintf(stderr, "%s\n", msg)
#define DIE(msg)                                                \
    do { fprintf(stderr, "%s\n", msg); ABORT(); } while(0)


#define FOR_RANGE(var, from, to)                        \
    __typeof__ (to) __for_range_to = (to);              \
    for (int var = (from); var < __for_range_to; var++)


#define XSTR(s) STR(s)
#define STR(s) #s

// An alternative to assert from assert.h that works better with
// coverage for AFL (runaflcov make target)
#define ASSERT(expr)                            \
    if (!(expr)) {                              \
    WARN_("ASSERT failure for %s on %s:%i",     \
          XSTR(expr), __FILE__, __LINE__);      \
        abort();                                \
    }

// A hack to lead AFL towards a particular number:
#define DEF_LADDER_FOR(var)                     \
    int ladder_##var##_4;                       \
    int ladder_##var##_8;                       \
    int ladder_##var##_12;                      \
    int ladder_##var##_16;
    
#define LADDER_UP_TO(var, from, to)                       \
    {                                                     \
        __typeof__ (from) _from = (from);                 \
        __typeof__ (to) _to = (to);                       \
        __typeof__ (to) _diff = _to - _from;              \
        if (var > (_to - (_diff >> 4))) {                 \
            ladder_##var##_4++;                           \
            if (var > (_to - (_diff >> 8))) {             \
                ladder_##var##_8++;                       \
                if (var > (_to - (_diff >> 12))) {        \
                    ladder_##var##_12++;                  \
                    if (var > (_to - (_diff >> 16))) {    \
                        ladder_##var##_16++;              \
                    }                                     \
                }                                         \
            }                                             \
        }                                                 \
    }


#endif /* UTIL_H_ */
