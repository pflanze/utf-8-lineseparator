/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef SLICE_H_
#define SLICE_H_

#include <stdlib.h>


/*

  A Slice is representing a subsection of an Array, borrowing the
  latter's data.

  See `Array_Slice_unsafe` and `Array_Slice_safer` in `Array.h`.

 */

#define DEFTYPE_Slice_(T)                       \
    typedef struct {                            \
        size_t startpos;                        \
        size_t endpos;                          \
        T *data;                                \
    } Slice_##T;

#define Slice_length(s)                         \
    ((s).endpos - (s).startpos)

#define Slice_start(s)                          \
    (&(s).data[(s).startpos])

#define Slice_end(s)                            \
    (&(s).data[(s).endpos])

#define Slice_ref_start_unsafe(s)               \
    (*Slice_start(s))

#define Slice_get_unsafe(s)                     \
    (s).data[(s).startpos++]

#define Slice_is_empty(s)                       \
    ((s).startpos >= (s).endpos)



#endif /* SLICE_H_ */
