/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

/*
  Utilities for writing macros
*/

#ifndef MACRO_UTIL_H_
#define MACRO_UTIL_H_


#define _STR(s) #s
#define XSTR(s) _STR(s)

#define _CAT(a, b) a##b
#define XCAT(a,b) _CAT(a,b)


#endif /* MACRO_UTIL_H_ */
