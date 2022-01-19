/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef IO_H_
#define IO_H_

#include "String.h"


static
String strerror_String(int err) {
#define EBUFSIZ 256
    char msg[EBUFSIZ];
    strerror_r(err, msg, EBUFSIZ);
    return String_copy(msg);
#undef EBUFSIZ
}

#endif /* IO_H_ */
