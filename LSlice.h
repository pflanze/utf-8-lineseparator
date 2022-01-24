/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef LSLICE_H_
#define LSLICE_H_

/*

  An LSlice is representing a subsection of an Vec, borrowing the
  latter's data.

  See `Vec_LSlice_unsafe` and `Vec_LSlice_safer` in `Vec.h`.

  See `LSlice.h` for a slice that has just 2, not 3 fields.
 */


#include <stdlib.h>
#include "macro-util.h"


#define LSlice_(T) XCAT(LSlice_, T)

#define DEFTYPE_LSlice_(T)                       \
    typedef struct {                            \
        size_t startpos;                        \
        size_t endpos;                          \
        T *data;                                \
    } LSlice_(T);

#define LSlice_length(s)                         \
    ((s).endpos - (s).startpos)

#define LSlice_start(s)                          \
    (&(s).data[(s).startpos])

#define LSlice_end(s)                            \
    (&(s).data[(s).endpos])

#define LSlice_ref_start_unsafe(s)               \
    (*LSlice_start(s))

#define LSlice_get_unsafe(s)                     \
    (s).data[(s).startpos++]

#define LSlice_is_empty(s)                       \
    ((s).startpos >= (s).endpos)



#endif /* LSLICE_H_ */
