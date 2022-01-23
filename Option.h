/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef OPTION_H_
#define OPTION_H_

#include <stdbool.h>
#include "macro-util.h"


#define Option(T) XCAT(Option_,T)

#define DEFTYPE_Option(T)                       \
    typedef struct {                            \
        bool is_none;                        \
        T value;                                \
    } Option(T);

#define None(T)                                 \
    (Option(T)) { true, XCAT(default_,T) }
#define Some(T, val)                            \
    (Option(T)) { false, val }

#define Option_is_some(v) (!(v).is_none)


#endif /* OPTION_H_ */
