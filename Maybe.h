/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef OPTION_H_
#define OPTION_H_

#include <stdbool.h>


#define DEFTYPE_Option(T)                       \
    typedef struct {                            \
        bool is_nothing;                        \
        T value;                                \
    } Option_##T;

#define None(T)                                 \
    (Option_##T) { true, default_##T }
#define Some(T, val)                            \
    (Option_##T) { false, val }

#define Option_is_some(v) (!(v).is_nothing)


#endif /* OPTION_H_ */
