/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef SLICE_H_
#define SLICE_H_

#include <stdlib.h>


/*

  A Slice is representing a subsection of an Vec, borrowing the
  latter's data.

  See `Vec_Slice_unsafe` and `Vec_Slice_safer` in `Vec.h`.

  See `LSlice.h` for a variant that carries around the original
  base pointer at the cost of a 3rd field.

 */

#define DEFTYPE_Slice_(T)                       \
    typedef struct {                            \
        T *start;                               \
        T *end;                                 \
    } Slice_##T;

#define Slice_length(s)                         \
    ((s).end - (s).start)

#define Slice_start(s)                          \
    ((s).start)

#define Slice_end(s)                            \
    ((s).end)

#define Slice_ref_start_unsafe(s)               \
    (*Slice_start(s))

#define Slice_get_unsafe(s)                     \
    (*((s).start++))

#define Slice_is_empty(s)                       \
    ((s).start > (s).end)



#endif /* SLICE_H_ */
