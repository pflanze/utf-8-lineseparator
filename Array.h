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


#define Array_release(s)                        \
    if ((s).needs_freeing) {                    \
        free((s).data);                         \
    }

#define Array_T(s)                              \
    __typeof__((s).data[0])

// needs #include "LSlice.h" and DEFTYPE_LSlice_(T)
#define Array_LSlice_unsafe(T, s, startpos, endpos)      \
    (LSlice_##T) {                                       \
        .startpos = startpos,                            \
        .endpos = endpos,                                \
        .data = (s).data                                 \
    }

#define Array_LSlice_safer(T, s, startpos, endpos)               \
    (LSlice_##T) {                                               \
        .startpos = (assert(startpos <= (s).length), startpos),  \
        .endpos = (assert(endpos <= (s).length), endpos)         \
        .data = (s).data                                         \
    }

// needs #include "Slice.h" and DEFTYPE_Slice_(T)
#define Array_Slice_unsafe(T, s, startpos, endpos)       \
    (Slice_##T) {                                        \
        .start = (s).data + (startpos),                  \
        .endpos = (s).data + (endpos)                    \
    }

#define Array_Slice_safer(T, s, startpos, endpos)            \
    (Slice_##T) {                                            \
        .start = (assert((startpos) <= (s).length),          \
                  (s).data + (startpos)),                    \
        .end = (assert((endpos) <= (s).length),              \
                (s).data + (endpos))                         \
    }


#endif /* ARRAY_H_ */
