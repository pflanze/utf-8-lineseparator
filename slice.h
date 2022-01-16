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

  See `array_slice_unsafe` and `array_slice_safer` in `array.h`.

 */

#define DEFTYPE_Slice_(T)                       \
    typedef struct {                            \
        size_t startpos;                        \
        size_t endpos;                          \
        T *data;                                \
    } Slice_##T;

#define slice_length(s)                         \
    ((s).endpos - (s).startpos)

#define slice_start(s)                          \
    (&(s).data[(s).startpos])

#define slice_end(s)                            \
    (&(s).data[(s).endpos])

#define slice_ref_start_unsafe(s)               \
    (*slice_start(s))

#define slice_get_unsafe(s)                     \
    (s).data[(s).startpos++]

#define slice_is_empty(s)                       \
    ((s).startpos >= (s).endpos)



#endif /* SLICE_H_ */
