/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef IO_H_
#define IO_H_

#include "string.h"
#include "mem.h" /* xstrdup */


static
String strerror_String(int err) {
#define EBUFSIZ 256
    char msg[EBUFSIZ];
    strerror_r(err, msg, EBUFSIZ);
    return (String){ true, xstrdup(msg) };
#undef EBUFSIZ
}

#endif /* IO_H_ */
