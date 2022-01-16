/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef ARRAY_H_
#define ARRAY_H_

#include <stdlib.h>


#define DEFTYPE_Array_(T)                       \
    typedef struct {                            \
        bool needs_freeing;                     \
        size_t length;                          \
        T *data                                 \
    } Array_##T;


#define array_release(s)                        \
    if ((s).needs_freeing) {                    \
        free((s).data);                         \
    }

#define array_T(s)                              \
    __typeof__((s).data[0])

// needs #include "slice.h" and DEFTYPE_Slice_(T)
#define array_slice_unsafe(T, s, startpos, endpos)      \
    (Slice_##T) {                                       \
        .startpos = startpos,                           \
        .endpos = endpos,                               \
        .data = (s).data                                \
    }

#define array_slice_safer(T, s, startpos, endpos)               \
    (Slice_##T) {                                               \
        .startpos = (assert(startpos <= (s).length), startpos), \
        .endpos = (assert(endpos <= (s).length), endpos)        \
        .data = (s).data                                        \
    }


#endif /* ARRAY_H_ */
