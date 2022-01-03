/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef UTIL_H_
#define UTIL_H_

#define UNUSED __attribute__ ((unused))

#define MAX2(a,b)                               \
    ((a) < (b) ? (b) : (a))

#define MAX3(a,b,c)                             \
    ((a) < (b) ? MAX2(b, c) : MAX2(a, c))


#endif /* UTIL_H_ */
