/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef VEC_H_
#define VEC_H_

#include <stdlib.h>
#include "macro-util.h"
#include "Slice.h"
#include "LSlice.h"

#define Vec_(T) XCAT(Vec_, T)

#define DEFTYPE_Vec_(T)                         \
    typedef struct {                            \
        bool needs_freeing;                     \
        size_t length;                          \
        T *data                                 \
    } Vec_(T);


#define Vec_release(s)                          \
    if ((s).needs_freeing) {                    \
        free((s).data);                         \
    }

#define Vec_T(s)                                \
    __typeof__((s).data[0])


// These need #include "LSlice.h" and DEFTYPE_LSlice_(T)

#define Vec_LSlice_unsafe(T, s, startpos, endpos)        \
    (LSlice_(T)) {                                       \
        .startpos = startpos,                            \
        .endpos = endpos,                                \
        .data = (s).data                                 \
    }

#define Vec_LSlice_safer(T, s, startpos, endpos)                 \
    (LSlice_(T)) {                                               \
        .startpos = (assert(startpos <= (s).length), startpos),  \
        .endpos = (assert(endpos <= (s).length), endpos)         \
        .data = (s).data                                         \
    }


// These need DEFTYPE_Slice_(T)

#define Vec_sub_Slice_unsafe(T, s, startpos, endpos)         \
    (Slice_(T)) {                                            \
        .start = (s).data + (startpos),                      \
        .endpos = (s).data + (endpos)                        \
    }

#define Vec_sub_Slice(T, s, startpos, endpos)                \
    (Slice_(T)) {                                            \
        .start = (assert((startpos) <= (s).length),          \
                  (s).data + (startpos)),                    \
        .end = (assert((endpos) <= (s).length),              \
                (s).data + (endpos))                         \
    }

#define Vec_to_Slice(T, s)                                   \
    (Slice_(T)) {                                            \
        .start = (s).data,                                   \
        .end = (s).data + (s).length                         \
    }


#define Vec_for_each(T, var, vec, body)         \
    for (size_t _Vec_for_each_i = 0;            \
         _Vec_for_each_i < (vec).length;        \
         _Vec_for_each_i++) {                   \
        T var = (vec).data[_Vec_for_each_i];    \
        body;                                   \
    }

#define Vec_for_each_ptr(T, var, vec, body)     \
    for (size_t _Vec_for_each_i = 0;            \
         _Vec_for_each_i < (vec).length;        \
         _Vec_for_each_i++) {                   \
        T *var = &(vec).data[_Vec_for_each_i];  \
        body;                                   \
    }

// ^ XX Bummer: can the approach from video solve it to avoid body
// argument? var has to be inside the block, otherwise access out of
// bounds. (If macros had ability to define macros, then var access
// could be defined as a macro.)


#endif /* VEC_H_ */
