/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef MAYBE_H_
#define MAYBE_H_

#include <stdbool.h>


#define DEFTYPE_Maybe_(T)                       \
    typedef struct {                            \
        bool is_nothing;                        \
        T value;                                \
    } Maybe_##T;

#define Nothing(T)                              \
    (Maybe_##T) { true, default_##T }
#define Just(T, val)                            \
    (Maybe_##T) { false, val }

#define Maybe_is_just(v) (!(v).is_nothing)


#endif /* MAYBE_H_ */
